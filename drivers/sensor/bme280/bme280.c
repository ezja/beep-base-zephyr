#define DT_DRV_COMPAT bosch_bme280

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "bme280.h"

LOG_MODULE_REGISTER(bme280, CONFIG_SENSOR_LOG_LEVEL);

static int bme280_reg_read(const struct device *dev, uint8_t reg,
                          uint8_t *data, uint8_t len)
{
    const struct bme280_config *config = dev->config;
    return i2c_write_read_dt(&config->i2c, &reg, 1, data, len);
}

static int bme280_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
    const struct bme280_config *config = dev->config;
    uint8_t buf[2] = {reg, val};
    return i2c_write_dt(&config->i2c, buf, sizeof(buf));
}

static int bme280_read_compensation(const struct device *dev)
{
    struct bme280_data *data = dev->data;
    uint8_t buf[32];
    int err;

    err = bme280_reg_read(dev, 0x88, buf, 24);
    if (err < 0) {
        return err;
    }

    data->dig_t1 = sys_get_le16(&buf[0]);
    data->dig_t2 = sys_get_le16(&buf[2]);
    data->dig_t3 = sys_get_le16(&buf[4]);
    data->dig_p1 = sys_get_le16(&buf[6]);
    data->dig_p2 = sys_get_le16(&buf[8]);
    data->dig_p3 = sys_get_le16(&buf[10]);
    data->dig_p4 = sys_get_le16(&buf[12]);
    data->dig_p5 = sys_get_le16(&buf[14]);
    data->dig_p6 = sys_get_le16(&buf[16]);
    data->dig_p7 = sys_get_le16(&buf[18]);
    data->dig_p8 = sys_get_le16(&buf[20]);
    data->dig_p9 = sys_get_le16(&buf[22]);
    data->dig_h1 = buf[25];

    err = bme280_reg_read(dev, 0xE1, buf, 7);
    if (err < 0) {
        return err;
    }

    data->dig_h2 = sys_get_le16(&buf[0]);
    data->dig_h3 = buf[2];
    data->dig_h4 = (buf[3] << 4) | (buf[4] & 0x0F);
    data->dig_h5 = (buf[5] << 4) | (buf[4] >> 4);
    data->dig_h6 = buf[6];

    return 0;
}

static int bme280_sample_fetch(const struct device *dev,
                             enum sensor_channel chan)
{
    struct bme280_data *data = dev->data;
    uint8_t buf[8];
    int32_t adc_temp, adc_press, adc_hum;
    int err;

    err = bme280_reg_read(dev, BME280_REG_PRESS_MSB, buf, 8);
    if (err < 0) {
        return err;
    }

    adc_press = (buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4);
    adc_temp = (buf[3] << 12) | (buf[4] << 4) | (buf[5] >> 4);
    adc_hum = (buf[6] << 8) | buf[7];

    /* Temperature compensation */
    int32_t var1, var2;
    var1 = ((((adc_temp >> 3) - ((int32_t)data->dig_t1 << 1))) *
            ((int32_t)data->dig_t2)) >> 11;
    var2 = (((((adc_temp >> 4) - ((int32_t)data->dig_t1)) *
              ((adc_temp >> 4) - ((int32_t)data->dig_t1))) >> 12) *
            ((int32_t)data->dig_t3)) >> 14;
    
    data->t_fine = var1 + var2;
    data->temperature = (data->t_fine * 5 + 128) >> 8;

    /* Pressure compensation */
    int64_t p;
    var1 = ((int64_t)data->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)data->dig_p6;
    var2 = var2 + ((var1 * (int64_t)data->dig_p5) << 17);
    var2 = var2 + (((int64_t)data->dig_p4) << 35);
    var1 = ((var1 * var1 * (int64_t)data->dig_p3) >> 8) +
           ((var1 * (int64_t)data->dig_p2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)data->dig_p1) >> 33;

    if (var1 == 0) {
        return -EIO;
    }

    p = 1048576 - adc_press;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)data->dig_p9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)data->dig_p8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)data->dig_p7) << 4);

    data->pressure = (uint32_t)p;

    /* Humidity compensation */
    int32_t h;
    h = (data->t_fine - ((int32_t)76800));
    h = (((((adc_hum << 14) - (((int32_t)data->dig_h4) << 20) -
            (((int32_t)data->dig_h5) * h)) + ((int32_t)16384)) >> 15) *
         (((((((h * ((int32_t)data->dig_h6)) >> 10) *
             (((h * ((int32_t)data->dig_h3)) >> 11) + ((int32_t)32768))) >> 10) +
           ((int32_t)2097152)) * ((int32_t)data->dig_h2) + 8192) >> 14));
    h = (h - (((((h >> 15) * (h >> 15)) >> 7) * ((int32_t)data->dig_h1)) >> 4));
    h = (h < 0) ? 0 : h;
    h = (h > 419430400) ? 419430400 : h;

    data->humidity = (uint32_t)(h >> 12);

    return 0;
}

static int bme280_channel_get(const struct device *dev,
                            enum sensor_channel chan,
                            struct sensor_value *val)
{
    struct bme280_data *data = dev->data;

    switch (chan) {
    case SENSOR_CHAN_AMBIENT_TEMP:
        val->val1 = data->temperature / 100;
        val->val2 = (data->temperature % 100) * 10000;
        break;
    case SENSOR_CHAN_PRESS:
        val->val1 = data->pressure / 256;
        val->val2 = (data->pressure % 256) * 1000000 / 256;
        break;
    case SENSOR_CHAN_HUMIDITY:
        val->val1 = data->humidity / 1024;
        val->val2 = (data->humidity % 1024) * 1000000 / 1024;
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static int bme280_init(const struct device *dev)
{
    const struct bme280_config *config = dev->config;
    uint8_t chip_id;
    int err;

    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    err = bme280_reg_read(dev, BME280_REG_CHIP_ID, &chip_id, 1);
    if (err < 0) {
        return err;
    }

    if (chip_id != BME280_CHIP_ID) {
        LOG_ERR("Wrong chip ID: %02x", chip_id);
        return -EINVAL;
    }

    /* Reset the sensor */
    err = bme280_reg_write(dev, BME280_REG_RESET, BME280_RESET_CMD);
    if (err < 0) {
        return err;
    }

    k_msleep(2);

    err = bme280_read_compensation(dev);
    if (err < 0) {
        return err;
    }

    /* Configure the sensor */
    err = bme280_reg_write(dev, BME280_REG_CTRL_HUM, config->osr_humidity);
    if (err < 0) {
        return err;
    }

    err = bme280_reg_write(dev, BME280_REG_CTRL_MEAS,
                          (config->osr_temp << 5) |
                          (config->osr_press << 2) |
                          config->mode);
    if (err < 0) {
        return err;
    }

    err = bme280_reg_write(dev, BME280_REG_CONFIG,
                          (config->standby_time << 5) |
                          (config->filter << 2));
    if (err < 0) {
        return err;
    }

    return 0;
}

static const struct sensor_driver_api bme280_api = {
    .sample_fetch = bme280_sample_fetch,
    .channel_get = bme280_channel_get,
};

#define BME280_CONFIG_INIT(inst)                                              \
    {                                                                         \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                   \
        .osr_press = DT_INST_PROP_OR(inst, pressure_oversampling,            \
                                    BME280_OSR_16X),                          \
        .osr_temp = DT_INST_PROP_OR(inst, temperature_oversampling,          \
                                   BME280_OSR_16X),                           \
        .osr_humidity = DT_INST_PROP_OR(inst, humidity_oversampling,         \
                                       BME280_OSR_16X),                       \
        .standby_time = DT_INST_PROP_OR(inst, standby_time,                  \
                                       BME280_STANDBY_1000_MS),              \
        .filter = DT_INST_PROP_OR(inst, filter, BME280_FILTER_4),           \
        .mode = DT_INST_PROP_OR(inst, mode, BME280_MODE_NORMAL),            \
    }

#define BME280_DEFINE(inst)                                                  \
    static struct bme280_data bme280_data_##inst;                           \
    static const struct bme280_config bme280_config_##inst =                \
        BME280_CONFIG_INIT(inst);                                           \
                                                                            \
    DEVICE_DT_INST_DEFINE(inst,                                             \
                         bme280_init,                                        \
                         NULL,                                               \
                         &bme280_data_##inst,                               \
                         &bme280_config_##inst,                             \
                         POST_KERNEL,                                        \
                         CONFIG_SENSOR_INIT_PRIORITY,                        \
                         &bme280_api);

DT_INST_FOREACH_STATUS_OKAY(BME280_DEFINE)
