#define DT_DRV_COMPAT ti_tlv320adc3100

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>

#include "tlv320adc3100.h"

LOG_MODULE_REGISTER(tlv320adc3100, CONFIG_SENSOR_LOG_LEVEL);

struct tlv320adc3100_data tlv320adc3100_driver_data;

static int tlv320adc3100_reg_write(const struct device *dev, uint8_t reg, uint8_t value)
{
    const struct tlv320adc3100_config *config = dev->config;
    uint8_t buf[2] = {reg, value};

    return i2c_write_dt(&config->i2c, buf, sizeof(buf));
}

static int tlv320adc3100_reg_read(const struct device *dev, uint8_t reg, uint8_t *value)
{
    const struct tlv320adc3100_config *config = dev->config;

    return i2c_write_read_dt(&config->i2c, &reg, 1, value, 1);
}

static int tlv320adc3100_reset(const struct device *dev)
{
    const struct tlv320adc3100_config *config = dev->config;
    int ret;

    // Toggle reset pin if available
    if (config->reset_gpio.port != NULL) {
        gpio_pin_set_dt(&config->reset_gpio, 0);
        k_msleep(1);
        gpio_pin_set_dt(&config->reset_gpio, 1);
        k_msleep(10);
    }

    // Software reset
    ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_RESET, 0x01);
    if (ret < 0) {
        return ret;
    }

    k_msleep(10);
    return 0;
}

static int tlv320adc3100_configure_pll(const struct device *dev)
{
    int ret;

    // Configure PLL for 32MHz MCLK input
    ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_CLK_GEN, 0x01);  // Enable PLL
    if (ret < 0) return ret;

    ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_PLL_P_R, 0x91);  // P=1, R=1
    if (ret < 0) return ret;

    ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_PLL_J, 0x08);    // J=8
    if (ret < 0) return ret;

    ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_PLL_D_MSB, 0x00); // D=0
    if (ret < 0) return ret;

    ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_PLL_D_LSB, 0x00);
    if (ret < 0) return ret;

    return 0;
}

static int tlv320adc3100_configure_adc(const struct device *dev)
{
    struct tlv320adc3100_data *data = dev->data;
    int ret;

    // Configure ADC clock
    ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_NDAC, 0x81);  // NDAC=1, powered on
    if (ret < 0) return ret;

    ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_MDAC, 0x81);  // MDAC=1, powered on
    if (ret < 0) return ret;

    ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_NADC, 0x81);  // NADC=1, powered on
    if (ret < 0) return ret;

    ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_MADC, 0x81);  // MADC=1, powered on
    if (ret < 0) return ret;

    ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_AOSR, 0x80);  // AOSR=128
    if (ret < 0) return ret;

    // Configure input routing based on channel selection
    switch (data->channel) {
        case 0: // IN3L
            ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_IN3L_2_LADC_CTL, 0x7C);
            break;
        case 1: // IN2L
            ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_IN2L_2_LADC_CTL, 0x7C);
            break;
        case 2: // IN2R
            ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_IN2R_2_RADC_CTL, 0x7C);
            break;
        default:
            return -EINVAL;
    }
    if (ret < 0) return ret;

    // Set volume
    ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_LADC_VOL, data->volume + 127);
    if (ret < 0) return ret;

    // Configure gain
    ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_AGC_MAX_GAIN, data->gain);
    if (ret < 0) return ret;

    // Enable -6dB mode if requested
    if (data->min6db) {
        ret = tlv320adc3100_reg_write(dev, TLV320ADC3100_ADC_DIGITAL, 0x02);
        if (ret < 0) return ret;
    }

    return 0;
}

int tlv320adc3100_configure(const struct device *dev, uint8_t channel,
                           int8_t volume, uint8_t gain, bool min6db)
{
    struct tlv320adc3100_data *data = dev->data;
    
    data->channel = channel;
    data->volume = volume;
    data->gain = gain;
    data->min6db = min6db;

    return tlv320adc3100_configure_adc(dev);
}

static int tlv320adc3100_init(const struct device *dev)
{
    const struct tlv320adc3100_config *config = dev->config;
    int ret;

    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    if (config->reset_gpio.port != NULL) {
        if (!device_is_ready(config->reset_gpio.port)) {
            LOG_ERR("Reset GPIO not ready");
            return -ENODEV;
        }

        ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure reset GPIO");
            return ret;
        }
    }

    ret = tlv320adc3100_reset(dev);
    if (ret < 0) {
        LOG_ERR("Failed to reset device");
        return ret;
    }

    ret = tlv320adc3100_configure_pll(dev);
    if (ret < 0) {
        LOG_ERR("Failed to configure PLL");
        return ret;
    }

    return 0;
}

#define TLV320ADC3100_INIT(inst)                                              \
    static struct tlv320adc3100_config tlv320adc3100_config_##inst = {       \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                   \
        .reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),      \
    };                                                                        \
                                                                             \
    DEVICE_DT_INST_DEFINE(inst,                                              \
                         tlv320adc3100_init,                                 \
                         NULL,                                               \
                         &tlv320adc3100_driver_data,                         \
                         &tlv320adc3100_config_##inst,                       \
                         POST_KERNEL,                                        \
                         CONFIG_SENSOR_INIT_PRIORITY,                        \
                         NULL);

DT_INST_FOREACH_STATUS_OKAY(TLV320ADC3100_INIT)
