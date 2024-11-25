/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "audio_app.h"
#include "lorawan_app.h"
#include "beep_protocol.h"
#include "beep_types.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* Thread stack sizes */
#define SENSOR_STACK_SIZE 2048
#define AUDIO_STACK_SIZE 4096
#define DATA_STACK_SIZE 2048

/* Thread priorities */
#define SENSOR_PRIORITY 5
#define AUDIO_PRIORITY 6
#define DATA_PRIORITY 7

/* Measurement intervals (in seconds) */
#define TEMP_INTERVAL 300  /* 5 minutes */
#define ENV_INTERVAL 300   /* 5 minutes */
#define WEIGHT_INTERVAL 300 /* 5 minutes */
#define AUDIO_INTERVAL 3600 /* 1 hour */

/* Thread stacks */
K_THREAD_STACK_DEFINE(sensor_stack, SENSOR_STACK_SIZE);
K_THREAD_STACK_DEFINE(audio_stack, AUDIO_STACK_SIZE);
K_THREAD_STACK_DEFINE(data_stack, DATA_STACK_SIZE);

/* Thread data */
static struct k_thread sensor_thread_data;
static struct k_thread audio_thread_data;
static struct k_thread data_thread_data;

/* Message queue for measurements */
K_MSGQ_DEFINE(measurement_msgq, sizeof(MEASUREMENT_RESULT_s), 10, 4);

/* LoRaWAN configuration */
static const lorawan_config_t lorawan_config = {
    .dev_eui = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  /* Set your DevEUI */
    .join_eui = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* Set your JoinEUI */
    .app_key = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* Set your AppKey */
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    .adr_enabled = true,
    .data_rate = 0,
    .tx_power = 0,
    .tx_interval = 300,  /* 5 minutes */
};

/* Device references */
static const struct device *bme280_dev;
static const struct device *ds18b20_dev[MAX_TEMP_SENSORS];
static const struct device *hx711_dev;
static int num_temp_sensors;

/* Measurement callback for audio processing */
static void measurement_handler(MEASUREMENT_RESULT_s *result)
{
    int ret = k_msgq_put(&measurement_msgq, result, K_NO_WAIT);
    if (ret < 0) {
        LOG_ERR("Failed to queue measurement: %d", ret);
    }
}

/* Sensor measurement thread */
static void sensor_thread(void *p1, void *p2, void *p3)
{
    MEASUREMENT_RESULT_s result;
    struct sensor_value temp, press, humidity;
    int ret;

    while (1) {
        /* Read BME280 environmental data */
        if (bme280_dev != NULL) {
            ret = sensor_sample_fetch(bme280_dev);
            if (ret == 0) {
                sensor_channel_get(bme280_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
                sensor_channel_get(bme280_dev, SENSOR_CHAN_PRESS, &press);
                sensor_channel_get(bme280_dev, SENSOR_CHAN_HUMIDITY, &humidity);

                result.type = BME280;
                result.source = INTERNAL_SOURCE;
                result.result.bme280.temperature = temp.val1 * 100 + temp.val2 / 10000;
                result.result.bme280.airPressure = press.val1;
                result.result.bme280.humidity = humidity.val1 * 100 + humidity.val2 / 10000;

                measurement_handler(&result);
            }
        }

        /* Read DS18B20 temperature sensors */
        for (int i = 0; i < num_temp_sensors; i++) {
            if (ds18b20_dev[i] != NULL) {
                ret = sensor_sample_fetch(ds18b20_dev[i]);
                if (ret == 0) {
                    sensor_channel_get(ds18b20_dev[i], SENSOR_CHAN_AMBIENT_TEMP, &temp);

                    result.type = DS18B20;
                    result.source = INTERNAL_SOURCE;
                    result.result.ds18B20.devices = num_temp_sensors;
                    result.result.ds18B20.temperatures[i] = temp.val1 * 100 + temp.val2 / 10000;

                    measurement_handler(&result);
                }
            }
        }

        /* Read HX711 weight sensor */
        if (hx711_dev != NULL) {
            ret = sensor_sample_fetch(hx711_dev);
            if (ret == 0) {
                struct sensor_value weight;
                sensor_channel_get(hx711_dev, SENSOR_CHAN_WEIGHT, &weight);

                result.type = HX711;
                result.source = INTERNAL_SOURCE;
                result.result.hx711.channel = 0;
                result.result.hx711.samples = 1;
                result.result.hx711.value[0] = weight.val1;

                measurement_handler(&result);
            }
        }

        k_sleep(K_SECONDS(TEMP_INTERVAL));
    }
}

/* Audio processing thread */
static void audio_thread(void *p1, void *p2, void *p3)
{
    while (1) {
        if (!audio_app_busy()) {
            audio_app_start(true, INTERNAL_SOURCE);
            while (audio_app_busy()) {
                k_sleep(K_MSEC(100));
            }
        }
        k_sleep(K_SECONDS(AUDIO_INTERVAL));
    }
}

/* Data transmission thread */
static void data_thread(void *p1, void *p2, void *p3)
{
    MEASUREMENT_RESULT_s result;

    while (1) {
        /* Wait for measurement data */
        if (k_msgq_get(&measurement_msgq, &result, K_FOREVER) == 0) {
            /* Send measurement via LoRaWAN if connected */
            if (lorawan_app_get_state() == LORAWAN_STATE_JOINED) {
                lorawan_app_send_measurement(&result);
            }
        }
    }
}

int main(void)
{
    int ret;

    LOG_INF("BEEP Base Firmware v%d.%d.%d", 
            FIRMWARE_MAJOR, FIRMWARE_MINOR, FIRMWARE_SUB);

    /* Initialize LoRaWAN */
    ret = lorawan_app_init(&lorawan_config);
    if (ret < 0) {
        LOG_ERR("Failed to initialize LoRaWAN: %d", ret);
        return ret;
    }

    /* Initialize audio subsystem */
    audio_app_init(measurement_handler);

    /* Get device references */
    bme280_dev = DEVICE_DT_GET_ANY(bosch_bme280);
    if (!device_is_ready(bme280_dev)) {
        LOG_WRN("BME280 device not found");
        bme280_dev = NULL;
    }

    /* Find all DS18B20 sensors */
    num_temp_sensors = 0;
    for (int i = 0; i < MAX_TEMP_SENSORS; i++) {
        char name[16];
        snprintf(name, sizeof(name), "DS18B20_%d", i);
        ds18b20_dev[i] = device_get_binding(name);
        if (ds18b20_dev[i] != NULL && device_is_ready(ds18b20_dev[i])) {
            num_temp_sensors++;
        } else {
            break;
        }
    }
    LOG_INF("Found %d DS18B20 sensors", num_temp_sensors);

    /* Get HX711 device */
    hx711_dev = DEVICE_DT_GET_ANY(avia_hx711);
    if (!device_is_ready(hx711_dev)) {
        LOG_WRN("HX711 device not found");
        hx711_dev = NULL;
    }

    /* Create threads */
    k_thread_create(&sensor_thread_data, sensor_stack,
                   K_THREAD_STACK_SIZEOF(sensor_stack),
                   sensor_thread, NULL, NULL, NULL,
                   SENSOR_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&sensor_thread_data, "sensors");

    k_thread_create(&audio_thread_data, audio_stack,
                   K_THREAD_STACK_SIZEOF(audio_stack),
                   audio_thread, NULL, NULL, NULL,
                   AUDIO_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&audio_thread_data, "audio");

    k_thread_create(&data_thread_data, data_stack,
                   K_THREAD_STACK_SIZEOF(data_stack),
                   data_thread, NULL, NULL, NULL,
                   DATA_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&data_thread_data, "data");

    /* Enable LoRaWAN */
    lorawan_app_enable(true);

    return 0;
}
