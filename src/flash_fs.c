/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include "flash_fs.h"

LOG_MODULE_REGISTER(flash_fs, CONFIG_APP_LOG_LEVEL);

/* LittleFS configuration */
#define PARTITION_NODE DT_NODELABEL(mx25_flash)

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t fs_mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &storage,
    .storage_dev = (void *)PARTITION_NODE,
    .mnt_point = FLASH_FS_MOUNT_POINT,
};

/* Mutex for filesystem access */
K_MUTEX_DEFINE(fs_mutex);

/* Internal functions */
static int ensure_directory(const char *path)
{
    struct fs_dirent entry;
    int ret;

    ret = fs_stat(path, &entry);
    if (ret == 0 && entry.type == FS_DIR_ENTRY_DIR) {
        return 0;
    }

    ret = fs_mkdir(path);
    if (ret < 0 && ret != -EEXIST) {
        LOG_ERR("Failed to create directory %s: %d", path, ret);
        return ret;
    }

    return 0;
}

static int get_next_index(const char *dir_path, uint32_t *index)
{
    struct fs_dir_t dir;
    struct fs_dirent entry;
    uint32_t max_index = 0;
    int ret;

    ret = fs_opendir(&dir, dir_path);
    if (ret < 0) {
        LOG_ERR("Failed to open directory %s: %d", dir_path, ret);
        return ret;
    }

    while (fs_readdir(&dir, &entry) == 0) {
        if (entry.name[0] == 0) {
            break;
        }
        uint32_t current_index;
        if (sscanf(entry.name, "%u.dat", &current_index) == 1) {
            max_index = MAX(max_index, current_index);
        }
    }

    fs_closedir(&dir);
    *index = max_index + 1;
    return 0;
}

/* API Implementation */
int flash_fs_init(void)
{
    int ret;

    /* Mount filesystem */
    ret = fs_mount(&fs_mnt);
    if (ret < 0) {
        LOG_ERR("Failed to mount filesystem: %d", ret);
        return ret;
    }

    /* Create required directories */
    ret = ensure_directory(FLASH_FS_MOUNT_POINT "/measurements");
    if (ret < 0) {
        return ret;
    }

    ret = ensure_directory(FLASH_FS_MOUNT_POINT "/config");
    if (ret < 0) {
        return ret;
    }

    LOG_INF("Flash filesystem initialized");
    return 0;
}

int flash_fs_store_measurement(const MEASUREMENT_RESULT_s *result)
{
    char path[FLASH_FS_MAX_FILENAME];
    uint32_t index;
    struct fs_file_t file;
    int ret;

    k_mutex_lock(&fs_mutex, K_FOREVER);

    /* Get next measurement index */
    ret = get_next_index(FLASH_FS_MOUNT_POINT "/measurements", &index);
    if (ret < 0) {
        k_mutex_unlock(&fs_mutex);
        return ret;
    }

    /* Create measurement file */
    snprintf(path, sizeof(path), FLASH_FS_MOUNT_POINT "/measurements/%u.dat", index);
    ret = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
    if (ret < 0) {
        LOG_ERR("Failed to create measurement file: %d", ret);
        k_mutex_unlock(&fs_mutex);
        return ret;
    }

    /* Write measurement data */
    ret = fs_write(&file, result, sizeof(MEASUREMENT_RESULT_s));
    fs_close(&file);

    k_mutex_unlock(&fs_mutex);
    return ret < 0 ? ret : 0;
}

int flash_fs_read_measurement(uint32_t index, MEASUREMENT_RESULT_s *result)
{
    char path[FLASH_FS_MAX_FILENAME];
    struct fs_file_t file;
    int ret;

    k_mutex_lock(&fs_mutex, K_FOREVER);

    /* Open measurement file */
    snprintf(path, sizeof(path), FLASH_FS_MOUNT_POINT "/measurements/%u.dat", index);
    ret = fs_open(&file, path, FS_O_READ);
    if (ret < 0) {
        k_mutex_unlock(&fs_mutex);
        return ret;
    }

    /* Read measurement data */
    ret = fs_read(&file, result, sizeof(MEASUREMENT_RESULT_s));
    fs_close(&file);

    k_mutex_unlock(&fs_mutex);
    return ret < 0 ? ret : 0;
}

int flash_fs_get_measurement_count(uint32_t *count)
{
    struct fs_dir_t dir;
    struct fs_dirent entry;
    uint32_t file_count = 0;
    int ret;

    k_mutex_lock(&fs_mutex, K_FOREVER);

    ret = fs_opendir(&dir, FLASH_FS_MOUNT_POINT "/measurements");
    if (ret < 0) {
        k_mutex_unlock(&fs_mutex);
        return ret;
    }

    while (fs_readdir(&dir, &entry) == 0) {
        if (entry.name[0] == 0) {
            break;
        }
        if (entry.type == FS_DIR_ENTRY_FILE) {
            file_count++;
        }
    }

    fs_closedir(&dir);
    *count = file_count;

    k_mutex_unlock(&fs_mutex);
    return 0;
}

int flash_fs_delete_measurement(uint32_t index)
{
    char path[FLASH_FS_MAX_FILENAME];
    int ret;

    k_mutex_lock(&fs_mutex, K_FOREVER);

    snprintf(path, sizeof(path), FLASH_FS_MOUNT_POINT "/measurements/%u.dat", index);
    ret = fs_unlink(path);

    k_mutex_unlock(&fs_mutex);
    return ret;
}

int flash_fs_clear_measurements(void)
{
    struct fs_dir_t dir;
    struct fs_dirent entry;
    char path[FLASH_FS_MAX_FILENAME];
    int ret;

    k_mutex_lock(&fs_mutex, K_FOREVER);

    ret = fs_opendir(&dir, FLASH_FS_MOUNT_POINT "/measurements");
    if (ret < 0) {
        k_mutex_unlock(&fs_mutex);
        return ret;
    }

    while (fs_readdir(&dir, &entry) == 0) {
        if (entry.name[0] == 0) {
            break;
        }
        if (entry.type == FS_DIR_ENTRY_FILE) {
            snprintf(path, sizeof(path), FLASH_FS_MOUNT_POINT "/measurements/%s", entry.name);
            fs_unlink(path);
        }
    }

    fs_closedir(&dir);
    k_mutex_unlock(&fs_mutex);
    return 0;
}

int flash_fs_store_config(const void *data, size_t size)
{
    struct fs_file_t file;
    int ret;

    k_mutex_lock(&fs_mutex, K_FOREVER);

    ret = fs_open(&file, FLASH_FS_MOUNT_POINT "/config/config.dat", FS_O_CREATE | FS_O_WRITE);
    if (ret < 0) {
        k_mutex_unlock(&fs_mutex);
        return ret;
    }

    ret = fs_write(&file, data, size);
    fs_close(&file);

    k_mutex_unlock(&fs_mutex);
    return ret < 0 ? ret : 0;
}

int flash_fs_read_config(void *data, size_t size)
{
    struct fs_file_t file;
    int ret;

    k_mutex_lock(&fs_mutex, K_FOREVER);

    ret = fs_open(&file, FLASH_FS_MOUNT_POINT "/config/config.dat", FS_O_READ);
    if (ret < 0) {
        k_mutex_unlock(&fs_mutex);
        return ret;
    }

    ret = fs_read(&file, data, size);
    fs_close(&file);

    k_mutex_unlock(&fs_mutex);
    return ret < 0 ? ret : 0;
}

int flash_fs_get_stats(size_t *total_bytes, size_t *used_bytes)
{
    struct fs_statvfs stats;
    int ret;

    k_mutex_lock(&fs_mutex, K_FOREVER);

    ret = fs_statvfs(FLASH_FS_MOUNT_POINT, &stats);
    if (ret < 0) {
        k_mutex_unlock(&fs_mutex);
        return ret;
    }

    *total_bytes = stats.f_bsize * stats.f_blocks;
    *used_bytes = stats.f_bsize * (stats.f_blocks - stats.f_bfree);

    k_mutex_unlock(&fs_mutex);
    return 0;
}
