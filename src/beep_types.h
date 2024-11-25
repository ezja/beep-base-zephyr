/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BEEP_TYPES_H
#define BEEP_TYPES_H

#include <stdint.h>

/* Maximum number of DS18B20 sensors */
#define MAX_TEMP_SENSORS 8

/* Maximum number of HX711 channels */
#define HX711_N_CHANNELS 2

/* Maximum FFT size */
#define MAX_FFT_SIZE 256

/* Measurement source */
typedef enum {
    INTERNAL_SOURCE,
    BLE_SOURCE,
    LORAWAN_SOURCE,
} MEASUREMENT_SOURCE_e;

/* Measurement type */
typedef enum {
    DS18B20,
    BME280,
    HX711,
    AUDIO_ADC,
} MEASUREMENT_TYPE_e;

/* DS18B20 results */
typedef struct {
    uint8_t devices;
    int16_t temperatures[MAX_TEMP_SENSORS];
} DS18B20_RESULTS_s;

/* BME280 results */
typedef struct {
    int16_t temperature;
    uint32_t airPressure;
    uint16_t humidity;
} BME280_RESULT_s;

/* HX711 results */
typedef struct {
    uint8_t channel;
    uint8_t samples;
    int32_t value[HX711_N_CHANNELS];
} HX711_CONV_s;

/* FFT results */
typedef struct {
    uint16_t size;
    uint16_t frequency;
    uint16_t magnitude[MAX_FFT_SIZE];
} FFT_RESULT_s;

/* Combined measurement result */
typedef struct {
    MEASUREMENT_TYPE_e type;
    MEASUREMENT_SOURCE_e source;
    union {
        DS18B20_RESULTS_s ds18B20;
        BME280_RESULT_s bme280;
        HX711_CONV_s hx711;
        FFT_RESULT_s fft;
    } result;
} MEASUREMENT_RESULT_s;

/* DS18B20 alarm thresholds */
typedef struct {
    int16_t Min;
    int16_t Max;
    int16_t Diff;
} DS_ALARM_s;

/* BME280 alarm thresholds */
typedef struct {
    int16_t Temp_Min;
    int16_t Temp_Max;
    int16_t Temp_Diff;
    uint16_t humidity_Min;
    uint16_t humidity_Max;
    uint16_t humidity_Diff;
    uint32_t press_Min;
    uint32_t press_Max;
    uint32_t press_Diff;
} BME_ALARM_s;

/* HX711 alarm thresholds */
typedef struct {
    int32_t Min;
    int32_t Max;
    int32_t Diff;
} HX711_ALARM_s;

/* Audio alarm thresholds */
typedef struct {
    uint16_t Min;
    uint16_t Max;
    uint16_t Diff;
    uint16_t FreqMin;
    uint16_t FreqMax;
} AUDIO_ALARM_s;

/* Combined alarm configuration */
typedef struct {
    MEASUREMENT_TYPE_e type;
    union {
        DS_ALARM_s ds;
        BME_ALARM_s bme;
        HX711_ALARM_s hx;
        AUDIO_ALARM_s audio;
    } thr;
} ALARM_CONFIG_s;

/* Sensor configuration */
typedef struct {
    uint8_t enabled;
    uint16_t interval;
    uint8_t resolution;
    uint8_t gain;
    uint16_t samples;
} SENSOR_CONFIG_s;

/* Measurement schedule */
typedef struct {
    uint8_t enabled;
    uint8_t hour;
    uint8_t minute;
    uint16_t interval;
} SCHEDULE_CONFIG_s;

/* LoRaWAN parameters */
typedef struct {
    uint8_t enabled;
    uint8_t confirmed;
    uint8_t port;
    uint8_t dataRate;
    uint8_t txPower;
    uint16_t interval;
} LORAWAN_CONFIG_s;

#endif /* BEEP_TYPES_H */
