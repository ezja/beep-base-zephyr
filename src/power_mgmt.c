/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "power_mgmt.h"
#include "ble_app.h"
#include "lorawan_app.h"
#include "rtc_app.h"
#include "flash_fs.h"

LOG_MODULE_REGISTER(power_mgmt, CONFIG_APP_LOG_LEVEL);

/* Power management state */
static struct {
    sleep_config_t config;
    uint32_t last_activity;
    uint32_t last_wakeup_source;
    bool auto_sleep_enabled;
    struct k_mutex lock;
    struct k_work_delayable sleep_work;
} power_state;

/* GPIO configuration for wake-up sources */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec sensor_int = GPIO_DT_SPEC_GET(DT_ALIAS(sensor_int), gpios);
static struct gpio_callback button_cb_data;
static struct gpio_callback sensor_cb_data;

/* Work queue for power management */
K_THREAD_STACK_DEFINE(power_stack, 1024);
static struct k_work_q power_work_q;

/* Helper functions */
static void configure_wakeup_sources(void)
{
    if (power_state.config.wakeup_sources & WAKEUP_SOURCE_BUTTON) {
        gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
        gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    }

    if (power_state.config.wakeup_sources & WAKEUP_SOURCE_SENSOR) {
        gpio_pin_configure_dt(&sensor_int, GPIO_INPUT | GPIO_PULL_UP);
        gpio_pin_interrupt_configure_dt(&sensor_int, GPIO_INT_EDGE_TO_ACTIVE);
    }

    /* RTC alarm is always configured as wake source */
    /* BLE wake source is handled by the BLE stack */
    /* UART wake source is handled by the UART driver */
}

static void disable_unused_peripherals(void)
{
    /* Disable peripherals based on sleep mode */
    switch (power_state.config.mode) {
        case SLEEP_MODE_DEEP:
            /* Disable all peripherals except RTC */
            /* Implementation depends on specific hardware */
            break;

        case SLEEP_MODE_STANDBY:
            /* Disable most peripherals but keep RAM */
            break;

        case SLEEP_MODE_OFF:
            /* Disable all peripherals */
            break;

        default:
            break;
    }
}

static void prepare_for_sleep(void)
{
    /* Ensure all data is written to flash */
    flash_fs_sync();

    /* Complete any pending LoRaWAN transmissions */
    if (lorawan_app_get_state() == LORAWAN_STATE_SENDING) {
        k_sleep(K_MSEC(100));
    }

    /* Disable unused peripherals */
    disable_unused_peripherals();

    /* Configure wake-up sources */
    configure_wakeup_sources();
}

static void sleep_work_handler(struct k_work *work)
{
    if (power_mgmt_can_sleep()) {
        power_mgmt_enter_sleep();
    }
}

static void button_callback(const struct device *dev, struct gpio_callback *cb,
                          uint32_t pins)
{
    power_state.last_wakeup_source |= WAKEUP_SOURCE_BUTTON;
    power_mgmt_notify_activity();
}

static void sensor_callback(const struct device *dev, struct gpio_callback *cb,
                          uint32_t pins)
{
    power_state.last_wakeup_source |= WAKEUP_SOURCE_SENSOR;
    power_mgmt_notify_activity();
}

/* API Implementation */
int power_mgmt_init(void)
{
    k_mutex_init(&power_state.lock);

    /* Initialize work queue */
    k_work_queue_init(&power_work_q);
    k_work_queue_start(&power_work_q, power_stack,
                      K_THREAD_STACK_SIZEOF(power_stack),
                      K_PRIO_PREEMPT(10), NULL);

    /* Initialize work items */
    k_work_init_delayable(&power_state.sleep_work, sleep_work_handler);

    /* Configure GPIO callbacks */
    gpio_init_callback(&button_cb_data, button_callback, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    gpio_init_callback(&sensor_cb_data, sensor_callback, BIT(sensor_int.pin));
    gpio_add_callback(sensor_int.port, &sensor_cb_data);

    /* Set default configuration */
    power_state.config.mode = SLEEP_MODE_IDLE;
    power_state.config.wakeup_sources = WAKEUP_SOURCE_RTC | WAKEUP_SOURCE_BLE;
    power_state.config.sleep_timeout_ms = 300000; /* 5 minutes */
    power_state.config.retain_memory = true;
    power_state.auto_sleep_enabled = true;

    return 0;
}

int power_mgmt_configure(const sleep_config_t *config)
{
    if (!config) {
        return -EINVAL;
    }

    k_mutex_lock(&power_state.lock, K_FOREVER);
    memcpy(&power_state.config, config, sizeof(sleep_config_t));
    k_mutex_unlock(&power_state.lock);

    return 0;
}

int power_mgmt_get_config(sleep_config_t *config)
{
    if (!config) {
        return -EINVAL;
    }

    k_mutex_lock(&power_state.lock, K_FOREVER);
    memcpy(config, &power_state.config, sizeof(sleep_config_t));
    k_mutex_unlock(&power_state.lock);

    return 0;
}

bool power_mgmt_can_sleep(void)
{
    /* Check if BLE is connected and not a wake source */
    if (ble_app_is_connected() &&
        !(power_state.config.wakeup_sources & WAKEUP_SOURCE_BLE)) {
        return false;
    }

    /* Check if LoRaWAN is transmitting */
    if (lorawan_app_get_state() == LORAWAN_STATE_SENDING) {
        return false;
    }

    /* Check if audio processing is active */
    if (audio_app_busy()) {
        return false;
    }

    /* Check if flash operations are pending */
    size_t total, used;
    if (flash_fs_get_stats(&total, &used) < 0) {
        return false;
    }

    /* Check if sleep timeout has expired */
    if (k_uptime_get() - power_state.last_activity <
        power_state.config.sleep_timeout_ms) {
        return false;
    }

    return true;
}

int power_mgmt_enter_sleep(void)
{
    if (!power_mgmt_can_sleep()) {
        return -EBUSY;
    }

    k_mutex_lock(&power_state.lock, K_FOREVER);

    /* Prepare system for sleep */
    prepare_for_sleep();

    /* Set power state based on configuration */
    switch (power_state.config.mode) {
        case SLEEP_MODE_DEEP:
            pm_state_force(0u, &(struct pm_state_info){
                .state = PM_STATE_SOFT_OFF,
                .min_residency_us = 1000,
                .exit_latency_us = 1000
            });
            break;

        case SLEEP_MODE_STANDBY:
            pm_state_force(0u, &(struct pm_state_info){
                .state = PM_STATE_STANDBY,
                .min_residency_us = 1000,
                .exit_latency_us = 1000
            });
            break;

        case SLEEP_MODE_OFF:
            pm_state_force(0u, &(struct pm_state_info){
                .state = PM_STATE_SOFT_OFF,
                .min_residency_us = 1000,
                .exit_latency_us = 1000
            });
            break;

        default:
            /* Just let system idle */
            break;
    }

    k_mutex_unlock(&power_state.lock);
    return 0;
}

int power_mgmt_force_sleep(void)
{
    k_mutex_lock(&power_state.lock, K_FOREVER);

    /* Force prepare and enter sleep */
    prepare_for_sleep();

    pm_state_force(0u, &(struct pm_state_info){
        .state = PM_STATE_SOFT_OFF,
        .min_residency_us = 1000,
        .exit_latency_us = 1000
    });

    k_mutex_unlock(&power_state.lock);
    return 0;
}

uint32_t power_mgmt_get_wakeup_source(void)
{
    return power_state.last_wakeup_source;
}

int power_mgmt_auto_sleep(bool enable)
{
    power_state.auto_sleep_enabled = enable;
    
    if (enable) {
        /* Schedule sleep check */
        k_work_schedule(&power_state.sleep_work,
                       K_MSEC(power_state.config.sleep_timeout_ms));
    } else {
        /* Cancel pending sleep */
        k_work_cancel_delayable(&power_state.sleep_work);
    }

    return 0;
}

int power_mgmt_notify_activity(void)
{
    power_state.last_activity = k_uptime_get();

    if (power_state.auto_sleep_enabled) {
        /* Reschedule sleep */
        k_work_reschedule(&power_state.sleep_work,
                         K_MSEC(power_state.config.sleep_timeout_ms));
    }

    return 0;
}

int power_mgmt_get_power_state(uint16_t *voltage_mv, bool *charging)
{
    /* Implementation depends on specific hardware */
    /* This is a placeholder implementation */
    if (voltage_mv) {
        *voltage_mv = 3300; /* 3.3V */
    }
    if (charging) {
        *charging = false;
    }
    return 0;
}
