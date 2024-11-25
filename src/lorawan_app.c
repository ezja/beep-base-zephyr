/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/logging/log.h>
#include "lorawan_app.h"

LOG_MODULE_REGISTER(lorawan_app, CONFIG_LOG_DEFAULT_LEVEL);

/* LoRaWAN join parameters */
#define JOIN_RETRY_INTERVAL K_SECONDS(10)
#define MAX_JOIN_RETRIES    10

/* LoRaWAN message ports */
#define LORAWAN_PORT_MEASUREMENT 1
#define LORAWAN_PORT_CONFIG     2

/* Thread stack size */
#define LORAWAN_STACK_SIZE 2048

/* Thread priority */
#define LORAWAN_PRIORITY 7

static struct k_work_delayable join_retry_work;
static uint8_t join_retries;
static lorawan_state_t current_state = LORAWAN_STATE_IDLE;
static struct k_mutex state_lock;
static struct k_sem tx_done_sem;
static bool lorawan_enabled = false;

K_THREAD_STACK_DEFINE(lorawan_stack, LORAWAN_STACK_SIZE);
static struct k_thread lorawan_thread_data;

static void set_state(lorawan_state_t new_state)
{
    k_mutex_lock(&state_lock, K_FOREVER);
    current_state = new_state;
    k_mutex_unlock(&state_lock);
}

static void lorawan_datarate_changed(enum lorawan_datarate dr)
{
    LOG_INF("Datarate changed: DR_%d", dr);
}

static void lorawan_join_cb(bool joined, int32_t err)
{
    if (joined) {
        LOG_INF("Join successful");
        set_state(LORAWAN_STATE_JOINED);
        join_retries = 0;
    } else {
        LOG_ERR("Join failed: %d", err);
        if (join_retries < MAX_JOIN_RETRIES) {
            join_retries++;
            k_work_schedule(&join_retry_work, JOIN_RETRY_INTERVAL);
        } else {
            set_state(LORAWAN_STATE_ERROR);
        }
    }
}

static void lorawan_tx_done_cb(bool ack_received, int32_t err)
{
    if (err) {
        LOG_ERR("Transmission failed: %d", err);
    } else {
        LOG_DBG("Transmission complete (ACK %s)", ack_received ? "received" : "not received");
    }
    k_sem_give(&tx_done_sem);
    set_state(LORAWAN_STATE_JOINED);
}

static void join_retry_handler(struct k_work *work)
{
    int ret;

    LOG_INF("Retrying join (attempt %d/%d)", join_retries, MAX_JOIN_RETRIES);
    ret = lorawan_join(LORAWAN_ACT_OTAA);
    if (ret < 0) {
        LOG_ERR("Failed to start join procedure: %d", ret);
        set_state(LORAWAN_STATE_ERROR);
    } else {
        set_state(LORAWAN_STATE_JOINING);
    }
}

static int encode_measurement(const MEASUREMENT_RESULT_s *result, uint8_t *buffer, size_t *size)
{
    if (*size < sizeof(MEASUREMENT_RESULT_s)) {
        return -ENOSPC;
    }

    /* Add message type */
    buffer[0] = result->type;
    uint8_t offset = 1;

    /* Encode measurement data based on type */
    switch (result->type) {
    case DS18B20:
        memcpy(&buffer[offset], &result->result.ds18B20, sizeof(result->result.ds18B20));
        offset += sizeof(result->result.ds18B20);
        break;

    case BME280:
        memcpy(&buffer[offset], &result->result.bme280, sizeof(result->result.bme280));
        offset += sizeof(result->result.bme280);
        break;

    case HX711:
        memcpy(&buffer[offset], &result->result.hx711, sizeof(result->result.hx711));
        offset += sizeof(result->result.hx711);
        break;

    case AUDIO_ADC:
        memcpy(&buffer[offset], &result->result.fft, sizeof(result->result.fft));
        offset += sizeof(result->result.fft);
        break;

    default:
        return -EINVAL;
    }

    *size = offset;
    return 0;
}

static void lorawan_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    struct lorawan_join_config join_cfg;
    int ret;

    /* Initialize LoRaWAN */
    ret = lorawan_start();
    if (ret < 0) {
        LOG_ERR("Failed to start LoRaWAN stack: %d", ret);
        set_state(LORAWAN_STATE_ERROR);
        return;
    }

    /* Configure join parameters */
    join_cfg.mode = LORAWAN_ACT_OTAA;
    join_cfg.dev_eui = ((lorawan_config_t *)p1)->dev_eui;
    join_cfg.join_eui = ((lorawan_config_t *)p1)->join_eui;
    join_cfg.app_key = ((lorawan_config_t *)p1)->app_key;

    ret = lorawan_join_config(&join_cfg);
    if (ret < 0) {
        LOG_ERR("Failed to configure join parameters: %d", ret);
        set_state(LORAWAN_STATE_ERROR);
        return;
    }

    /* Start join procedure */
    ret = lorawan_join(LORAWAN_ACT_OTAA);
    if (ret < 0) {
        LOG_ERR("Failed to start join procedure: %d", ret);
        set_state(LORAWAN_STATE_ERROR);
        return;
    }

    set_state(LORAWAN_STATE_JOINING);

    while (1) {
        k_sleep(K_MSEC(100));
    }
}

int lorawan_app_init(const lorawan_config_t *config)
{
    static const struct lorawan_callbacks callbacks = {
        .datarate_changed = lorawan_datarate_changed,
        .join_cb = lorawan_join_cb,
        .tx_done = lorawan_tx_done_cb,
    };

    k_mutex_init(&state_lock);
    k_sem_init(&tx_done_sem, 0, 1);
    k_work_init_delayable(&join_retry_work, join_retry_handler);

    lorawan_register_callback(&callbacks);

    k_thread_create(&lorawan_thread_data, lorawan_stack,
                   K_THREAD_STACK_SIZEOF(lorawan_stack),
                   lorawan_thread, (void *)config, NULL, NULL,
                   LORAWAN_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&lorawan_thread_data, "lorawan");

    return 0;
}

int lorawan_app_send_measurement(const MEASUREMENT_RESULT_s *result)
{
    uint8_t buffer[LORAWAN_MAX_DATA_SIZE];
    size_t size = sizeof(buffer);
    int ret;

    if (!lorawan_enabled || current_state != LORAWAN_STATE_JOINED) {
        return -ENOREADY;
    }

    ret = encode_measurement(result, buffer, &size);
    if (ret < 0) {
        return ret;
    }

    set_state(LORAWAN_STATE_SENDING);

    ret = lorawan_send(LORAWAN_PORT_MEASUREMENT, buffer, size,
                      LORAWAN_MSG_CONFIRMED);
    if (ret < 0) {
        LOG_ERR("Failed to send measurement: %d", ret);
        set_state(LORAWAN_STATE_JOINED);
        return ret;
    }

    /* Wait for transmission to complete */
    ret = k_sem_take(&tx_done_sem, K_SECONDS(30));
    if (ret < 0) {
        LOG_ERR("Transmission timeout");
        set_state(LORAWAN_STATE_JOINED);
        return ret;
    }

    return 0;
}

lorawan_state_t lorawan_app_get_state(void)
{
    lorawan_state_t state;

    k_mutex_lock(&state_lock, K_FOREVER);
    state = current_state;
    k_mutex_unlock(&state_lock);

    return state;
}

int lorawan_app_enable(bool enable)
{
    lorawan_enabled = enable;
    return 0;
}
