/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CELLULAR_APP_H
#define CELLULAR_APP_H

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include "beep_types.h"

/* PSM (Power Saving Mode) configuration */
typedef struct {
    bool enabled;           /* Enable PSM */
    uint32_t tau_sec;      /* Periodic TAU interval in seconds */
    uint32_t active_sec;   /* Active time in seconds */
} psm_config_t;

/* eDRX (Extended Discontinuous Reception) configuration */
typedef struct {
    bool enabled;           /* Enable eDRX */
    uint8_t mode;          /* eDRX mode (WB-S1/NB-S1/A-GB-S1) */
    uint8_t ptw;           /* Paging Time Window */
    uint8_t cycle;         /* eDRX cycle length */
} edrx_config_t;

/* RAI (Release Assistance Indication) configuration */
typedef struct {
    bool enabled;           /* Enable RAI */
    bool no_more_data;     /* Indicate no more data after transmission */
    bool more_data;        /* Indicate more data pending */
} rai_config_t;

/* Write buffer configuration */
typedef struct {
    size_t buffer_size;     /* Buffer size in bytes */
    uint32_t flush_ms;      /* Flush timeout in milliseconds */
    bool wear_leveling;     /* Enable wear leveling */
} write_buffer_config_t;

/* Sensor calibration configuration */
typedef struct {
    bool auto_cal;          /* Enable automatic calibration */
    uint16_t interval_h;    /* Calibration interval in hours */
    int32_t manual_offset;  /* Manual calibration offset */
} sensor_cal_config_t;

/* Thread synchronization configuration */
typedef struct {
    uint32_t timeout_ms;    /* Lock timeout in milliseconds */
    uint8_t max_retries;    /* Maximum retry attempts */
    bool priority_inherit;  /* Enable priority inheritance */
} sync_config_t;

/* Wake-up optimization configuration */
typedef struct {
    bool enabled;           /* Enable wake-up optimization */
    uint32_t min_latency;   /* Minimum wake-up latency (ms) */
    uint8_t max_retries;    /* Maximum optimization attempts */
} wakeup_config_t;

/* Extended cellular configuration */
typedef struct {
    psm_config_t psm;              /* PSM configuration */
    edrx_config_t edrx;            /* eDRX configuration */
    rai_config_t rai;              /* RAI configuration */
    write_buffer_config_t buffer;   /* Write buffer configuration */
    sensor_cal_config_t cal;        /* Sensor calibration configuration */
    sync_config_t sync;             /* Thread synchronization configuration */
    wakeup_config_t wakeup;         /* Wake-up optimization configuration */
} cellular_config_t;

/* Cellular state */
typedef enum {
    CELLULAR_STATE_IDLE,
    CELLULAR_STATE_CONNECTING,
    CELLULAR_STATE_CONNECTED,
    CELLULAR_STATE_SENDING,
    CELLULAR_STATE_ERROR,
} cellular_state_t;

/* Cellular statistics */
typedef struct {
    uint32_t messages_sent;
    uint32_t messages_failed;
    uint32_t wake_latency_avg;
    uint32_t buffer_usage_max;
    uint32_t lock_timeouts;
} cellular_stats_t;

/* Global cellular state */
struct cellular_context {
    cellular_config_t config;
    cellular_state_t state;
    cellular_stats_t stats;
    struct k_mutex lock;
    struct k_work_delayable work;
};

extern struct cellular_context cellular_state;

/**
 * @brief Initialize cellular communication
 *
 * @param config Cellular configuration
 * @return 0 on success, negative errno on failure
 */
int cellular_app_init(const cellular_config_t *config);

/**
 * @brief Send measurement data over cellular
 *
 * @param result Pointer to measurement result
 * @return 0 on success, negative errno on failure
 */
int cellular_app_send_measurement(const MEASUREMENT_RESULT_s *result);

/**
 * @brief Get current cellular state
 *
 * @return Current state
 */
cellular_state_t cellular_app_get_state(void);

/**
 * @brief Configure cellular parameters
 *
 * @param config Cellular configuration
 * @return 0 on success, negative errno on failure
 */
int cellular_app_config(const cellular_config_t *config);

/**
 * @brief Get cellular configuration
 *
 * @param config Pointer to store configuration
 * @return 0 on success, negative errno on failure
 */
int cellular_app_get_config(cellular_config_t *config);

/**
 * @brief Get cellular signal strength
 *
 * @param rssi Pointer to store RSSI value in dBm
 * @return 0 on success, negative errno on failure
 */
int cellular_app_get_signal_strength(int8_t *rssi);

/**
 * @brief Power down cellular modem
 *
 * @return 0 on success, negative errno on failure
 */
int cellular_app_power_down(void);

/**
 * @brief Power up cellular modem
 *
 * @return 0 on success, negative errno on failure
 */
int cellular_app_power_up(void);

/**
 * @brief Get cellular statistics
 *
 * @param stats Pointer to store statistics
 * @return 0 on success, negative errno on failure
 */
int cellular_app_get_stats(cellular_stats_t *stats);

/**
 * @brief Reset cellular statistics
 *
 * @return 0 on success, negative errno on failure
 */
int cellular_app_reset_stats(void);

/**
 * @brief Force immediate buffer flush
 *
 * @return 0 on success, negative errno on failure
 */
int cellular_app_flush_buffer(void);

#endif /* CELLULAR_APP_H */
