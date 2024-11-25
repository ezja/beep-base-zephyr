/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RTC_APP_H
#define RTC_APP_H

#include <zephyr/kernel.h>
#include <time.h>

/* RTC callback types */
typedef void (*rtc_alarm_callback_t)(uint8_t alarm_num);
typedef void (*rtc_time_update_callback_t)(const struct tm *time);

/**
 * @brief Initialize RTC subsystem
 *
 * @param alarm_cb Alarm callback function
 * @param time_cb Time update callback function
 * @return 0 on success, negative errno code on failure
 */
int rtc_app_init(rtc_alarm_callback_t alarm_cb, rtc_time_update_callback_t time_cb);

/**
 * @brief Set current time
 *
 * @param time Time structure to set
 * @return 0 on success, negative errno code on failure
 */
int rtc_app_set_time(const struct tm *time);

/**
 * @brief Get current time
 *
 * @param time Time structure to fill
 * @return 0 on success, negative errno code on failure
 */
int rtc_app_get_time(struct tm *time);

/**
 * @brief Set alarm
 *
 * @param alarm_num Alarm number (1 or 2)
 * @param time Time structure with alarm time
 * @return 0 on success, negative errno code on failure
 */
int rtc_app_set_alarm(uint8_t alarm_num, const struct tm *time);

/**
 * @brief Get alarm time
 *
 * @param alarm_num Alarm number (1 or 2)
 * @param time Time structure to fill with alarm time
 * @return 0 on success, negative errno code on failure
 */
int rtc_app_get_alarm(uint8_t alarm_num, struct tm *time);

/**
 * @brief Enable alarm
 *
 * @param alarm_num Alarm number (1 or 2)
 * @param enable true to enable, false to disable
 * @return 0 on success, negative errno code on failure
 */
int rtc_app_enable_alarm(uint8_t alarm_num, bool enable);

/**
 * @brief Clear alarm
 *
 * @param alarm_num Alarm number (1 or 2)
 * @return 0 on success, negative errno code on failure
 */
int rtc_app_clear_alarm(uint8_t alarm_num);

/**
 * @brief Get temperature from RTC sensor
 *
 * @param temp Pointer to store temperature (in 0.25°C units)
 * @return 0 on success, negative errno code on failure
 */
int rtc_app_get_temperature(int16_t *temp);

/**
 * @brief Convert Unix timestamp to tm structure
 *
 * @param timestamp Unix timestamp
 * @param tm Time structure to fill
 */
void rtc_app_timestamp_to_tm(uint32_t timestamp, struct tm *tm);

/**
 * @brief Convert tm structure to Unix timestamp
 *
 * @param tm Time structure
 * @return Unix timestamp
 */
uint32_t rtc_app_tm_to_timestamp(const struct tm *tm);

#endif /* RTC_APP_H */
