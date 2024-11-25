/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "alarm_app.h"
#include "flash_fs.h"

LOG_MODULE_REGISTER(alarm_app, CONFIG_APP_LOG_LEVEL);

/* Alarm state */
static struct {
    bool enabled;
    alarm_callback_t callback;
    ALARM_CONFIG_s config;
    uint32_t active_alarms;
} alarm_state;

/* Mutex for alarm state access */
K_MUTEX_DEFINE(alarm_mutex);

/* Internal functions */
static bool check_ds18b20_alarm(const DS18B20_RESULTS_s *result, const DS_ALARM_s *config)
{
    for (int i = 0; i < result->devices; i++) {
        if (result->temperatures[i] > config->Max ||
            result->temperatures[i] < config->Min ||
            (config->Diff > 0 && 
             abs(result->temperatures[i] - result->temperatures[(i + 1) % result->devices]) > config->Diff)) {
            return true;
        }
    }
    return false;
}

static bool check_bme280_alarm(const BME280_RESULT_s *result, const BME_ALARM_s *config)
{
    /* Temperature check */
    if (result->temperature > config->Temp_Max ||
        result->temperature < config->Temp_Min ||
        (config->Temp_Diff > 0 && abs(result->temperature) > config->Temp_Diff)) {
        return true;
    }

    /* Humidity check */
    if (result->humidity > config->humidity_Max ||
        result->humidity < config->humidity_Min ||
        (config->humidity_Diff > 0 && abs(result->humidity) > config->humidity_Diff)) {
        return true;
    }

    /* Pressure check */
    if (result->airPressure > config->press_Max ||
        result->airPressure < config->press_Min ||
        (config->press_Diff > 0 && abs(result->airPressure) > config->press_Diff)) {
        return true;
    }

    return false;
}

static bool check_hx711_alarm(const HX711_CONV_s *result, const HX711_ALARM_s *config)
{
    for (int i = 0; i < HX711_N_CHANNELS; i++) {
        if (result->value[i] > config->Max ||
            result->value[i] < config->Min ||
            (config->Diff > 0 && abs(result->value[i]) > config->Diff)) {
            return true;
        }
    }
    return false;
}

/* API Implementation */
int alarm_app_init(alarm_callback_t callback)
{
    k_mutex_lock(&alarm_mutex, K_FOREVER);
    
    alarm_state.enabled = false;
    alarm_state.callback = callback;
    alarm_state.active_alarms = 0;
    memset(&alarm_state.config, 0, sizeof(ALARM_CONFIG_s));

    /* Try to load alarm configuration from flash */
    if (flash_fs_read_config(&alarm_state.config, sizeof(ALARM_CONFIG_s)) < 0) {
        LOG_WRN("No stored alarm configuration found");
    }

    k_mutex_unlock(&alarm_mutex);
    return 0;
}

int alarm_app_config(const ALARM_CONFIG_s *config)
{
    if (!config) {
        return -EINVAL;
    }

    k_mutex_lock(&alarm_mutex, K_FOREVER);
    
    /* Store new configuration */
    memcpy(&alarm_state.config, config, sizeof(ALARM_CONFIG_s));
    
    /* Save to flash */
    int ret = flash_fs_store_config(config, sizeof(ALARM_CONFIG_s));
    if (ret < 0) {
        LOG_ERR("Failed to store alarm configuration: %d", ret);
    }

    /* Clear active alarms on reconfiguration */
    alarm_state.active_alarms = 0;

    k_mutex_unlock(&alarm_mutex);
    return ret;
}

int alarm_app_get_config(ALARM_CONFIG_s *config)
{
    if (!config) {
        return -EINVAL;
    }

    k_mutex_lock(&alarm_mutex, K_FOREVER);
    memcpy(config, &alarm_state.config, sizeof(ALARM_CONFIG_s));
    k_mutex_unlock(&alarm_mutex);
    
    return 0;
}

int alarm_app_process(const MEASUREMENT_RESULT_s *result)
{
    if (!result || !alarm_state.enabled) {
        return -EINVAL;
    }

    k_mutex_lock(&alarm_mutex, K_FOREVER);

    bool alarm_triggered = false;
    alarm_type_t alarm_type;

    /* Check alarms based on measurement type */
    switch (result->type) {
        case DS18B20:
            if (alarm_state.config.type == DS18B20 &&
                check_ds18b20_alarm(&result->result.ds18B20, &alarm_state.config.thr.ds)) {
                alarm_triggered = true;
                alarm_type = ALARM_TEMP;
            }
            break;

        case BME280:
            if (alarm_state.config.type == BME280 &&
                check_bme280_alarm(&result->result.bme280, &alarm_state.config.thr.bme)) {
                alarm_triggered = true;
                alarm_type = ALARM_HUMIDITY;
            }
            break;

        case HX711:
            if (alarm_state.config.type == HX711 &&
                check_hx711_alarm(&result->result.hx711, &alarm_state.config.thr.hx)) {
                alarm_triggered = true;
                alarm_type = ALARM_WEIGHT;
            }
            break;

        default:
            break;
    }

    /* Handle alarm if triggered */
    if (alarm_triggered && alarm_state.callback) {
        alarm_state.callback(alarm_type, result);
        alarm_state.active_alarms |= BIT(alarm_type);
    }

    k_mutex_unlock(&alarm_mutex);
    return 0;
}

int alarm_app_enable(bool enable)
{
    k_mutex_lock(&alarm_mutex, K_FOREVER);
    alarm_state.enabled = enable;
    k_mutex_unlock(&alarm_mutex);
    return 0;
}

bool alarm_app_is_enabled(void)
{
    bool enabled;
    k_mutex_lock(&alarm_mutex, K_FOREVER);
    enabled = alarm_state.enabled;
    k_mutex_unlock(&alarm_mutex);
    return enabled;
}

int alarm_app_clear(void)
{
    k_mutex_lock(&alarm_mutex, K_FOREVER);
    alarm_state.active_alarms = 0;
    k_mutex_unlock(&alarm_mutex);
    return 0;
}
