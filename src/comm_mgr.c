/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "comm_mgr.h"
#include "lorawan_app.h"
#include "cellular_app.h"
#include "rtc_app.h"

LOG_MODULE_REGISTER(comm_mgr, CONFIG_APP_LOG_LEVEL);

/* Communication manager state */
static struct {
    COMM_CONFIG_s config;
    COMM_STATUS_s status;
    struct k_mutex lock;
    struct k_work_delayable retry_work;
    uint16_t current_retry;
    bool fallback_active;
} comm_state;

/* Work queue for retry handling */
K_THREAD_STACK_DEFINE(comm_stack, 2048);
static struct k_work_q comm_work_q;

/* Helper functions */
static bool is_cellular_better(void)
{
    int8_t lora_rssi, cell_rssi;
    
    /* Get signal strengths */
    if (lorawan_app_get_rssi(&lora_rssi) < 0) {
        lora_rssi = -127;
    }
    if (cellular_app_get_signal_strength(&cell_rssi) < 0) {
        cell_rssi = -127;
    }

    /* Update status */
    comm_state.status.lorawan_rssi = lora_rssi;
    comm_state.status.cellular_rssi = cell_rssi;

    /* Compare signal strengths with hysteresis */
    return (cell_rssi > lora_rssi + 10);  /* 10 dB hysteresis */
}

static void update_availability(void)
{
    /* Check LoRaWAN availability */
    comm_state.status.lorawan_available = 
        (lorawan_app_get_state() == LORAWAN_STATE_JOINED);

    /* Check cellular availability */
    comm_state.status.cellular_available = 
        (cellular_app_get_state() == CELLULAR_STATE_CONNECTED);
}

static COMM_METHOD_e select_method(void)
{
    update_availability();

    /* Handle forced method */
    if (comm_state.config.method != COMM_METHOD_AUTO) {
        if ((comm_state.config.method == COMM_METHOD_LORAWAN && 
             comm_state.status.lorawan_available) ||
            (comm_state.config.method == COMM_METHOD_CELLULAR && 
             comm_state.status.cellular_available)) {
            return comm_state.config.method;
        }
        if (!comm_state.config.auto_fallback) {
            return comm_state.config.method;
        }
    }

    /* Automatic selection */
    if (comm_state.status.lorawan_available && 
        comm_state.status.cellular_available) {
        return is_cellular_better() ? 
               COMM_METHOD_CELLULAR : COMM_METHOD_LORAWAN;
    }

    if (comm_state.status.lorawan_available) {
        return COMM_METHOD_LORAWAN;
    }

    if (comm_state.status.cellular_available) {
        return COMM_METHOD_CELLULAR;
    }

    /* Default to configured method if nothing available */
    return comm_state.config.method;
}

static void retry_work_handler(struct k_work *work)
{
    k_mutex_lock(&comm_state.lock, K_FOREVER);

    if (comm_state.current_retry < comm_state.config.retry_count) {
        /* Try sending again with current method */
        comm_state.current_retry++;
        k_work_schedule(&comm_state.retry_work, 
                       K_SECONDS(comm_state.config.retry_interval));
    } else if (comm_state.config.auto_fallback && !comm_state.fallback_active) {
        /* Try fallback method */
        comm_state.fallback_active = true;
        comm_state.current_retry = 0;
        k_work_schedule(&comm_state.retry_work, K_NO_WAIT);
    } else {
        /* All retries failed */
        comm_state.status.failed_transmissions++;
        comm_state.current_retry = 0;
        comm_state.fallback_active = false;
    }

    k_mutex_unlock(&comm_state.lock);
}

/* API Implementation */
int comm_mgr_init(const COMM_CONFIG_s *config)
{
    if (!config) {
        return -EINVAL;
    }

    /* Initialize state */
    k_mutex_init(&comm_state.lock);
    memcpy(&comm_state.config, config, sizeof(COMM_CONFIG_s));
    memset(&comm_state.status, 0, sizeof(COMM_STATUS_s));
    comm_state.current_retry = 0;
    comm_state.fallback_active = false;

    /* Initialize work queue */
    k_work_queue_init(&comm_work_q);
    k_work_queue_start(&comm_work_q, comm_stack,
                      K_THREAD_STACK_SIZEOF(comm_stack),
                      K_PRIO_PREEMPT(10), NULL);

    /* Initialize work items */
    k_work_init_delayable(&comm_state.retry_work, retry_work_handler);

    return 0;
}

int comm_mgr_send_measurement(const MEASUREMENT_RESULT_s *result)
{
    int ret;
    COMM_METHOD_e method;

    if (!result) {
        return -EINVAL;
    }

    k_mutex_lock(&comm_state.lock, K_FOREVER);

    /* Select communication method */
    method = select_method();
    comm_state.status.active_method = method;

    /* Try sending with selected method */
    if (method == COMM_METHOD_LORAWAN) {
        ret = lorawan_app_send_measurement(result);
    } else {
        ret = cellular_app_send_measurement(result);
    }

    if (ret == 0) {
        /* Success */
        comm_state.status.last_success_time = rtc_app_get_timestamp();
        comm_state.current_retry = 0;
        comm_state.fallback_active = false;
    } else {
        /* Start retry process */
        comm_state.current_retry = 1;
        k_work_schedule(&comm_state.retry_work,
                       K_SECONDS(comm_state.config.retry_interval));
        ret = -EAGAIN;
    }

    k_mutex_unlock(&comm_state.lock);
    return ret;
}

int comm_mgr_configure(const COMM_CONFIG_s *config)
{
    if (!config) {
        return -EINVAL;
    }

    k_mutex_lock(&comm_state.lock, K_FOREVER);
    memcpy(&comm_state.config, config, sizeof(COMM_CONFIG_s));
    k_mutex_unlock(&comm_state.lock);

    return 0;
}

int comm_mgr_get_config(COMM_CONFIG_s *config)
{
    if (!config) {
        return -EINVAL;
    }

    k_mutex_lock(&comm_state.lock, K_FOREVER);
    memcpy(config, &comm_state.config, sizeof(COMM_CONFIG_s));
    k_mutex_unlock(&comm_state.lock);

    return 0;
}

int comm_mgr_get_status(COMM_STATUS_s *status)
{
    if (!status) {
        return -EINVAL;
    }

    k_mutex_lock(&comm_state.lock, K_FOREVER);
    update_availability();
    memcpy(status, &comm_state.status, sizeof(COMM_STATUS_s));
    k_mutex_unlock(&comm_state.lock);

    return 0;
}

int comm_mgr_force_method(COMM_METHOD_e method)
{
    k_mutex_lock(&comm_state.lock, K_FOREVER);
    comm_state.config.method = method;
    comm_state.config.auto_fallback = false;
    k_mutex_unlock(&comm_state.lock);

    return 0;
}

bool comm_mgr_is_available(COMM_METHOD_e method)
{
    bool available = false;

    k_mutex_lock(&comm_state.lock, K_FOREVER);
    update_availability();

    if (method == COMM_METHOD_LORAWAN) {
        available = comm_state.status.lorawan_available;
    } else if (method == COMM_METHOD_CELLULAR) {
        available = comm_state.status.cellular_available;
    }

    k_mutex_unlock(&comm_state.lock);
    return available;
}

int comm_mgr_get_signal_strength(COMM_METHOD_e method, int8_t *rssi)
{
    int ret = 0;

    if (!rssi) {
        return -EINVAL;
    }

    k_mutex_lock(&comm_state.lock, K_FOREVER);

    if (method == COMM_METHOD_LORAWAN) {
        ret = lorawan_app_get_rssi(rssi);
        if (ret == 0) {
            comm_state.status.lorawan_rssi = *rssi;
        }
    } else if (method == COMM_METHOD_CELLULAR) {
        ret = cellular_app_get_signal_strength(rssi);
        if (ret == 0) {
            comm_state.status.cellular_rssi = *rssi;
        }
    } else {
        ret = -EINVAL;
    }

    k_mutex_unlock(&comm_state.lock);
    return ret;
}

int comm_mgr_auto_fallback(bool enable)
{
    k_mutex_lock(&comm_state.lock, K_FOREVER);
    comm_state.config.auto_fallback = enable;
    k_mutex_unlock(&comm_state.lock);

    return 0;
}

int comm_mgr_get_transmission_status(uint32_t *timestamp, uint16_t *failures)
{
    if (!timestamp || !failures) {
        return -EINVAL;
    }

    k_mutex_lock(&comm_state.lock, K_FOREVER);
    *timestamp = comm_state.status.last_success_time;
    *failures = comm_state.status.failed_transmissions;
    k_mutex_unlock(&comm_state.lock);

    return 0;
}

int comm_mgr_reset_statistics(void)
{
    k_mutex_lock(&comm_state.lock, K_FOREVER);
    comm_state.status.failed_transmissions = 0;
    comm_state.status.last_success_time = 0;
    k_mutex_unlock(&comm_state.lock);

    return 0;
}

int comm_mgr_power_down(void)
{
    int ret = 0;

    k_mutex_lock(&comm_state.lock, K_FOREVER);

    /* Cancel any pending retries */
    k_work_cancel_delayable(&comm_state.retry_work);

    /* Power down both interfaces */
    ret = lorawan_app_enable(false);
    if (ret == 0) {
        ret = cellular_app_power_down();
    }

    k_mutex_unlock(&comm_state.lock);
    return ret;
}

int comm_mgr_power_up(void)
{
    int ret = 0;

    k_mutex_lock(&comm_state.lock, K_FOREVER);

    /* Power up both interfaces */
    ret = lorawan_app_enable(true);
    if (ret == 0) {
        ret = cellular_app_power_up();
    }

    k_mutex_unlock(&comm_state.lock);
    return ret;
}
