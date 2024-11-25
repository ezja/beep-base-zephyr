/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_info.h>
#include <net/socket.h>
#include "cellular_app.h"
#include "flash_fs.h"

LOG_MODULE_REGISTER(cellular_app, CONFIG_APP_LOG_LEVEL);

/* Global cellular state */
struct cellular_context cellular_state;

/* Write buffer */
static uint8_t write_buffer_data[CONFIG_CELLULAR_WRITE_BUFFER_SIZE];
static struct k_work_delayable flush_work;

/* Temperature reference for calibration */
static const struct device *temp_ref_dev;

/* Initialize cellular application */
int cellular_app_init(const cellular_config_t *config)
{
    int err;

    /* Initialize state */
    memset(&cellular_state, 0, sizeof(cellular_state));
    k_mutex_init(&cellular_state.lock);
    
    if (config) {
        memcpy(&cellular_state.config, config, sizeof(cellular_config_t));
    } else {
        /* Set defaults */
        cellular_state.config.psm.enabled = true;
        cellular_state.config.psm.tau_sec = 43200;  /* 12 hours */
        cellular_state.config.psm.active_sec = 60;  /* 1 minute */
        
        cellular_state.config.edrx.enabled = true;
        cellular_state.config.edrx.cycle = 5;       /* 81.92 seconds */
        
        cellular_state.config.buffer.buffer_size = sizeof(write_buffer_data);
        cellular_state.config.buffer.flush_ms = 100;
        cellular_state.config.buffer.wear_leveling = true;
        
        cellular_state.config.cal.auto_cal = true;
        cellular_state.config.cal.interval_h = 24;
        
        cellular_state.config.sync.timeout_ms = 1000;
        cellular_state.config.sync.max_retries = 3;
        
        cellular_state.config.wakeup.enabled = true;
        cellular_state.config.wakeup.min_latency = 100;
        cellular_state.config.wakeup.max_retries = 3;
    }

    /* Initialize modem library */
    err = nrf_modem_lib_init();
    if (err) {
        LOG_ERR("Failed to initialize modem library: %d", err);
        return err;
    }

    /* Initialize write buffer */
    k_work_init_delayable(&flush_work, flush_write_buffer);

    /* Get reference temperature device */
    temp_ref_dev = DEVICE_DT_GET(DT_ALIAS(temp_ref));
    if (!device_is_ready(temp_ref_dev)) {
        LOG_WRN("Temperature reference device not ready");
    }

    /* Configure modem */
    err = configure_modem();
    if (err) {
        LOG_ERR("Failed to configure modem: %d", err);
        return err;
    }

    cellular_state.state = CELLULAR_STATE_IDLE;
    return 0;
}

/* Configure modem with power saving features */
static int configure_modem(void)
{
    int err;

    /* Configure network mode */
    err = lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_LTEM_GPS,
                                LTE_LC_SYSTEM_MODE_PREFER_LTEM);
    if (err) {
        LOG_ERR("Failed to set system mode: %d", err);
        return err;
    }

    /* Configure PSM if enabled */
    if (cellular_state.config.psm.enabled) {
        char tau_str[8], active_str[8];
        snprintf(tau_str, sizeof(tau_str), "%02X",
                cellular_state.config.psm.tau_sec / 60);
        snprintf(active_str, sizeof(active_str), "%02X",
                cellular_state.config.psm.active_sec);
        
        err = lte_lc_psm_param_set(tau_str, active_str);
        if (err) {
            LOG_ERR("Failed to set PSM parameters: %d", err);
            return err;
        }
        
        err = lte_lc_psm_req(true);
        if (err) {
            LOG_ERR("Failed to enable PSM: %d", err);
            return err;
        }
    }

    /* Configure eDRX if enabled */
    if (cellular_state.config.edrx.enabled) {
        struct lte_lc_edrx_cfg cfg = {
            .mode = cellular_state.config.edrx.mode,
            .edrx = cellular_state.config.edrx.cycle,
            .ptw = cellular_state.config.edrx.ptw
        };
        
        err = lte_lc_edrx_param_set(&cfg);
        if (err) {
            LOG_ERR("Failed to set eDRX parameters: %d", err);
            return err;
        }
        
        err = lte_lc_edrx_req(true);
        if (err) {
            LOG_ERR("Failed to enable eDRX: %d", err);
            return err;
        }
    }

    return 0;
}

/* Write buffer management */
static void flush_write_buffer(struct k_work *work)
{
    k_mutex_lock(&cellular_state.lock, K_FOREVER);
    
    if (cellular_state.config.buffer.wear_leveling) {
        /* Implement wear leveling strategy */
        flash_write_with_leveling(write_buffer_data,
                                cellular_state.stats.buffer_usage_max);
    } else {
        flash_write(write_buffer_data,
                   cellular_state.stats.buffer_usage_max);
    }
    
    cellular_state.stats.buffer_usage_max = 0;
    k_mutex_unlock(&cellular_state.lock);
}

/* Send measurement data */
int cellular_app_send_measurement(const MEASUREMENT_RESULT_s *result)
{
    int err;
    uint32_t start_time = k_uptime_get_32();

    k_mutex_lock(&cellular_state.lock, K_FOREVER);

    /* Check if we need to optimize wake-up */
    if (cellular_state.config.wakeup.enabled) {
        optimize_wakeup(start_time);
    }

    /* Apply calibration if needed */
    if (cellular_state.config.cal.auto_cal && temp_ref_dev) {
        apply_calibration(result);
    }

    /* Prepare and send data */
    cellular_state.state = CELLULAR_STATE_SENDING;
    err = send_data(result);
    if (err) {
        cellular_state.stats.messages_failed++;
    } else {
        cellular_state.stats.messages_sent++;
    }

    cellular_state.state = CELLULAR_STATE_CONNECTED;
    k_mutex_unlock(&cellular_state.lock);

    return err;
}

/* Rest of implementation with other enhanced features */
/* ... */
