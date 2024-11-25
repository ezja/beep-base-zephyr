/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMM_MGR_H
#define COMM_MGR_H

#include <zephyr/kernel.h>
#include "beep_protocol.h"
#include "beep_types.h"

/**
 * @brief Initialize communication manager
 *
 * @param config Communication configuration
 * @return 0 on success, negative errno code on failure
 */
int comm_mgr_init(const COMM_CONFIG_s *config);

/**
 * @brief Send measurement data
 *
 * Automatically selects communication method based on configuration
 * and current conditions.
 *
 * @param result Measurement result to send
 * @return 0 on success, negative errno code on failure
 */
int comm_mgr_send_measurement(const MEASUREMENT_RESULT_s *result);

/**
 * @brief Configure communication method
 *
 * @param config Communication configuration
 * @return 0 on success, negative errno code on failure
 */
int comm_mgr_configure(const COMM_CONFIG_s *config);

/**
 * @brief Get current communication configuration
 *
 * @param config Pointer to store configuration
 * @return 0 on success, negative errno code on failure
 */
int comm_mgr_get_config(COMM_CONFIG_s *config);

/**
 * @brief Get communication status
 *
 * @param status Pointer to store status
 * @return 0 on success, negative errno code on failure
 */
int comm_mgr_get_status(COMM_STATUS_s *status);

/**
 * @brief Force specific communication method
 *
 * @param method Communication method to use
 * @return 0 on success, negative errno code on failure
 */
int comm_mgr_force_method(COMM_METHOD_e method);

/**
 * @brief Check if specific communication method is available
 *
 * @param method Communication method to check
 * @return true if available, false if not
 */
bool comm_mgr_is_available(COMM_METHOD_e method);

/**
 * @brief Get signal strength for specific method
 *
 * @param method Communication method
 * @param rssi Pointer to store RSSI value
 * @return 0 on success, negative errno code on failure
 */
int comm_mgr_get_signal_strength(COMM_METHOD_e method, int8_t *rssi);

/**
 * @brief Enable or disable automatic fallback
 *
 * @param enable true to enable, false to disable
 * @return 0 on success, negative errno code on failure
 */
int comm_mgr_auto_fallback(bool enable);

/**
 * @brief Get last transmission status
 *
 * @param timestamp Pointer to store last success timestamp
 * @param failures Pointer to store failure count
 * @return 0 on success, negative errno code on failure
 */
int comm_mgr_get_transmission_status(uint32_t *timestamp, uint16_t *failures);

/**
 * @brief Reset transmission statistics
 *
 * @return 0 on success, negative errno code on failure
 */
int comm_mgr_reset_statistics(void);

/**
 * @brief Power down communication interfaces
 *
 * @return 0 on success, negative errno code on failure
 */
int comm_mgr_power_down(void);

/**
 * @brief Power up communication interfaces
 *
 * @return 0 on success, negative errno code on failure
 */
int comm_mgr_power_up(void);

#endif /* COMM_MGR_H */
