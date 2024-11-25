/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_ds3231

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "ds3231.h"

LOG_MODULE_REGISTER(ds3231, CONFIG_RTC_LOG_LEVEL);

/* Helper functions */
static uint8_t bin2bcd(uint8_t value)
{
    return ((value / 10) << 4) | (value % 10);
}

static uint8_t bcd2bin(uint8_t value)
{
    return ((value >> 4) * 10) + (value & 0x0F);
}

static int ds3231_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
    const struct ds3231_config *config = dev->config;
    return i2c_write_read_dt(&config->i2c, &reg, 1, val, 1);
}

static int ds3231_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
    const struct ds3231_config *config = dev->config;
    uint8_t buf[2] = {reg, val};
    return i2c_write_dt(&config->i2c, buf, sizeof(buf));
}

static int ds3231_reg_update(const struct device *dev, uint8_t reg,
                           uint8_t mask, uint8_t val)
{
    uint8_t old_val;
    int ret;

    ret = ds3231_reg_read(dev, reg, &old_val);
    if (ret < 0) {
        return ret;
    }

    return ds3231_reg_write(dev, reg, (old_val & ~mask) | (val & mask));
}

/* Interrupt handler */
static void ds3231_alarm_handler(const struct device *dev,
                               struct gpio_callback *cb, uint32_t pins)
{
    struct ds3231_data *data = CONTAINER_OF(cb, struct ds3231_data, gpio_cb);
    k_work_submit(&data->alarm_work);
}

static void ds3231_work_handler(struct k_work *work)
{
    struct ds3231_data *data = CONTAINER_OF(work, struct ds3231_data, alarm_work);
    uint8_t status;

    /* Read status register to determine which alarm fired */
    if (ds3231_reg_read(data->dev, DS3231_REG_STATUS, &status) == 0) {
        if (status & DS3231_STAT_A1F && data->alarm_cb) {
            data->alarm_cb(1);
        }
        if (status & DS3231_STAT_A2F && data->alarm_cb) {
            data->alarm_cb(2);
        }
        /* Clear alarm flags */
        ds3231_reg_write(data->dev, DS3231_REG_STATUS,
                        status & ~(DS3231_STAT_A1F | DS3231_STAT_A2F));
    }
}

/* API Implementation */
int ds3231_set_time(const struct device *dev, const struct tm *tm)
{
    struct ds3231_data *data = dev->data;
    uint8_t buf[8] = {DS3231_REG_SECONDS};
    int ret;

    if (!tm) {
        return -EINVAL;
    }

    k_mutex_lock(&data->lock, K_FOREVER);

    /* Convert time to BCD format */
    buf[1] = bin2bcd(tm->tm_sec);
    buf[2] = bin2bcd(tm->tm_min);
    buf[3] = bin2bcd(tm->tm_hour);
    buf[4] = bin2bcd(tm->tm_wday + 1);
    buf[5] = bin2bcd(tm->tm_mday);
    buf[6] = bin2bcd(tm->tm_mon + 1);
    buf[7] = bin2bcd(tm->tm_year % 100);

    /* Write all time registers in one transaction */
    ret = i2c_write_dt(&((const struct ds3231_config *)dev->config)->i2c,
                      buf, sizeof(buf));

    k_mutex_unlock(&data->lock);
    return ret;
}

int ds3231_get_time(const struct device *dev, struct tm *tm)
{
    struct ds3231_data *data = dev->data;
    uint8_t buf[7];
    int ret;

    if (!tm) {
        return -EINVAL;
    }

    k_mutex_lock(&data->lock, K_FOREVER);

    /* Read all time registers in one transaction */
    ret = i2c_write_read_dt(&((const struct ds3231_config *)dev->config)->i2c,
                           (uint8_t[]){DS3231_REG_SECONDS}, 1, buf, sizeof(buf));
    if (ret == 0) {
        /* Convert from BCD format */
        tm->tm_sec = bcd2bin(buf[0]);
        tm->tm_min = bcd2bin(buf[1]);
        tm->tm_hour = bcd2bin(buf[2]);
        tm->tm_wday = bcd2bin(buf[3]) - 1;
        tm->tm_mday = bcd2bin(buf[4]);
        tm->tm_mon = bcd2bin(buf[5]) - 1;
        tm->tm_year = bcd2bin(buf[6]) + 100; /* Assume 20xx */
    }

    k_mutex_unlock(&data->lock);
    return ret;
}

int ds3231_set_alarm(const struct device *dev, uint8_t alarm_num,
                    const struct tm *tm)
{
    struct ds3231_data *data = dev->data;
    uint8_t buf[5];
    int ret;

    if (!tm || (alarm_num != 1 && alarm_num != 2)) {
        return -EINVAL;
    }

    k_mutex_lock(&data->lock, K_FOREVER);

    if (alarm_num == 1) {
        buf[0] = DS3231_REG_ALARM1_SEC;
        buf[1] = bin2bcd(tm->tm_sec);
        buf[2] = bin2bcd(tm->tm_min);
        buf[3] = bin2bcd(tm->tm_hour);
        buf[4] = bin2bcd(tm->tm_mday);
        ret = i2c_write_dt(&((const struct ds3231_config *)dev->config)->i2c,
                          buf, 5);
    } else {
        buf[0] = DS3231_REG_ALARM2_MIN;
        buf[1] = bin2bcd(tm->tm_min);
        buf[2] = bin2bcd(tm->tm_hour);
        buf[3] = bin2bcd(tm->tm_mday);
        ret = i2c_write_dt(&((const struct ds3231_config *)dev->config)->i2c,
                          buf, 4);
    }

    k_mutex_unlock(&data->lock);
    return ret;
}

int ds3231_get_alarm(const struct device *dev, uint8_t alarm_num,
                    struct tm *tm)
{
    struct ds3231_data *data = dev->data;
    uint8_t buf[4];
    int ret;

    if (!tm || (alarm_num != 1 && alarm_num != 2)) {
        return -EINVAL;
    }

    k_mutex_lock(&data->lock, K_FOREVER);

    if (alarm_num == 1) {
        ret = i2c_write_read_dt(&((const struct ds3231_config *)dev->config)->i2c,
                               (uint8_t[]){DS3231_REG_ALARM1_SEC}, 1, buf, 4);
        if (ret == 0) {
            tm->tm_sec = bcd2bin(buf[0]);
            tm->tm_min = bcd2bin(buf[1]);
            tm->tm_hour = bcd2bin(buf[2]);
            tm->tm_mday = bcd2bin(buf[3]);
        }
    } else {
        ret = i2c_write_read_dt(&((const struct ds3231_config *)dev->config)->i2c,
                               (uint8_t[]){DS3231_REG_ALARM2_MIN}, 1, buf, 3);
        if (ret == 0) {
            tm->tm_sec = 0;
            tm->tm_min = bcd2bin(buf[0]);
            tm->tm_hour = bcd2bin(buf[1]);
            tm->tm_mday = bcd2bin(buf[2]);
        }
    }

    k_mutex_unlock(&data->lock);
    return ret;
}

int ds3231_enable_alarm(const struct device *dev, uint8_t alarm_num,
                       bool enable)
{
    struct ds3231_data *data = dev->data;
    uint8_t mask;
    int ret;

    if (alarm_num != 1 && alarm_num != 2) {
        return -EINVAL;
    }

    k_mutex_lock(&data->lock, K_FOREVER);

    mask = (alarm_num == 1) ? DS3231_CTRL_A1IE : DS3231_CTRL_A2IE;
    ret = ds3231_reg_update(dev, DS3231_REG_CONTROL, mask,
                           enable ? mask : 0);

    k_mutex_unlock(&data->lock);
    return ret;
}

int ds3231_clear_alarm(const struct device *dev, uint8_t alarm_num)
{
    struct ds3231_data *data = dev->data;
    uint8_t mask;
    int ret;

    if (alarm_num != 1 && alarm_num != 2) {
        return -EINVAL;
    }

    k_mutex_lock(&data->lock, K_FOREVER);

    mask = (alarm_num == 1) ? DS3231_STAT_A1F : DS3231_STAT_A2F;
    ret = ds3231_reg_update(dev, DS3231_REG_STATUS, mask, 0);

    k_mutex_unlock(&data->lock);
    return ret;
}

int ds3231_get_temperature(const struct device *dev, int16_t *temp)
{
    struct ds3231_data *data = dev->data;
    uint8_t buf[2];
    int ret;

    if (!temp) {
        return -EINVAL;
    }

    k_mutex_lock(&data->lock, K_FOREVER);

    ret = i2c_write_read_dt(&((const struct ds3231_config *)dev->config)->i2c,
                           (uint8_t[]){DS3231_REG_TEMP_MSB}, 1, buf, 2);
    if (ret == 0) {
        *temp = (buf[0] << 2) | (buf[1] >> 6);
        if (buf[0] & BIT(7)) {
            *temp |= 0xFC00;
        }
    }

    k_mutex_unlock(&data->lock);
    return ret;
}

int ds3231_set_alarm_callback(const struct device *dev,
                            ds3231_alarm_callback_t callback)
{
    struct ds3231_data *data = dev->data;

    k_mutex_lock(&data->lock, K_FOREVER);
    data->alarm_cb = callback;
    k_mutex_unlock(&data->lock);

    return 0;
}

/* Device initialization */
static int ds3231_init(const struct device *dev)
{
    const struct ds3231_config *config = dev->config;
    struct ds3231_data *data = dev->data;
    int ret;

    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    /* Initialize mutex */
    k_mutex_init(&data->lock);

    /* Initialize work queue item */
    k_work_init(&data->alarm_work, ds3231_work_handler);

    /* Store device reference */
    data->dev = dev;

    /* Configure interrupt if available */
    if (config->int_gpio.port != NULL) {
        if (!device_is_ready(config->int_gpio.port)) {
            LOG_ERR("Interrupt GPIO device not ready");
            return -ENODEV;
        }

        ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
        if (ret < 0) {
            LOG_ERR("Failed to configure interrupt pin");
            return ret;
        }

        gpio_init_callback(&data->gpio_cb, ds3231_alarm_handler,
                          BIT(config->int_gpio.pin));

        ret = gpio_add_callback(config->int_gpio.port, &data->gpio_cb);
        if (ret < 0) {
            LOG_ERR("Failed to add interrupt callback");
            return ret;
        }

        ret = gpio_pin_interrupt_configure_dt(&config->int_gpio,
                                            GPIO_INT_EDGE_FALLING);
        if (ret < 0) {
            LOG_ERR("Failed to configure interrupt");
            return ret;
        }
    }

    /* Configure RTC */
    ret = ds3231_reg_write(dev, DS3231_REG_CONTROL,
                          DS3231_CTRL_INTCN | DS3231_CTRL_RS2);
    if (ret < 0) {
        LOG_ERR("Failed to configure RTC");
        return ret;
    }

    /* Clear any pending alarms */
    ret = ds3231_reg_write(dev, DS3231_REG_STATUS, 0);
    if (ret < 0) {
        LOG_ERR("Failed to clear status");
        return ret;
    }

    LOG_INF("DS3231 initialized");
    return 0;
}

#define DS3231_INIT(inst)                                                      \
    static struct ds3231_data ds3231_data_##inst;                             \
                                                                              \
    static const struct ds3231_config ds3231_config_##inst = {                \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                   \
        .int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),         \
    };                                                                        \
                                                                              \
    DEVICE_DT_INST_DEFINE(inst,                                              \
                         ds3231_init,                                         \
                         NULL,                                                \
                         &ds3231_data_##inst,                                \
                         &ds3231_config_##inst,                              \
                         POST_KERNEL,                                         \
                         CONFIG_RTC_INIT_PRIORITY,                           \
                         NULL);

DT_INST_FOREACH_STATUS_OKAY(DS3231_INIT)
