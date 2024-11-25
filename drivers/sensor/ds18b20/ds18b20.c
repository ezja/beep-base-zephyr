/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_ds18b20

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "ds18b20.h"

LOG_MODULE_REGISTER(ds18b20, CONFIG_SENSOR_LOG_LEVEL);

static int ds18b20_write_scratchpad(const struct device *dev,
                                  uint8_t th, uint8_t tl, uint8_t config)
{
    const struct ds18b20_config *cfg = dev->config;
    uint8_t buf[3];
    int ret;

    ret = w1_reset(cfg->w1_dev);
    if (ret < 0) {
        return ret;
    }

    ret = w1_write_byte(cfg->w1_dev, W1_CMD_MATCH_ROM);
    if (ret < 0) {
        return ret;
    }

    ret = w1_write_block(cfg->w1_dev, (uint8_t *)&cfg->w1_config.rom,
                        sizeof(struct w1_rom));
    if (ret < 0) {
        return ret;
    }

    ret = w1_write_byte(cfg->w1_dev, DS18B20_CMD_WRITE_SCRATCHPAD);
    if (ret < 0) {
        return ret;
    }

    buf[0] = th;
    buf[1] = tl;
    buf[2] = config;

    ret = w1_write_block(cfg->w1_dev, buf, sizeof(buf));
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int ds18b20_read_scratchpad(const struct device *dev, uint8_t *buf)
{
    const struct ds18b20_config *cfg = dev->config;
    int ret;

    ret = w1_reset(cfg->w1_dev);
    if (ret < 0) {
        return ret;
    }

    ret = w1_write_byte(cfg->w1_dev, W1_CMD_MATCH_ROM);
    if (ret < 0) {
        return ret;
    }

    ret = w1_write_block(cfg->w1_dev, (uint8_t *)&cfg->w1_config.rom,
                        sizeof(struct w1_rom));
    if (ret < 0) {
        return ret;
    }

    ret = w1_write_byte(cfg->w1_dev, DS18B20_CMD_READ_SCRATCHPAD);
    if (ret < 0) {
        return ret;
    }

    ret = w1_read_block(cfg->w1_dev, buf, 9);
    if (ret < 0) {
        return ret;
    }

    if (w1_crc8(buf, 8) != buf[8]) {
        return -EIO;
    }

    return 0;
}

int ds18b20_trigger_conversion(const struct device *dev)
{
    const struct ds18b20_config *cfg = dev->config;
    int ret;

    ret = w1_reset(cfg->w1_dev);
    if (ret < 0) {
        return ret;
    }

    ret = w1_write_byte(cfg->w1_dev, W1_CMD_MATCH_ROM);
    if (ret < 0) {
        return ret;
    }

    ret = w1_write_block(cfg->w1_dev, (uint8_t *)&cfg->w1_config.rom,
                        sizeof(struct w1_rom));
    if (ret < 0) {
        return ret;
    }

    ret = w1_write_byte(cfg->w1_dev, DS18B20_CMD_CONVERT_T);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

int ds18b20_read_temp(const struct device *dev, int16_t *temp)
{
    uint8_t buf[9];
    int ret;

    ret = ds18b20_read_scratchpad(dev, buf);
    if (ret < 0) {
        return ret;
    }

    *temp = (buf[1] << 8) | buf[0];
    return 0;
}

int ds18b20_set_resolution(const struct device *dev, uint8_t resolution)
{
    const struct ds18b20_config *cfg = dev->config;
    uint8_t config;

    switch (resolution) {
    case 9:
        config = DS18B20_RES_9_BIT;
        break;
    case 10:
        config = DS18B20_RES_10_BIT;
        break;
    case 11:
        config = DS18B20_RES_11_BIT;
        break;
    case 12:
        config = DS18B20_RES_12_BIT;
        break;
    default:
        return -EINVAL;
    }

    return ds18b20_write_scratchpad(dev, 0, 0, config);
}

int ds18b20_check_power_mode(const struct device *dev, bool *parasite)
{
    const struct ds18b20_config *cfg = dev->config;
    uint8_t value;
    int ret;

    ret = w1_reset(cfg->w1_dev);
    if (ret < 0) {
        return ret;
    }

    ret = w1_write_byte(cfg->w1_dev, W1_CMD_MATCH_ROM);
    if (ret < 0) {
        return ret;
    }

    ret = w1_write_block(cfg->w1_dev, (uint8_t *)&cfg->w1_config.rom,
                        sizeof(struct w1_rom));
    if (ret < 0) {
        return ret;
    }

    ret = w1_write_byte(cfg->w1_dev, DS18B20_CMD_READ_POWER_SUPPLY);
    if (ret < 0) {
        return ret;
    }

    ret = w1_read_byte(cfg->w1_dev, &value);
    if (ret < 0) {
        return ret;
    }

    *parasite = (value == 0);
    return 0;
}

static int ds18b20_sample_fetch(const struct device *dev,
                              enum sensor_channel chan)
{
    struct ds18b20_data *data = dev->data;
    const struct ds18b20_config *cfg = dev->config;
    int ret;
    int16_t temp;

    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
        return -ENOTSUP;
    }

    ret = ds18b20_trigger_conversion(dev);
    if (ret < 0) {
        return ret;
    }

    /* Wait for conversion to complete based on resolution */
    switch (cfg->resolution) {
    case 9:
        k_msleep(DS18B20_CONV_TIME_9_BIT);
        break;
    case 10:
        k_msleep(DS18B20_CONV_TIME_10_BIT);
        break;
    case 11:
        k_msleep(DS18B20_CONV_TIME_11_BIT);
        break;
    case 12:
        k_msleep(DS18B20_CONV_TIME_12_BIT);
        break;
    }

    ret = ds18b20_read_temp(dev, &temp);
    if (ret < 0) {
        return ret;
    }

    data->temperature = temp;
    data->timestamp = k_uptime_get();

    return 0;
}

static int ds18b20_channel_get(const struct device *dev,
                             enum sensor_channel chan,
                             struct sensor_value *val)
{
    struct ds18b20_data *data = dev->data;

    if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
        return -ENOTSUP;
    }

    /* Convert temperature to sensor_value format */
    val->val1 = data->temperature / 16;
    val->val2 = (data->temperature % 16) * 62500;

    return 0;
}

static int ds18b20_init(const struct device *dev)
{
    const struct ds18b20_config *cfg = dev->config;
    bool parasite;
    int ret;

    if (!device_is_ready(cfg->w1_dev)) {
        LOG_ERR("1-Wire bus not ready");
        return -ENODEV;
    }

    /* Check ROM family code */
    if (cfg->w1_config.rom.family != DS18B20_FAMILY_CODE) {
        LOG_ERR("Invalid family code: 0x%02x", cfg->w1_config.rom.family);
        return -EINVAL;
    }

    /* Set resolution */
    ret = ds18b20_set_resolution(dev, cfg->resolution);
    if (ret < 0) {
        LOG_ERR("Failed to set resolution");
        return ret;
    }

    /* Check power mode */
    ret = ds18b20_check_power_mode(dev, &parasite);
    if (ret < 0) {
        LOG_ERR("Failed to check power mode");
        return ret;
    }

    LOG_INF("DS18B20 initialized (parasite power: %s)",
            parasite ? "yes" : "no");

    return 0;
}

static const struct sensor_driver_api ds18b20_api = {
    .sample_fetch = ds18b20_sample_fetch,
    .channel_get = ds18b20_channel_get,
};

#define DS18B20_INIT(inst)                                                     \
    static struct ds18b20_data ds18b20_data_##inst;                           \
                                                                              \
    static const struct ds18b20_config ds18b20_config_##inst = {              \
        .w1_dev = DEVICE_DT_GET(DT_INST_BUS(inst)),                          \
        .w1_config.rom.family = DS18B20_FAMILY_CODE,                          \
        .w1_config.rom.serial = DT_INST_PROP(inst, serial_number),            \
        .resolution = DT_INST_PROP_OR(inst, resolution, 12),                  \
    };                                                                        \
                                                                              \
    DEVICE_DT_INST_DEFINE(inst,                                              \
                         ds18b20_init,                                        \
                         NULL,                                                \
                         &ds18b20_data_##inst,                               \
                         &ds18b20_config_##inst,                             \
                         POST_KERNEL,                                         \
                         CONFIG_SENSOR_INIT_PRIORITY,                         \
                         &ds18b20_api);

DT_INST_FOREACH_STATUS_OKAY(DS18B20_INIT)
