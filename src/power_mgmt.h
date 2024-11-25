/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef POWER_MGMT_H
#define POWER_MGMT_H

#include <zephyr/kernel.h>
#include <stdbool.h>

/* Wake-up sources */
#define WAKEUP_SOURCE_RTC      BIT(0)  /* RTC alarm */
#define WAKEUP_SOURCE_BLE      BIT(1)  /* BLE connection */
#define WAKEUP_SOURCE_BUTTON   BIT(2)  /* Button press */
#define WAKEUP_SOURCE_SENSOR   BIT(3)  /* Sensor interrupt */
#define WAKEUP_SOURCE_UART     BIT(4)  /* UART activity */

/* Sleep modes */
typedef enum {
    SLEEP_MODE_IDLE,      /* CPU sleep, peripherals active */
    SLEEP_MODE_DEEP,      /* Deep sleep, RTC only */
    SLEEP_MODE_STANDBY,   /* Standby mode, RAM retention */
    SLEEP_MODE_OFF,       /* Power off, no retention */
} sleep_mode_t;

/* Sleep configuration */
typedef struct {
    sleep_mode_t mode;            /* Sleep mode */
    uint32_t wakeup_sources;      /* Enabled wake-up sources */
    uint32_t sleep_timeout_ms;    /* Time before entering sleep */
    bool retain_memory;           /* Keep RAM contents */
} sleep_config_t;

/**
 * @brief Initialize power management
 *
 * @return 0 on success, negative errno code on failure
 */
int power_mgmt_init(void);

/**
 * @brief Configure sleep parameters
 *
 * @param config Sleep configuration
 * @return 0 on success, negative errno code on failure
 */
int power_mgmt_configure(const sleep_config_t *config);

/**
 * @brief Get current sleep configuration
 *
 * @param config Pointer to store configuration
 * @return 0 on success, negative errno code on failure
 */
int power_mgmt_get_config(sleep_config_t *config);

/**
 * @brief Enter sleep mode
 *
 * System will enter configured sleep mode after all conditions are met:
 * - No active measurements
 * - No BLE connection (unless enabled as wake source)
 * - No pending data transmissions
 *
 * @return 0 on success, negative errno code on failure
 */
int power_mgmt_enter_sleep(void);

/**
 * @brief Force immediate sleep mode entry
 *
 * System will enter sleep mode immediately, regardless of current activity.
 * Use with caution as this may interrupt ongoing operations.
 *
 * @return 0 on success, negative errno code on failure
 */
int power_mgmt_force_sleep(void);

/**
 * @brief Check if system can enter sleep mode
 *
 * @return true if sleep is allowed, false otherwise
 */
bool power_mgmt_can_sleep(void);

/**
 * @brief Get last wake-up source
 *
 * @return Wake-up source bitmap
 */
uint32_t power_mgmt_get_wakeup_source(void);

/**
 * @brief Enable or disable automatic sleep
 *
 * When enabled, system will automatically enter sleep mode
 * when conditions are met and sleep timeout expires.
 *
 * @param enable true to enable, false to disable
 * @return 0 on success, negative errno code on failure
 */
int power_mgmt_auto_sleep(bool enable);

/**
 * @brief Notify power management of activity
 *
 * Call this function when activity occurs that should
 * prevent or delay sleep mode entry.
 *
 * @return 0 on success, negative errno code on failure
 */
int power_mgmt_notify_activity(void);

/**
 * @brief Get current system power state
 *
 * @param voltage_mv Pointer to store voltage in millivolts
 * @param charging Pointer to store charging status
 * @return 0 on success, negative errno code on failure
 */
int power_mgmt_get_power_state(uint16_t *voltage_mv, bool *charging);

#endif /* POWER_MGMT_H */
