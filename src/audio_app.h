/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AUDIO_APP_H
#define AUDIO_APP_H

#include <zephyr/kernel.h>
#include "beep_types.h"

/* Audio sampling parameters */
#define AUDIO_SAMPLE_RATE     16000  /* 16 kHz */
#define AUDIO_BITS_PER_SAMPLE 16     /* 16-bit */
#define AUDIO_MAX_DURATION    300    /* 5 minutes in seconds */
#define AUDIO_FRAME_SIZE      1024   /* Samples per frame */
#define AUDIO_FFT_SIZE        512    /* FFT size (power of 2) */

/* FFT frequency bands for LoRaWAN payload */
#define FFT_BAND_COUNT        16     /* Number of frequency bands */
#define FFT_BYTES_PER_BAND    2      /* Bytes per band magnitude */
#define FFT_HEADER_SIZE       4      /* Timestamp and config bytes */
#define LORAWAN_MAX_PAYLOAD   51     /* Maximum LoRaWAN payload size */

/* FFT band configuration */
typedef struct {
    uint16_t start_freq;  /* Band start frequency in Hz */
    uint16_t end_freq;    /* Band end frequency in Hz */
} fft_band_config_t;

/* Audio configuration */
typedef struct {
    uint32_t duration;    /* Recording duration in seconds */
    uint16_t interval;    /* Time between recordings in seconds */
    uint8_t gain;        /* Input gain (0-255) */
    bool agc_enabled;    /* Automatic gain control */
} audio_config_t;

/* FFT result for LoRaWAN */
typedef struct {
    uint32_t timestamp;  /* Recording timestamp */
    uint8_t config;      /* Configuration byte */
    uint16_t bands[FFT_BAND_COUNT]; /* Band magnitudes */
} fft_result_t;

/**
 * @brief Initialize audio subsystem
 *
 * @param callback Measurement callback function
 * @return 0 on success, negative errno code on failure
 */
int audio_app_init(measurement_callback_t callback);

/**
 * @brief Start or stop audio processing
 *
 * @param start true to start, false to stop
 * @param source Source of the request
 * @return 0 on success, negative errno code on failure
 */
int audio_app_start(bool start, MEASUREMENT_SOURCE_e source);

/**
 * @brief Check if audio processing is active
 *
 * @return true if busy, false if idle
 */
bool audio_app_busy(void);

/**
 * @brief Configure audio parameters
 *
 * @param config Audio configuration
 * @return 0 on success, negative errno code on failure
 */
int audio_app_config(const audio_config_t *config);

/**
 * @brief Get current audio configuration
 *
 * @param config Pointer to store configuration
 * @return 0 on success, negative errno code on failure
 */
int audio_app_get_config(audio_config_t *config);

/**
 * @brief Encode FFT result for LoRaWAN transmission
 *
 * @param result FFT result to encode
 * @param payload Buffer to store encoded payload
 * @param size Pointer to store payload size
 * @return 0 on success, negative errno code on failure
 */
int audio_app_encode_fft(const fft_result_t *result, uint8_t *payload, uint8_t *size);

/**
 * @brief Decode FFT payload from LoRaWAN
 *
 * @param payload Received payload
 * @param size Payload size
 * @param result Pointer to store decoded result
 * @return 0 on success, negative errno code on failure
 */
int audio_app_decode_fft(const uint8_t *payload, uint8_t size, fft_result_t *result);

#endif /* AUDIO_APP_H */
