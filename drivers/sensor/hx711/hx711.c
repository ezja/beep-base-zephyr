/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT avia_hx711

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "hx711.h"

LOG_MODULE_REGISTER(hx711, CONFIG_SENSOR_LOG_LEVEL);

static int hx711_read_raw_value(const struct device *dev, int32_t *value)
{
    const struct hx711_config *config = dev->config;
    struct hx711_data *data = dev->data;
    int32_t raw = 0;
    int i;

    /* Wait for data to be ready */
    if (!gpio_pin_get_dt(&config->dout_gpio)) {
        /* Read 24 bits */
        for (i = 0; i < 24; i++) {
            gpio_pin_set_dt(&config->sck_gpio, 1);
            k_busy_wait(1);  /* T2 minimum */
            
            raw = (raw << 1);
            
            if (gpio_pin_get_dt(&config->dout_gpio)) {
                raw++;
            }
            
            gpio_pin_set_dt(&config->sck_gpio, 0);
            k_busy_wait(1);  /* T3 minimum */
        }

        /* Add the extra pulses for gain setting */
        for (i = 0; i < data->current_gain; i++) {
            gpio_pin_set_dt(&config->sck_gpio, 1);
            k_busy_wait(1);
            gpio_pin_set_dt(&config->sck_gpio, 0);
            k_busy_wait(1);
        }

        /* Convert to 32-bit signed value */
        if (raw & 0x800000) {
            raw |= 0xFF000000;
        }

        *value = raw;
        return 0;
    }

    return -EBUSY;
}

static int hx711_sample_fetch(const struct device *dev,
                            enum sensor_channel chan)
{
    struct hx711_data *data = dev->data;
    const struct hx711_config *config = dev->config;
    int32_t sum = 0;
    int32_t value;
    int ret;

    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_WEIGHT) {
        return -ENOTSUP;
    }

    k_mutex_lock(&data->lock, K_FOREVER);

    /* Take multiple samples and average */
    for (int i = 0; i < config->samples; i++) {
        ret = hx711_read_raw_value(dev, &value);
        if (ret < 0) {
            k_mutex_unlock(&data->lock);
            return ret;
        }
        sum += value;
    }

    data->raw_value = sum / config->samples;
    
    /* Apply offset and scale */
    int64_t processed = (int64_t)(data->raw_value - config->offset);
    processed *= config->scale;
    data->filtered_value = (int32_t)(processed >> 16);  /* Q16.16 to integer */

    k_mutex_unlock(&data->lock);
    return 0;
}

static int hx711_channel_get(const struct device *dev,
                           enum sensor_channel chan,
                           struct sensor_value *val)
{
    struct hx711_data *data = dev->data;

    if (chan != SENSOR_CHAN_WEIGHT) {
        return -ENOTSUP;
    }

    val->val1 = data->filtered_value;
    val->val2 = 0;

    return 0;
}

int hx711_set_gain(const struct device *dev, uint8_t gain)
{
    struct hx711_data *data = dev->data;

    if (gain != HX711_GAIN_128_CH_A &&
        gain != HX711_GAIN_32_CH_B &&
        gain != HX711_GAIN_64_CH_A) {
        return -EINVAL;
    }

    k_mutex_lock(&data->lock, K_FOREVER);
    data->current_gain = gain;
    k_mutex_unlock(&data->lock);

    return 0;
}

int hx711_set_offset(const struct device *dev, int32_t offset)
{
    const struct hx711_config *config = dev->config;
    struct hx711_data *data = dev->data;

    k_mutex_lock(&data->lock, K_FOREVER);
    ((struct hx711_config *)config)->offset = offset;
    k_mutex_unlock(&data->lock);

    return 0;
}

int hx711_set_scale(const struct device *dev, int32_t scale)
{
    const struct hx711_config *config = dev->config;
    struct hx711_data *data = dev->data;

    k_mutex_lock(&data->lock, K_FOREVER);
    ((struct hx711_config *)config)->scale = scale;
    k_mutex_unlock(&data->lock);

    return 0;
}

int hx711_power_down(const struct device *dev)
{
    const struct hx711_config *config = dev->config;

    gpio_pin_set_dt(&config->sck_gpio, 1);
    k_sleep(K_MSEC(1));  /* Hold SCK high for > 60µs */

    return 0;
}

int hx711_power_up(const struct device *dev)
{
    const struct hx711_config *config = dev->config;

    gpio_pin_set_dt(&config->sck_gpio, 0);
    k_sleep(K_MSEC(1));  /* Wait for power-up */

    return 0;
}

bool hx711_is_ready(const struct device *dev)
{
    const struct hx711_config *config = dev->config;
    return !gpio_pin_get_dt(&config->dout_gpio);
}

static int hx711_init(const struct device *dev)
{
    const struct hx711_config *config = dev->config;
    struct hx711_data *data = dev->data;
    int ret;

    /* Configure GPIO pins */
    if (!device_is_ready(config->sck_gpio.port)) {
        LOG_ERR("SCK GPIO device not ready");
        return -ENODEV;
    }

    if (!device_is_ready(config->dout_gpio.port)) {
        LOG_ERR("DOUT GPIO device not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&config->sck_gpio, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure SCK pin");
        return ret;
    }

    ret = gpio_pin_configure_dt(&config->dout_gpio, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Failed to configure DOUT pin");
        return ret;
    }

    /* Initialize mutex */
    k_mutex_init(&data->lock);

    /* Set initial gain */
    data->current_gain = config->gain;

    /* Power cycle the device */
    hx711_power_down(dev);
    k_sleep(K_MSEC(1));
    hx711_power_up(dev);

    LOG_INF("HX711 initialized");
    return 0;
}

static const struct sensor_driver_api hx711_api = {
    .sample_fetch = hx711_sample_fetch,
    .channel_get = hx711_channel_get,
};

#define HX711_INIT(inst)                                                      \
    static struct hx711_data hx711_data_##inst;                              \
                                                                             \
    static const struct hx711_config hx711_config_##inst = {                 \
        .sck_gpio = GPIO_DT_SPEC_INST_GET(inst, sck_gpios),                 \
        .dout_gpio = GPIO_DT_SPEC_INST_GET(inst, dout_gpios),               \
        .gain = DT_INST_PROP_OR(inst, gain, HX711_GAIN_128_CH_A),          \
        .samples = DT_INST_PROP_OR(inst, samples, 1),                       \
        .offset = DT_INST_PROP_OR(inst, offset, 0),                         \
        .scale = DT_INST_PROP_OR(inst, scale, 1 << 16),                    \
    };                                                                       \
                                                                            \
    DEVICE_DT_INST_DEFINE(inst,                                             \
                         hx711_init,                                         \
                         NULL,                                               \
                         &hx711_data_##inst,                                \
                         &hx711_config_##inst,                              \
                         POST_KERNEL,                                        \
                         CONFIG_SENSOR_INIT_PRIORITY,                        \
                         &hx711_api);

DT_INST_FOREACH_STATUS_OKAY(HX711_INIT)
