/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_APP_H
#define BLE_APP_H

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include "beep_types.h"

/* BLE connection parameters */
#define BLE_CONN_INTERVAL_MIN    0x0028 /* 40ms */
#define BLE_CONN_INTERVAL_MAX    0x0038 /* 56ms */
#define BLE_CONN_LATENCY        0
#define BLE_CONN_TIMEOUT        400     /* 4s */

/* Service UUIDs */
#define BT_UUID_BEEP_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)
#define BT_UUID_BEEP BT_UUID_DECLARE_128(BT_UUID_BEEP_VAL)

/* Characteristic UUIDs */
#define BT_UUID_BEEP_MEASUREMENT_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)
#define BT_UUID_BEEP_CONFIG_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)
#define BT_UUID_BEEP_CONTROL_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef3)

#define BT_UUID_BEEP_MEASUREMENT BT_UUID_DECLARE_128(BT_UUID_BEEP_MEASUREMENT_VAL)
#define BT_UUID_BEEP_CONFIG BT_UUID_DECLARE_128(BT_UUID_BEEP_CONFIG_VAL)
#define BT_UUID_BEEP_CONTROL BT_UUID_DECLARE_128(BT_UUID_BEEP_CONTROL_VAL)

/* Control commands */
typedef enum {
    BLE_CMD_START_MEASUREMENT = 0x01,
    BLE_CMD_STOP_MEASUREMENT = 0x02,
    BLE_CMD_TARE_SCALE = 0x03,
    BLE_CMD_CALIBRATE_SCALE = 0x04,
    BLE_CMD_START_AUDIO = 0x05,
    BLE_CMD_STOP_AUDIO = 0x06,
} ble_control_cmd_t;

/* Callback types */
typedef void (*ble_connected_cb_t)(struct bt_conn *conn);
typedef void (*ble_disconnected_cb_t)(struct bt_conn *conn);
typedef void (*ble_measurement_cb_t)(const MEASUREMENT_RESULT_s *result);
typedef void (*ble_config_cb_t)(const uint8_t *data, uint16_t len);
typedef void (*ble_control_cb_t)(ble_control_cmd_t cmd, const uint8_t *data, uint16_t len);

/* Callback structure */
struct ble_callbacks {
    ble_connected_cb_t connected;
    ble_disconnected_cb_t disconnected;
    ble_measurement_cb_t measurement;
    ble_config_cb_t config;
    ble_control_cb_t control;
};

/**
 * @brief Initialize BLE stack and services
 *
 * @param callbacks Pointer to callback structure
 * @return 0 on success, negative errno code on failure
 */
int ble_app_init(const struct ble_callbacks *callbacks);

/**
 * @brief Start BLE advertising
 *
 * @return 0 on success, negative errno code on failure
 */
int ble_app_start_adv(void);

/**
 * @brief Stop BLE advertising
 *
 * @return 0 on success, negative errno code on failure
 */
int ble_app_stop_adv(void);

/**
 * @brief Send measurement notification
 *
 * @param result Pointer to measurement result
 * @return 0 on success, negative errno code on failure
 */
int ble_app_send_measurement(const MEASUREMENT_RESULT_s *result);

/**
 * @brief Check if device is connected
 *
 * @return true if connected, false otherwise
 */
bool ble_app_is_connected(void);

/**
 * @brief Get current connection
 *
 * @return Pointer to connection handle or NULL if not connected
 */
struct bt_conn *ble_app_get_connection(void);

#endif /* BLE_APP_H */
