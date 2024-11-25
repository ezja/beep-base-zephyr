/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LORAWAN_APP_H
#define LORAWAN_APP_H

#include <zephyr/kernel.h>
#include "beep_types.h"

/* LoRaWAN configuration */
#define LORAWAN_DEV_EUI_SIZE    8
#define LORAWAN_JOIN_EUI_SIZE   8
#define LORAWAN_APP_KEY_SIZE    16
#define LORAWAN_MAX_DATA_SIZE   242

/* LoRaWAN application states */
typedef enum {
    LORAWAN_STATE_IDLE,
    LORAWAN_STATE_JOINING,
    LORAWAN_STATE_JOINED,
    LORAWAN_STATE_SENDING,
    LORAWAN_STATE_ERROR,
} lorawan_state_t;

/* LoRaWAN configuration structure */
typedef struct {
    uint8_t dev_eui[LORAWAN_DEV_EUI_SIZE];
    uint8_t join_eui[LORAWAN_JOIN_EUI_SIZE];
    uint8_t app_key[LORAWAN_APP_KEY_SIZE];
    bool adr_enabled;
    uint8_t data_rate;
    uint8_t tx_power;
    uint32_t tx_interval;
} lorawan_config_t;

/**
 * @brief Initialize LoRaWAN stack and start join procedure
 *
 * @param config Pointer to LoRaWAN configuration
 * @return 0 on success, negative errno code on failure
 */
int lorawan_app_init(const lorawan_config_t *config);

/**
 * @brief Send measurement data over LoRaWAN
 *
 * @param result Pointer to measurement result
 * @return 0 on success, negative errno code on failure
 */
int lorawan_app_send_measurement(const MEASUREMENT_RESULT_s *result);

/**
 * @brief Get current LoRaWAN state
 *
 * @return Current state
 */
lorawan_state_t lorawan_app_get_state(void);

/**
 * @brief Enable or disable LoRaWAN communication
 *
 * @param enable true to enable, false to disable
 * @return 0 on success, negative errno code on failure
 */
int lorawan_app_enable(bool enable);

#endif /* LORAWAN_APP_H */
