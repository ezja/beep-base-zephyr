/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include "ble_app.h"

LOG_MODULE_REGISTER(ble_app, CONFIG_LOG_DEFAULT_LEVEL);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static struct bt_conn *current_conn;
static const struct ble_callbacks *callbacks;
static uint8_t measurement_notify_enabled;
static uint8_t control_indicate_enabled;

/* Advertising data */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_BEEP_VAL),
};

/* BEEP Service Declaration */
BT_GATT_SERVICE_DEFINE(beep_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_BEEP),

    /* Measurement Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_BEEP_MEASUREMENT,
                          BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_NONE,
                          NULL, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    /* Configuration Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_BEEP_CONFIG,
                          BT_GATT_CHRC_WRITE,
                          BT_GATT_PERM_WRITE,
                          NULL, NULL, NULL),

    /* Control Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_BEEP_CONTROL,
                          BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
                          BT_GATT_PERM_WRITE,
                          NULL, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* Connection callbacks */
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection failed (err %u)", err);
        return;
    }

    LOG_INF("Connected");
    current_conn = bt_conn_ref(conn);

    if (callbacks && callbacks->connected) {
        callbacks->connected(conn);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected (reason %u)", reason);

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }

    if (callbacks && callbacks->disconnected) {
        callbacks->disconnected(conn);
    }

    /* Restart advertising */
    ble_app_start_adv();
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
                           enum bt_security_err err)
{
    if (!err) {
        LOG_INF("Security changed: level %u", level);
    } else {
        LOG_ERR("Security failed: level %u err %d", level, err);
    }
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
};

/* GATT callbacks */
static void measurement_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    measurement_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Measurement notifications %s", measurement_notify_enabled ? "enabled" : "disabled");
}

static void control_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    control_indicate_enabled = (value == BT_GATT_CCC_INDICATE);
    LOG_INF("Control indications %s", control_indicate_enabled ? "enabled" : "disabled");
}

static ssize_t write_config(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                          const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    if (callbacks && callbacks->config) {
        callbacks->config(buf, len);
    }
    return len;
}

static ssize_t write_control(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                           const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    if (len < 1) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    if (callbacks && callbacks->control) {
        callbacks->control(((uint8_t *)buf)[0], &((uint8_t *)buf)[1], len - 1);
    }
    return len;
}

int ble_app_init(const struct ble_callbacks *cb)
{
    int err;

    callbacks = cb;

    /* Initialize Bluetooth subsystem */
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }

    bt_conn_cb_register(&conn_callbacks);

    LOG_INF("Bluetooth initialized");
    return 0;
}

int ble_app_start_adv(void)
{
    int err;

    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return err;
    }

    LOG_INF("Advertising started");
    return 0;
}

int ble_app_stop_adv(void)
{
    int err;

    err = bt_le_adv_stop();
    if (err) {
        LOG_ERR("Advertising failed to stop (err %d)", err);
        return err;
    }

    LOG_INF("Advertising stopped");
    return 0;
}

int ble_app_send_measurement(const MEASUREMENT_RESULT_s *result)
{
    if (!measurement_notify_enabled || !current_conn) {
        return -ENOTCONN;
    }

    /* Serialize measurement data */
    uint8_t buffer[sizeof(MEASUREMENT_RESULT_s)];
    buffer[0] = result->type;
    buffer[1] = result->source;

    uint16_t len = 2;
    switch (result->type) {
    case DS18B20:
        memcpy(&buffer[len], &result->result.ds18B20, sizeof(result->result.ds18B20));
        len += sizeof(result->result.ds18B20);
        break;
    case BME280:
        memcpy(&buffer[len], &result->result.bme280, sizeof(result->result.bme280));
        len += sizeof(result->result.bme280);
        break;
    case HX711:
        memcpy(&buffer[len], &result->result.hx711, sizeof(result->result.hx711));
        len += sizeof(result->result.hx711);
        break;
    case AUDIO_ADC:
        memcpy(&buffer[len], &result->result.fft, sizeof(result->result.fft));
        len += sizeof(result->result.fft);
        break;
    default:
        return -EINVAL;
    }

    /* Send notification */
    return bt_gatt_notify(current_conn, &beep_svc.attrs[1], buffer, len);
}

bool ble_app_is_connected(void)
{
    return current_conn != NULL;
}

struct bt_conn *ble_app_get_connection(void)
{
    return current_conn;
}
