/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_HX711_H_
#define ZEPHYR_DRIVERS_SENSOR_HX711_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

/* HX711 Gain settings */
#define HX711_GAIN_128_CH_A  1  /* Channel A with gain 128 */
#define HX711_GAIN_32_CH_B   2  /* Channel B with gain 32 */
#define HX711_GAIN_64_CH_A   3  /* Channel A with gain 64 */

/* Timing constants (in microseconds) */
#define HX711_T1_MIN  100    /* Power down recovery time */
#define HX711_T2_MIN  0.1    /* PD_SCK high time */
#define HX711_T3_MIN  0.1    /* PD_SCK low time */
#define HX711_T4_MIN  0.1    /* Data ready after PD_SCK rising edge */

/* Configuration structure */
struct hx711_config {
    struct gpio_dt_spec sck_gpio;
    struct gpio_dt_spec dout_gpio;
    uint8_t gain;           /* Default gain setting */
    uint16_t samples;       /* Number of samples to average */
    int32_t offset;        /* Tare offset */
    int32_t scale;         /* Scaling factor (fixed point Q16.16) */
};

/* Data structure */
struct hx711_data {
    int32_t raw_value;     /* Raw ADC value */
    int32_t filtered_value; /* Filtered value */
    uint8_t current_gain;  /* Current gain setting */
    struct k_mutex lock;   /* Access lock */
};

/**
 * @brief Set the gain for the next reading
 *
 * @param dev Pointer to the device structure
 * @param gain Gain setting (HX711_GAIN_128_CH_A, HX711_GAIN_32_CH_B, or HX711_GAIN_64_CH_A)
 * @return 0 if successful, negative errno code on failure
 */
int hx711_set_gain(const struct device *dev, uint8_t gain);

/**
 * @brief Set the tare offset
 *
 * @param dev Pointer to the device structure
 * @param offset Tare offset value
 * @return 0 if successful, negative errno code on failure
 */
int hx711_set_offset(const struct device *dev, int32_t offset);

/**
 * @brief Set the scale factor
 *
 * @param dev Pointer to the device structure
 * @param scale Scale factor in Q16.16 fixed point format
 * @return 0 if successful, negative errno code on failure
 */
int hx711_set_scale(const struct device *dev, int32_t scale);

/**
 * @brief Power down the device
 *
 * @param dev Pointer to the device structure
 * @return 0 if successful, negative errno code on failure
 */
int hx711_power_down(const struct device *dev);

/**
 * @brief Power up the device
 *
 * @param dev Pointer to the device structure
 * @return 0 if successful, negative errno code on failure
 */
int hx711_power_up(const struct device *dev);

/**
 * @brief Check if new data is ready
 *
 * @param dev Pointer to the device structure
 * @return true if data is ready, false otherwise
 */
bool hx711_is_ready(const struct device *dev);

/**
 * @brief Read raw value from the device
 *
 * @param dev Pointer to the device structure
 * @param value Pointer to store the raw value
 * @return 0 if successful, negative errno code on failure
 */
int hx711_read_raw(const struct device *dev, int32_t *value);

/**
 * @brief Read averaged and scaled value from the device
 *
 * @param dev Pointer to the device structure
 * @param value Pointer to store the processed value
 * @return 0 if successful, negative errno code on failure
 */
int hx711_read_processed(const struct device *dev, int32_t *value);

#endif /* ZEPHYR_DRIVERS_SENSOR_HX711_H_ */
