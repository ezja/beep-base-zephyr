/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ALARM_APP_H
#define ALARM_APP_H

#include <zephyr/kernel.h>
#include "beep_types.h"

/* Alarm types */
typedef enum {
    ALARM_TEMP,       /* Temperature alarm */
    ALARM_HUMIDITY,   /* Humidity alarm */
    ALARM_PRESSURE,   /* Air pressure alarm */
    ALARM_WEIGHT,     /* Weight alarm */
    ALARM_BATTERY,    /* Battery level alarm */
    ALARM_TILT,       /* Tilt alarm */
    ALARM_AUDIO,      /* Audio level alarm */
} alarm_type_t;

/* Alarm callback */
typedef void (*alarm_callback_t)(alarm_type_t type, const MEASUREMENT_RESULT_s *result);

/**
 * @brief Initialize alarm system
 *
 * @param callback Alarm callback function
 * @return 0 on success, negative errno code on failure
 */
int alarm_app_init(alarm_callback_t callback);

/**
 * @brief Configure alarm thresholds
 *
 * @param config Alarm configuration
 * @return 0 on success, negative errno code on failure
 */
int alarm_app_config(const ALARM_CONFIG_s *config);

/**
 * @brief Get current alarm configuration
 *
 * @param config Pointer to store alarm configuration
 * @return 0 on success, negative errno code on failure
 */
int alarm_app_get_config(ALARM_CONFIG_s *config);

/**
 * @brief Process measurement for alarms
 *
 * @param result Measurement result to check
 * @return 0 on success, negative errno code on failure
 */
int alarm_app_process(const MEASUREMENT_RESULT_s *result);

/**
 * @brief Enable or disable alarm system
 *
 * @param enable true to enable, false to disable
 * @return 0 on success, negative errno code on failure
 */
int alarm_app_enable(bool enable);

/**
 * @brief Check if alarm system is enabled
 *
 * @return true if enabled, false if disabled
 */
bool alarm_app_is_enabled(void);

/**
 * @brief Clear all active alarms
 *
 * @return 0 on success, negative errno code on failure
 */
int alarm_app_clear(void);

#endif /* ALARM_APP_H */
