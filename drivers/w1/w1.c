/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "w1.h"

LOG_MODULE_REGISTER(w1, CONFIG_W1_LOG_LEVEL);

/* 1-Wire timing in microseconds */
#define W1_TIME_A           6    /* Write 1 low time */
#define W1_TIME_B           64   /* Write 1 high time */
#define W1_TIME_C           60   /* Write 0 low time */
#define W1_TIME_D           10   /* Write 0 high time */
#define W1_TIME_E           9    /* Read low time */
#define W1_TIME_F           55   /* Read sample time */
#define W1_TIME_G           0    /* Read high time */
#define W1_TIME_H           480  /* Reset low time */
#define W1_TIME_I           70   /* Reset response time */
#define W1_TIME_J           410  /* Reset high time */

static int w1_reset_bus(const struct device *dev)
{
    const struct w1_config *config = dev->config;
    int presence = 0;

    /* Pull bus low for reset */
    gpio_pin_set_dt(&config->gpio, 0);
    k_busy_wait(W1_TIME_H);
    gpio_pin_set_dt(&config->gpio, 1);

    /* Wait for presence pulse */
    k_busy_wait(W1_TIME_I);
    presence = gpio_pin_get_dt(&config->gpio);

    /* Complete reset cycle */
    k_busy_wait(W1_TIME_J);

    return presence ? -EIO : 0;
}

static int w1_write_bit(const struct device *dev, uint8_t bit)
{
    const struct w1_config *config = dev->config;

    if (bit) {
        /* Write 1 */
        gpio_pin_set_dt(&config->gpio, 0);
        k_busy_wait(W1_TIME_A);
        gpio_pin_set_dt(&config->gpio, 1);
        k_busy_wait(W1_TIME_B);
    } else {
        /* Write 0 */
        gpio_pin_set_dt(&config->gpio, 0);
        k_busy_wait(W1_TIME_C);
        gpio_pin_set_dt(&config->gpio, 1);
        k_busy_wait(W1_TIME_D);
    }

    return 0;
}

static int w1_read_bit(const struct device *dev)
{
    const struct w1_config *config = dev->config;
    int bit;

    /* Generate read time slot */
    gpio_pin_set_dt(&config->gpio, 0);
    k_busy_wait(W1_TIME_E);
    gpio_pin_set_dt(&config->gpio, 1);

    /* Sample the bit */
    k_busy_wait(W1_TIME_F);
    bit = gpio_pin_get_dt(&config->gpio);

    /* Complete read time slot */
    k_busy_wait(W1_TIME_G);

    return bit;
}

static int w1_write_byte(const struct device *dev, uint8_t byte)
{
    int ret;

    for (int i = 0; i < 8; i++) {
        ret = w1_write_bit(dev, byte & 0x01);
        if (ret < 0) {
            return ret;
        }
        byte >>= 1;
    }

    return 0;
}

static int w1_read_byte(const struct device *dev, uint8_t *byte)
{
    uint8_t value = 0;
    int bit;

    for (int i = 0; i < 8; i++) {
        bit = w1_read_bit(dev);
        if (bit < 0) {
            return bit;
        }
        value |= (bit << i);
    }

    *byte = value;
    return 0;
}

static int w1_write_block(const struct device *dev,
                         const uint8_t *buf, size_t len)
{
    int ret;

    for (size_t i = 0; i < len; i++) {
        ret = w1_write_byte(dev, buf[i]);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

static int w1_read_block(const struct device *dev,
                        uint8_t *buf, size_t len)
{
    int ret;

    for (size_t i = 0; i < len; i++) {
        ret = w1_read_byte(dev, &buf[i]);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

uint8_t w1_crc8(const uint8_t *buf, size_t len)
{
    uint8_t crc = 0;

    while (len--) {
        uint8_t inbyte = *buf++;
        for (int i = 8; i; i--) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= W1_CRC8_POLY;
            }
            inbyte >>= 1;
        }
    }

    return crc;
}

static int w1_init(const struct device *dev)
{
    const struct w1_config *config = dev->config;
    struct w1_data *data = dev->data;
    int ret;

    if (!device_is_ready(config->gpio.port)) {
        LOG_ERR("GPIO device not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&config->gpio, GPIO_OUTPUT | GPIO_OPEN_DRAIN);
    if (ret < 0) {
        LOG_ERR("Failed to configure GPIO pin");
        return ret;
    }

    gpio_pin_set_dt(&config->gpio, 1);
    k_mutex_init(&data->lock);

    return 0;
}

static const struct w1_driver_api w1_api = {
    .reset = w1_reset_bus,
    .write_byte = w1_write_byte,
    .read_byte = w1_read_byte,
    .write_block = w1_write_block,
    .read_block = w1_read_block,
};

#define W1_INIT(inst)                                                          \
    static struct w1_data w1_data_##inst;                                     \
                                                                              \
    static const struct w1_config w1_config_##inst = {                        \
        .gpio = GPIO_DT_SPEC_INST_GET(inst, gpios),                          \
        .overdrive_speed = DT_INST_PROP(inst, overdrive_speed),              \
    };                                                                        \
                                                                              \
    DEVICE_DT_INST_DEFINE(inst,                                              \
                         w1_init,                                             \
                         NULL,                                                \
                         &w1_data_##inst,                                     \
                         &w1_config_##inst,                                   \
                         POST_KERNEL,                                         \
                         CONFIG_W1_INIT_PRIORITY,                             \
                         &w1_api);

DT_INST_FOREACH_STATUS_OKAY(W1_INIT)
