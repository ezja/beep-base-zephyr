/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_DS3231_H_
#define ZEPHYR_DRIVERS_RTC_DS3231_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <time.h>

/* DS3231 Register Map */
#define DS3231_REG_SECONDS      0x00
#define DS3231_REG_MINUTES      0x01
#define DS3231_REG_HOURS        0x02
#define DS3231_REG_DAY          0x03
#define DS3231_REG_DATE         0x04
#define DS3231_REG_MONTH        0x05
#define DS3231_REG_YEAR         0x06
#define DS3231_REG_ALARM1_SEC   0x07
#define DS3231_REG_ALARM1_MIN   0x08
#define DS3231_REG_ALARM1_HOUR  0x09
#define DS3231_REG_ALARM1_DAY   0x0A
#define DS3231_REG_ALARM2_MIN   0x0B
#define DS3231_REG_ALARM2_HOUR  0x0C
#define DS3231_REG_ALARM2_DAY   0x0D
#define DS3231_REG_CONTROL      0x0E
#define DS3231_REG_STATUS       0x0F
#define DS3231_REG_AGING        0x10
#define DS3231_REG_TEMP_MSB     0x11
#define DS3231_REG_TEMP_LSB     0x12

/* Control Register Bits */
#define DS3231_CTRL_A1IE        BIT(0)  /* Alarm 1 Interrupt Enable */
#define DS3231_CTRL_A2IE        BIT(1)  /* Alarm 2 Interrupt Enable */
#define DS3231_CTRL_INTCN       BIT(2)  /* Interrupt Control */
#define DS3231_CTRL_RS1         BIT(3)  /* Rate Select 1 */
#define DS3231_CTRL_RS2         BIT(4)  /* Rate Select 2 */
#define DS3231_CTRL_CONV        BIT(5)  /* Convert Temperature */
#define DS3231_CTRL_BBSQW       BIT(6)  /* Battery-Backed Square Wave */
#define DS3231_CTRL_EOSC        BIT(7)  /* Enable Oscillator */

/* Status Register Bits */
#define DS3231_STAT_A1F         BIT(0)  /* Alarm 1 Flag */
#define DS3231_STAT_A2F         BIT(1)  /* Alarm 2 Flag */
#define DS3231_STAT_BSY         BIT(2)  /* Busy */
#define DS3231_STAT_EN32KHZ     BIT(3)  /* Enable 32kHz Output */
#define DS3231_STAT_OSF         BIT(7)  /* Oscillator Stop Flag */

/* Configuration structure */
struct ds3231_config {
    struct i2c_dt_spec i2c;
    struct gpio_dt_spec int_gpio;
};

/* Runtime data structure */
struct ds3231_data {
    struct k_mutex lock;
    struct k_work alarm_work;
    ds3231_alarm_callback_t alarm_cb;
};

/* Alarm callback type */
typedef void (*ds3231_alarm_callback_t)(uint8_t alarm_num);

/**
 * @brief Set current time
 *
 * @param dev Pointer to device structure
 * @param tm Time structure to set
 * @return 0 on success, negative errno code on failure
 */
int ds3231_set_time(const struct device *dev, const struct tm *tm);

/**
 * @brief Get current time
 *
 * @param dev Pointer to device structure
 * @param tm Time structure to fill
 * @return 0 on success, negative errno code on failure
 */
int ds3231_get_time(const struct device *dev, struct tm *tm);

/**
 * @brief Set alarm
 *
 * @param dev Pointer to device structure
 * @param alarm_num Alarm number (1 or 2)
 * @param tm Time structure with alarm time
 * @return 0 on success, negative errno code on failure
 */
int ds3231_set_alarm(const struct device *dev, uint8_t alarm_num, const struct tm *tm);

/**
 * @brief Get alarm
 *
 * @param dev Pointer to device structure
 * @param alarm_num Alarm number (1 or 2)
 * @param tm Time structure to fill with alarm time
 * @return 0 on success, negative errno code on failure
 */
int ds3231_get_alarm(const struct device *dev, uint8_t alarm_num, struct tm *tm);

/**
 * @brief Enable alarm
 *
 * @param dev Pointer to device structure
 * @param alarm_num Alarm number (1 or 2)
 * @param enable true to enable, false to disable
 * @return 0 on success, negative errno code on failure
 */
int ds3231_enable_alarm(const struct device *dev, uint8_t alarm_num, bool enable);

/**
 * @brief Clear alarm flag
 *
 * @param dev Pointer to device structure
 * @param alarm_num Alarm number (1 or 2)
 * @return 0 on success, negative errno code on failure
 */
int ds3231_clear_alarm(const struct device *dev, uint8_t alarm_num);

/**
 * @brief Get temperature
 *
 * @param dev Pointer to device structure
 * @param temp Pointer to store temperature (in 0.25°C units)
 * @return 0 on success, negative errno code on failure
 */
int ds3231_get_temperature(const struct device *dev, int16_t *temp);

/**
 * @brief Set alarm callback
 *
 * @param dev Pointer to device structure
 * @param callback Callback function
 * @return 0 on success, negative errno code on failure
 */
int ds3231_set_alarm_callback(const struct device *dev, ds3231_alarm_callback_t callback);

#endif /* ZEPHYR_DRIVERS_RTC_DS3231_H_ */
