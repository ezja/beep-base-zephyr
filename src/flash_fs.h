/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FLASH_FS_H
#define FLASH_FS_H

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include "beep_types.h"

/* Flash filesystem mount point */
#define FLASH_FS_MOUNT_POINT "/mx25"

/* Maximum filename length */
#define FLASH_FS_MAX_FILENAME 32

/* Flash partition definitions */
#define FLASH_PARTITION_LABEL "mx25_storage"

/* Error codes */
#define FLASH_FS_SUCCESS      0
#define FLASH_FS_ERROR       -1
#define FLASH_FS_FULL       -2
#define FLASH_FS_NOT_FOUND  -3
#define FLASH_FS_INVALID    -4

/**
 * @brief Initialize flash filesystem
 *
 * @return 0 on success, negative errno code on failure
 */
int flash_fs_init(void);

/**
 * @brief Store measurement data in flash
 *
 * @param result Pointer to measurement result
 * @return 0 on success, negative errno code on failure
 */
int flash_fs_store_measurement(const MEASUREMENT_RESULT_s *result);

/**
 * @brief Read measurement data from flash
 *
 * @param index Measurement index
 * @param result Pointer to store measurement result
 * @return 0 on success, negative errno code on failure
 */
int flash_fs_read_measurement(uint32_t index, MEASUREMENT_RESULT_s *result);

/**
 * @brief Get number of stored measurements
 *
 * @param count Pointer to store measurement count
 * @return 0 on success, negative errno code on failure
 */
int flash_fs_get_measurement_count(uint32_t *count);

/**
 * @brief Delete measurement data
 *
 * @param index Measurement index
 * @return 0 on success, negative errno code on failure
 */
int flash_fs_delete_measurement(uint32_t index);

/**
 * @brief Clear all measurement data
 *
 * @return 0 on success, negative errno code on failure
 */
int flash_fs_clear_measurements(void);

/**
 * @brief Store configuration data in flash
 *
 * @param data Pointer to configuration data
 * @param size Size of configuration data
 * @return 0 on success, negative errno code on failure
 */
int flash_fs_store_config(const void *data, size_t size);

/**
 * @brief Read configuration data from flash
 *
 * @param data Pointer to store configuration data
 * @param size Size of configuration data buffer
 * @return 0 on success, negative errno code on failure
 */
int flash_fs_read_config(void *data, size_t size);

/**
 * @brief Get filesystem statistics
 *
 * @param total_bytes Pointer to store total space in bytes
 * @param used_bytes Pointer to store used space in bytes
 * @return 0 on success, negative errno code on failure
 */
int flash_fs_get_stats(size_t *total_bytes, size_t *used_bytes);

#endif /* FLASH_FS_H */
