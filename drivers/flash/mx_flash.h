/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_MX_FLASH_H_
#define ZEPHYR_DRIVERS_FLASH_MX_FLASH_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/spi.h>

/* MX25 Commands */
#define MX25_CMD_WRITE_ENABLE      0x06
#define MX25_CMD_WRITE_DISABLE     0x04
#define MX25_CMD_READ_STATUS       0x05
#define MX25_CMD_WRITE_STATUS      0x01
#define MX25_CMD_READ_DATA         0x03
#define MX25_CMD_FAST_READ         0x0B
#define MX25_CMD_PAGE_PROGRAM      0x02
#define MX25_CMD_SECTOR_ERASE      0x20
#define MX25_CMD_BLOCK_ERASE_32K   0x52
#define MX25_CMD_BLOCK_ERASE_64K   0xD8
#define MX25_CMD_CHIP_ERASE        0xC7
#define MX25_CMD_POWER_DOWN        0xB9
#define MX25_CMD_RELEASE_POWER_DOWN 0xAB
#define MX25_CMD_READ_ID           0x9F

/* Status Register bits */
#define MX25_STATUS_WIP_BIT        0  /* Write in progress */
#define MX25_STATUS_WEL_BIT        1  /* Write enable latch */
#define MX25_STATUS_BP0_BIT        2  /* Block protect 0 */
#define MX25_STATUS_BP1_BIT        3  /* Block protect 1 */
#define MX25_STATUS_BP2_BIT        4  /* Block protect 2 */
#define MX25_STATUS_BP3_BIT        5  /* Block protect 3 */
#define MX25_STATUS_QE_BIT         6  /* Quad enable */
#define MX25_STATUS_SRWD_BIT       7  /* Status register write protect */

/* Device parameters */
#define MX25_PAGE_SIZE            256
#define MX25_SECTOR_SIZE          4096
#define MX25_BLOCK_SIZE_32K       32768
#define MX25_BLOCK_SIZE_64K       65536

/* Configuration structure */
struct mx_flash_config {
    struct spi_dt_spec spi;
    struct gpio_dt_spec reset_gpio;
    struct gpio_dt_spec wp_gpio;
    struct gpio_dt_spec hold_gpio;
    uint32_t size;
    uint32_t sector_size;
    uint32_t block_size;
    uint32_t page_size;
};

/* Runtime data structure */
struct mx_flash_data {
    struct k_sem lock;
    uint8_t *write_buf;
    size_t write_buf_size;
    bool write_protection;
};

/**
 * @brief Initialize MX flash device
 *
 * @param dev Pointer to device structure
 * @return 0 on success, negative errno code on failure
 */
int mx_flash_init(const struct device *dev);

/**
 * @brief Read data from flash
 *
 * @param dev Pointer to device structure
 * @param offset Offset to read from
 * @param data Buffer to store read data
 * @param len Number of bytes to read
 * @return 0 on success, negative errno code on failure
 */
int mx_flash_read(const struct device *dev, off_t offset, void *data, size_t len);

/**
 * @brief Write data to flash
 *
 * @param dev Pointer to device structure
 * @param offset Offset to write to
 * @param data Data to write
 * @param len Number of bytes to write
 * @return 0 on success, negative errno code on failure
 */
int mx_flash_write(const struct device *dev, off_t offset, const void *data, size_t len);

/**
 * @brief Erase flash sector
 *
 * @param dev Pointer to device structure
 * @param offset Offset of sector to erase
 * @return 0 on success, negative errno code on failure
 */
int mx_flash_erase(const struct device *dev, off_t offset);

/**
 * @brief Get flash device size
 *
 * @param dev Pointer to device structure
 * @return Size in bytes
 */
size_t mx_flash_size(const struct device *dev);

/**
 * @brief Enable or disable write protection
 *
 * @param dev Pointer to device structure
 * @param enable true to enable, false to disable
 * @return 0 on success, negative errno code on failure
 */
int mx_flash_write_protection_set(const struct device *dev, bool enable);

/**
 * @brief Check if write protection is enabled
 *
 * @param dev Pointer to device structure
 * @return true if enabled, false if disabled
 */
bool mx_flash_write_protection_get(const struct device *dev);

/**
 * @brief Power down the device
 *
 * @param dev Pointer to device structure
 * @return 0 on success, negative errno code on failure
 */
int mx_flash_power_down(const struct device *dev);

/**
 * @brief Release device from power down
 *
 * @param dev Pointer to device structure
 * @return 0 on success, negative errno code on failure
 */
