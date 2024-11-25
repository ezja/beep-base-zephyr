/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_DS18B20_H_
#define ZEPHYR_DRIVERS_SENSOR_DS18B20_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/w1.h>

/* DS18B20 ROM Commands */
#define DS18B20_FAMILY_CODE           0x28
#define DS18B20_CMD_CONVERT_T         0x44
#define DS18B20_CMD_WRITE_SCRATCHPAD  0x4E
#define DS18B20_CMD_READ_SCRATCHPAD   0xBE
#define DS18B20_CMD_COPY_SCRATCHPAD   0x48
#define DS18B20_CMD_RECALL_E2         0xB8
#define DS18B20_CMD_READ_POWER_SUPPLY 0xB4

/* Configuration Register */
#define DS18B20_RES_9_BIT            0x1F
#define DS18B20_RES_10_BIT           0x3F
#define DS18B20_RES_11_BIT           0x5F
#define DS18B20_RES_12_BIT           0x7F

/* Conversion times in milliseconds */
#define DS18B20_CONV_TIME_9_BIT      94
#define DS18B20_CONV_TIME_10_BIT     188
#define DS18B20_CONV_TIME_11_BIT     375
#define DS18B20_CONV_TIME_12_BIT     750

struct ds18b20_config {
    const struct device *w1_dev;
    struct w1_slave_config w1_config;
    uint8_t resolution;
};

struct ds18b20_data {
    int16_t temperature;
    uint64_t timestamp;
};

/**
 * @brief Start temperature conversion
 *
 * @param dev Pointer to the device structure
 * @return 0 if successful, negative errno code on failure
 */
int ds18b20_trigger_conversion(const struct device *dev);

/**
 * @brief Read temperature from sensor
 *
 * @param dev Pointer to the device structure
 * @param temp Pointer to store temperature value (in 1/16th degrees Celsius)
 * @return 0 if successful, negative errno code on failure
 */
int ds18b20_read_temp(const struct device *dev, int16_t *temp);

/**
 * @brief Configure sensor resolution
 *
 * @param dev Pointer to the device structure
 * @param resolution Resolution setting (9-12 bits)
 * @return 0 if successful, negative errno code on failure
 */
int ds18b20_set_resolution(const struct device *dev, uint8_t resolution);

/**
 * @brief Check if sensor is powered by external supply
 *
 * @param dev Pointer to the device structure
 * @param parasite true if parasite powered, false if externally powered
 * @return 0 if successful, negative errno code on failure
 */
int ds18b20_check_power_mode(const struct device *dev, bool *parasite);

#endif /* ZEPHYR_DRIVERS_SENSOR_DS18B20_H_ */
