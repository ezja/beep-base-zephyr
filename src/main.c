#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#incluxxde <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "audio_app.h"
#include "beep_protocol.h"
#include "beep_types.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* Thread stack sizes */
#define AUDIO_STACK_SIZE 4096
#define SENSOR_STACK_SIZE 2048
#define COMM_STACK_SIZE 2048

/* Thread priorities */
#define AUDIO_PRIORITY 5
#define SENSOR_PRIORITY 6
#define COMM_PRIORITY 7

/* Thread stacks */
K_THREAD_STACK_DEFINE(audio_stack, AUDIO_STACK_SIZE);
K_THREAD_STACK_DEFINE(sensor_stack, SENSOR_STACK_SIZE);
K_THREAD_STACK_DEFINE(comm_stack, COMM_STACK_SIZE);

/* Thread data */
static struct k_thread audio_thread_data;
static struct k_thread sensor_thread_data;
static struct k_thread comm_thread_data;

/* Measurement callback for audio processing */
static void measurement_handler(MEASUREMENT_RESULT_s *result)
{
    if (result->type == AUDIO_ADC) {
        LOG_INF("FFT Result - Bins: %u, Start: %u Hz, Stop: %u Hz",
                result->result.fft.bins,
                result->result.fft.start * FFT_TEST_SAMPLE_RES_HZ,
                result->result.fft.stop * FFT_TEST_SAMPLE_RES_HZ);
        
        for (int i = 0; i < result->result.fft.bins; i++) {
            LOG_INF("Bin[%d]: %u", i, result->result.fft.values[i]);
        }
    }
}

/* Audio processing thread */
static void audio_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    /* Initialize audio subsystem */
    audio_app_init(measurement_handler);

    while (1) {
        /* Start audio sampling */
        audio_app_start(true, INTERNAL_SOURCE);

        /* Wait for processing to complete */
        while (audio_app_busy()) {
            k_sleep(K_MSEC(100));
        }

        /* Wait before next sample */
        k_sleep(K_SECONDS(60));
    }
}

/* Sensor monitoring thread */
static void sensor_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while (1) {
        /* TODO: Implement environmental and weight monitoring */
        k_sleep(K_SECONDS(60));
    }
}

/* Communication thread */
static void comm_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while (1) {
        /* TODO: Implement BLE and LoRaWAN communication */
        k_sleep(K_SECONDS(60));
    }
}

int main(void)
{
    int ret;

    LOG_INF("BEEP Base Firmware Starting...");
    LOG_INF("Version %d.%d.%d", FIRMWARE_MAJOR, FIRMWARE_MINOR, FIRMWARE_SUB);

    /* Create threads for different subsystems */
    k_thread_create(&audio_thread_data, audio_stack,
                   K_THREAD_STACK_SIZEOF(audio_stack),
                   audio_thread, NULL, NULL, NULL,
                   AUDIO_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&audio_thread_data, "audio");

    k_thread_create(&sensor_thread_data, sensor_stack,
                   K_THREAD_STACK_SIZEOF(sensor_stack),
                   sensor_thread, NULL, NULL, NULL,
                   SENSOR_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&sensor_thread_data, "sensor");

    k_thread_create(&comm_thread_data, comm_stack,
                   K_THREAD_STACK_SIZEOF(comm_stack),
                   comm_thread, NULL, NULL, NULL,
                   COMM_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&comm_thread_data, "comm");

    /* Main thread can now go idle */
    while (1) {
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
