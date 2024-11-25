#ifndef AUDIO_APP_H
#define AUDIO_APP_H

#include <zephyr/kernel.h>
#include "beep_types.h"
#include "beep_protocol.h"

#define AUDIO_APP_LOG_ENABLED               0
#define BLOCKS_TO_TRANSFER                  2
#define I2S_DATA_BLOCK_WORDS                1024UL
#define FFT_COMPLEX_INPUT                   (I2S_DATA_BLOCK_WORDS * 2UL)
#define FFT_OUTPUT_SIZE                     (I2S_DATA_BLOCK_WORDS / 2UL)
#define MCLK_FREQ_HZ                        (32E6/31.0)
#define FFT_TEST_SAMPLE_FREQ_HZ             (MCLK_FREQ_HZ/128.0f)
#define FFT_TEST_SAMPLE_RES_HZ              (FFT_TEST_SAMPLE_FREQ_HZ/FFT_COMPLEX_INPUT)

typedef enum {
    AUDIO_IDLE,
    AUDIO_START,
    AUDIO_SAMPLING,
    AUDIO_PROCESS,
    AUDIO_STOP,
} AUDIO_STATES;

typedef struct {
    AUDIO_STATES            state;
    uint32_t                timestampStateChanged;
    uint16_t volatile       blocksTransferred;
    uint16_t                blocksOffset;
    bool                    loop;
    measurement_callback    callback;
    CONTROL_SOURCE         source;
} AUDIO_APPLICATIONs;

/**
 * @brief Get the current audio processing result
 *
 * @param result Pointer to store the measurement result
 * @return int 0 on success, negative errno on failure
 */
int audio_app_get_result(MEASUREMENT_RESULT_s *result);

/**
 * @brief Check if audio processing is busy
 *
 * @return true if busy
 * @return false if idle
 */
bool audio_app_busy(void);

/**
 * @brief Check if audio subsystem can sleep
 *
 * @return true if can sleep
 * @return false if must stay awake
 */
bool audio_app_sleep(void);

/**
 * @brief Start or stop audio processing
 *
 * @param en true to start, false to stop
 * @param source Control source initiating the action
 * @return int 0 on success, negative errno on failure
 */
int audio_app_start(const bool en, CONTROL_SOURCE source);

/**
 * @brief Initialize the audio subsystem
 *
 * @param measurement_handler Callback for measurement results
 */
void audio_app_init(measurement_callback measurement_handler);

#endif /* AUDIO_APP_H */
