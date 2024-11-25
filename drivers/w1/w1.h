/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_W1_W1_H_
#define ZEPHYR_DRIVERS_W1_W1_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/* 1-Wire ROM commands */
#define W1_CMD_READ_ROM          0x33
#define W1_CMD_MATCH_ROM         0x55
#define W1_CMD_SKIP_ROM          0xCC
#define W1_CMD_SEARCH_ROM        0xF0
#define W1_CMD_ALARM_SEARCH      0xEC

/* ROM size in bytes */
#define W1_ROM_SIZE             8

/* CRC8 polynomial used by 1-Wire devices */
#define W1_CRC8_POLY           0x8C

struct w1_rom {
    uint8_t family;
    uint8_t serial[6];
    uint8_t crc;
};

struct w1_slave_config {
    struct w1_rom rom;
};

struct w1_config {
    struct gpio_dt_spec gpio;
    uint32_t overdrive_speed:1;
};

struct w1_data {
    struct k_mutex lock;
};

/**
 * @brief 1-Wire bus API
 */
struct w1_driver_api {
    /**
     * @brief Reset the 1-Wire bus
     *
     * @param dev Pointer to the device structure for the driver instance
     * @return 0 if successful, negative errno code on failure
     */
    int (*reset)(const struct device *dev);

    /**
     * @brief Write a byte to the 1-Wire bus
     *
     * @param dev Pointer to the device structure for the driver instance
     * @param byte Byte to write
     * @return 0 if successful, negative errno code on failure
     */
    int (*write_byte)(const struct device *dev, uint8_t byte);

    /**
     * @brief Read a byte from the 1-Wire bus
     *
     * @param dev Pointer to the device structure for the driver instance
     * @param byte Pointer to store the read byte
     * @return 0 if successful, negative errno code on failure
     */
    int (*read_byte)(const struct device *dev, uint8_t *byte);

    /**
     * @brief Write multiple bytes to the 1-Wire bus
     *
     * @param dev Pointer to the device structure for the driver instance
     * @param buf Buffer containing bytes to write
     * @param len Number of bytes to write
     * @return 0 if successful, negative errno code on failure
     */
    int (*write_block)(const struct device *dev, const uint8_t *buf, size_t len);

    /**
     * @brief Read multiple bytes from the 1-Wire bus
     *
     * @param dev Pointer to the device structure for the driver instance
     * @param buf Buffer to store read bytes
     * @param len Number of bytes to read
     * @return 0 if successful, negative errno code on failure
     */
    int (*read_block)(const struct device *dev, uint8_t *buf, size_t len);
};

/**
 * @brief Reset the 1-Wire bus
 *
 * @param dev Pointer to the device structure for the driver instance
 * @return 0 if successful, negative errno code on failure
 */
static inline int w1_reset(const struct device *dev)
{
    const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;
    return api->reset(dev);
}

/**
 * @brief Write a byte to the 1-Wire bus
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param byte Byte to write
 * @return 0 if successful, negative errno code on failure
 */
static inline int w1_write_byte(const struct device *dev, uint8_t byte)
{
    const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;
    return api->write_byte(dev, byte);
}

/**
 * @brief Read a byte from the 1-Wire bus
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param byte Pointer to store the read byte
 * @return 0 if successful, negative errno code on failure
 */
static inline int w1_read_byte(const struct device *dev, uint8_t *byte)
{
    const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;
    return api->read_byte(dev, byte);
}

/**
 * @brief Write multiple bytes to the 1-Wire bus
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param buf Buffer containing bytes to write
 * @param len Number of bytes to write
 * @return 0 if successful, negative errno code on failure
 */
static inline int w1_write_block(const struct device *dev,
                               const uint8_t *buf, size_t len)
{
    const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;
    return api->write_block(dev, buf, len);
}

/**
 * @brief Read multiple bytes from the 1-Wire bus
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param buf Buffer to store read bytes
 * @param len Number of bytes to read
 * @return 0 if successful, negative errno code on failure
 */
static inline int w1_read_block(const struct device *dev,
                              uint8_t *buf, size_t len)
{
    const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;
    return api->read_block(dev, buf, len);
}

/**
 * @brief Calculate CRC8 for 1-Wire devices
 *
 * @param buf Buffer containing data
 * @param len Length of data
 * @return CRC8 value
 */
uint8_t w1_crc8(const uint8_t *buf, size_t len);

#endif /* ZEPHYR_DRIVERS_W1_W1_H_ */
