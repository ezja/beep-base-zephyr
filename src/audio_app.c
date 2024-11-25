/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <arm_math.h>
#include <zephyr/logging/log.h>
#include "audio_app.h"
#include "rtc_app.h"

LOG_MODULE_REGISTER(audio_app, CONFIG_APP_LOG_LEVEL);

/* Audio buffers */
static int16_t audio_buffer[AUDIO_FRAME_SIZE];
static float32_t fft_buffer[AUDIO_FFT_SIZE * 2];
static float32_t window_buffer[AUDIO_FFT_SIZE];
static arm_cfft_instance_f32 fft_instance;

/* FFT band configuration */
static const fft_band_config_t fft_bands[FFT_BAND_COUNT] = {
    {100, 200},    /* 100-200 Hz */
    {200, 300},    /* 200-300 Hz */
    {300, 400},    /* 300-400 Hz */
    {400, 500},    /* 400-500 Hz */
    {500, 600},    /* 500-600 Hz */
    {600, 700},    /* 600-700 Hz */
    {700, 800},    /* 700-800 Hz */
    {800, 900},    /* 800-900 Hz */
    {900, 1000},   /* 900-1000 Hz */
    {1000, 1200},  /* 1000-1200 Hz */
    {1200, 1400},  /* 1200-1400 Hz */
    {1400, 1600},  /* 1400-1600 Hz */
    {1600, 1800},  /* 1600-1800 Hz */
    {1800, 2000},  /* 1800-2000 Hz */
    {2000, 2500},  /* 2000-2500 Hz */
    {2500, 3000}   /* 2500-3000 Hz */
};

/* Audio state */
static struct {
    const struct device *i2s_dev;
    measurement_callback_t callback;
    audio_config_t config;
    bool busy;
    uint32_t samples_collected;
    struct k_work_delayable stop_work;
} audio_state;

/* Work queue for audio processing */
K_THREAD_STACK_DEFINE(audio_stack, 4096);
static struct k_work_q audio_work_q;
static struct k_work process_work;

/* Helper functions */
static void apply_window(float32_t *data, size_t size)
{
    /* Apply Hanning window */
    for (size_t i = 0; i < size; i++) {
        data[i] *= window_buffer[i];
    }
}

static void calculate_band_magnitudes(const float32_t *fft_data, uint16_t *band_magnitudes)
{
    float32_t magnitude;
    uint16_t bin_freq;
    
    /* Calculate magnitude for each frequency band */
    for (int band = 0; band < FFT_BAND_COUNT; band++) {
        float32_t band_sum = 0;
        int bin_count = 0;

        /* Sum magnitudes for bins in this band */
        for (int bin = 1; bin < AUDIO_FFT_SIZE/2; bin++) {
            bin_freq = (bin * AUDIO_SAMPLE_RATE) / AUDIO_FFT_SIZE;
            
            if (bin_freq >= fft_bands[band].start_freq && 
                bin_freq <= fft_bands[band].end_freq) {
                /* Calculate magnitude from real and imaginary parts */
                magnitude = sqrtf(fft_data[2*bin] * fft_data[2*bin] + 
                                fft_data[2*bin+1] * fft_data[2*bin+1]);
                band_sum += magnitude;
                bin_count++;
            }
        }

        /* Store average magnitude for this band */
        if (bin_count > 0) {
            band_magnitudes[band] = (uint16_t)(band_sum / bin_count);
        } else {
            band_magnitudes[band] = 0;
        }
    }
}

static void process_audio_data(void)
{
    fft_result_t fft_result;
    struct tm time;

    /* Convert samples to float and apply window */
    for (int i = 0; i < AUDIO_FFT_SIZE; i++) {
        fft_buffer[2*i] = (float32_t)audio_buffer[i] / 32768.0f;
        fft_buffer[2*i+1] = 0.0f;
    }
    apply_window(fft_buffer, AUDIO_FFT_SIZE);

    /* Perform FFT */
    arm_cfft_f32(&fft_instance, fft_buffer, 0, 1);

    /* Get current time for timestamp */
    rtc_app_get_time(&time);
    fft_result.timestamp = rtc_app_tm_to_timestamp(&time);
    
    /* Set configuration byte */
    fft_result.config = (audio_state.config.gain & 0xF0) | 
                       ((audio_state.config.agc_enabled & 0x01) << 3) |
                       (AUDIO_FFT_SIZE >> 10); /* Log2 of FFT size / 1024 */

    /* Calculate band magnitudes */
    calculate_band_magnitudes(fft_buffer, fft_result.bands);

    /* Prepare LoRaWAN payload */
    uint8_t payload[LORAWAN_MAX_PAYLOAD];
    uint8_t payload_size;
    
    if (audio_app_encode_fft(&fft_result, payload, &payload_size) == 0) {
        /* Create measurement result */
        MEASUREMENT_RESULT_s result = {
            .type = AUDIO_ADC,
            .source = INTERNAL_SOURCE,
            .result.fft = {
                .size = AUDIO_FFT_SIZE,
                .frequency = AUDIO_SAMPLE_RATE
            }
        };
        memcpy(result.result.fft.magnitude, fft_result.bands, 
               sizeof(fft_result.bands));

        /* Send to callback */
        if (audio_state.callback) {
            audio_state.callback(&result);
        }
    }
}

static void audio_process_handler(struct k_work *work)
{
    process_audio_data();
}

static void audio_stop_handler(struct k_work *work)
{
    audio_app_start(false, INTERNAL_SOURCE);
}

/* API Implementation */
int audio_app_init(measurement_callback_t callback)
{
    /* Initialize work queue */
    k_work_queue_init(&audio_work_q);
    k_work_queue_start(&audio_work_q, audio_stack,
                      K_THREAD_STACK_SIZEOF(audio_stack),
                      K_PRIO_PREEMPT(10), NULL);

    /* Initialize work items */
    k_work_init(&process_work, audio_process_handler);
    k_work_init_delayable(&audio_state.stop_work, audio_stop_handler);

    /* Initialize FFT */
    arm_cfft_init_f32(&fft_instance, AUDIO_FFT_SIZE);

    /* Create Hanning window */
    for (int i = 0; i < AUDIO_FFT_SIZE; i++) {
        window_buffer[i] = 0.5f * (1.0f - cosf(2.0f * PI * i / (AUDIO_FFT_SIZE - 1)));
    }

    /* Store callback */
    audio_state.callback = callback;

    /* Get I2S device */
    audio_state.i2s_dev = DEVICE_DT_GET(DT_ALIAS(audio_in));
    if (!device_is_ready(audio_state.i2s_dev)) {
        LOG_ERR("I2S device not ready");
        return -ENODEV;
    }

    /* Set default configuration */
    audio_state.config.duration = 60;
    audio_state.config.interval = 3600;
    audio_state.config.gain = 128;
    audio_state.config.agc_enabled = true;

    return 0;
}

int audio_app_encode_fft(const fft_result_t *result, uint8_t *payload, uint8_t *size)
{
    if (!result || !payload || !size) {
        return -EINVAL;
    }

    /* Check maximum payload size */
    if (FFT_HEADER_SIZE + (FFT_BAND_COUNT * FFT_BYTES_PER_BAND) > LORAWAN_MAX_PAYLOAD) {
        return -ENOSPC;
    }

    /* Encode header */
    payload[0] = (result->timestamp >> 24) & 0xFF;
    payload[1] = (result->timestamp >> 16) & 0xFF;
    payload[2] = (result->timestamp >> 8) & 0xFF;
    payload[3] = result->timestamp & 0xFF;
    payload[4] = result->config;

    /* Encode band magnitudes */
    for (int i = 0; i < FFT_BAND_COUNT; i++) {
        payload[FFT_HEADER_SIZE + (i * 2)] = (result->bands[i] >> 8) & 0xFF;
        payload[FFT_HEADER_SIZE + (i * 2) + 1] = result->bands[i] & 0xFF;
    }

    *size = FFT_HEADER_SIZE + (FFT_BAND_COUNT * FFT_BYTES_PER_BAND);
    return 0;
}

int audio_app_decode_fft(const uint8_t *payload, uint8_t size, fft_result_t *result)
{
    if (!payload || !result || size < FFT_HEADER_SIZE) {
        return -EINVAL;
    }

    /* Decode header */
    result->timestamp = ((uint32_t)payload[0] << 24) |
                       ((uint32_t)payload[1] << 16) |
                       ((uint32_t)payload[2] << 8) |
                       payload[3];
    result->config = payload[4];

    /* Decode band magnitudes */
    int band_count = (size - FFT_HEADER_SIZE) / FFT_BYTES_PER_BAND;
    if (band_count > FFT_BAND_COUNT) {
        band_count = FFT_BAND_COUNT;
    }

    for (int i = 0; i < band_count; i++) {
        result->bands[i] = ((uint16_t)payload[FFT_HEADER_SIZE + (i * 2)] << 8) |
                          payload[FFT_HEADER_SIZE + (i * 2) + 1];
    }

    return 0;
}

int audio_app_config(const audio_config_t *config)
{
    if (!config) {
        return -EINVAL;
    }

    /* Validate configuration */
    if (config->duration > AUDIO_MAX_DURATION) {
        return -EINVAL;
    }

    memcpy(&audio_state.config, config, sizeof(audio_config_t));
    return 0;
}

int audio_app_get_config(audio_config_t *config)
{
    if (!config) {
        return -EINVAL;
    }

    memcpy(config, &audio_state.config, sizeof(audio_config_t));
    return 0;
}

bool audio_app_busy(void)
{
    return audio_state.busy;
}

int audio_app_start(bool start, MEASUREMENT_SOURCE_e source)
{
    if (start && !audio_state.busy) {
        /* Configure I2S */
        struct i2s_config i2s_cfg = {
            .word_size = AUDIO_BITS_PER_SAMPLE,
            .channels = 1,
            .format = I2S_FMT_DATA_FORMAT_I2S,
            .options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
            .frame_clk_freq = AUDIO_SAMPLE_RATE,
            .mem_slab = NULL,
            .block_size = AUDIO_FRAME_SIZE * sizeof(int16_t),
            .timeout = K_MSEC(200)
        };
        
        int ret = i2s_configure(audio_state.i2s_dev, I2S_DIR_RX, &i2s_cfg);
        if (ret < 0) {
            return ret;
        }

        /* Start I2S */
        ret = i2s_trigger(audio_state.i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START);
        if (ret < 0) {
            return ret;
        }

        audio_state.busy = true;
        audio_state.samples_collected = 0;

        /* Schedule stop after configured duration */
        k_work_schedule(&audio_state.stop_work, 
                       K_SECONDS(audio_state.config.duration));

    } else if (!start && audio_state.busy) {
        /* Stop I2S */
        i2s_trigger(audio_state.i2s_dev, I2S_DIR_RX, I2S_TRIGGER_STOP);
        audio_state.busy = false;
        k_work_cancel_delayable(&audio_state.stop_work);
    }

    return 0;
}
