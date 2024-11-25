/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include "rtc_app.h"

LOG_MODULE_REGISTER(rtc_app, CONFIG_APP_LOG_LEVEL);

/* RTC device reference */
static const struct device *rtc_dev;

/* Callbacks */
static rtc_alarm_callback_t alarm_callback;
static rtc_time_update_callback_t time_callback;

/* Work item for periodic time updates */
static struct k_work_delayable time_update_work;

/* Time update interval (1 minute) */
#define TIME_UPDATE_INTERVAL K_MINUTES(1)

/* RTC alarm callback */
static void rtc_alarm_handler(uint8_t alarm_num)
{
    if (alarm_callback) {
        alarm_callback(alarm_num);
    }
}

/* Time update work handler */
static void time_update_handler(struct k_work *work)
{
    struct tm current_time;

    if (rtc_app_get_time(&current_time) == 0 && time_callback) {
        time_callback(&current_time);
    }

    /* Schedule next update */
    k_work_schedule(&time_update_work, TIME_UPDATE_INTERVAL);
}

int rtc_app_init(rtc_alarm_callback_t alarm_cb, rtc_time_update_callback_t time_cb)
{
    /* Get RTC device */
    rtc_dev = DEVICE_DT_GET(DT_ALIAS(rtc));
    if (!device_is_ready(rtc_dev)) {
        LOG_ERR("RTC device not ready");
        return -ENODEV;
    }

    /* Store callbacks */
    alarm_callback = alarm_cb;
    time_callback = time_cb;

    /* Initialize work item */
    k_work_init_delayable(&time_update_work, time_update_handler);

    /* Set alarm callback */
    ds3231_set_alarm_callback(rtc_dev, rtc_alarm_handler);

    /* Start time updates */
    k_work_schedule(&time_update_work, K_NO_WAIT);

    LOG_INF("RTC initialized");
    return 0;
}

int rtc_app_set_time(const struct tm *time)
{
    if (!time) {
        return -EINVAL;
    }

    return ds3231_set_time(rtc_dev, time);
}

int rtc_app_get_time(struct tm *time)
{
    if (!time) {
        return -EINVAL;
    }

    return ds3231_get_time(rtc_dev, time);
}

int rtc_app_set_alarm(uint8_t alarm_num, const struct tm *time)
{
    if (!time || (alarm_num != 1 && alarm_num != 2)) {
        return -EINVAL;
    }

    return ds3231_set_alarm(rtc_dev, alarm_num, time);
}

int rtc_app_get_alarm(uint8_t alarm_num, struct tm *time)
{
    if (!time || (alarm_num != 1 && alarm_num != 2)) {
        return -EINVAL;
    }

    return ds3231_get_alarm(rtc_dev, alarm_num, time);
}

int rtc_app_enable_alarm(uint8_t alarm_num, bool enable)
{
    if (alarm_num != 1 && alarm_num != 2) {
        return -EINVAL;
    }

    return ds3231_enable_alarm(rtc_dev, alarm_num, enable);
}

int rtc_app_clear_alarm(uint8_t alarm_num)
{
    if (alarm_num != 1 && alarm_num != 2) {
        return -EINVAL;
    }

    return ds3231_clear_alarm(rtc_dev, alarm_num);
}

int rtc_app_get_temperature(int16_t *temp)
{
    if (!temp) {
        return -EINVAL;
    }

    return ds3231_get_temperature(rtc_dev, temp);
}

void rtc_app_timestamp_to_tm(uint32_t timestamp, struct tm *tm)
{
    time_t time = timestamp;
    struct tm *temp = gmtime(&time);
    memcpy(tm, temp, sizeof(struct tm));
}

uint32_t rtc_app_tm_to_timestamp(const struct tm *tm)
{
    struct tm temp;
    memcpy(&temp, tm, sizeof(struct tm));
    return (uint32_t)mktime(&temp);
}
