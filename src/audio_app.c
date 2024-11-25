#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio_app, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/gpio.h>
#include <arm_math.h>

#include "audio_app.h"
#include "beep_protocol.h"
#include "beep_types.h"

#define AUDIO_STACK_SIZE 4096
#define AUDIO_PRIORITY 5

static K_THREAD_STACK_DEFINE(audio_stack, AUDIO_STACK_SIZE);
static struct k_thread audio_thread;

static const struct device *i2s_dev;
static const struct device *i2c_dev;
static const struct device *tlv_dev;

static uint32_t m_buffer_rx[2][I2S_DATA_BLOCK_WORDS];
static volatile bool fftIsBusy = false;
static float m_fft_output_f32[I2S_DATA_BLOCK_WORDS];
static float m_fft_input_f32[FFT_COMPLEX_INPUT] = {0};

static uint32_t fftSamples[I2S_DATA_BLOCK_WORDS] = {0};
BEEP_protocol_s fft_result;
BEEP_protocol_s settings;

static struct k_mutex audio_mutex;
static struct k_sem audio_sem;

AUDIO_APPLICATIONs audio = {
    .state = AUDIO_IDLE,
};

static void audio_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    struct i2s_config i2s_cfg = {
        .word_size = 32,
        .channels = 1,
        .format = I2S_FMT_DATA_FORMAT_I2S,
        .options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
        .frame_clk_freq = MCLK_FREQ_HZ/128,
        .mem_slab = NULL,
        .block_size = sizeof(uint32_t) * I2S_DATA_BLOCK_WORDS,
        .timeout = K_MSEC(200),
    };

    while (1) {
        k_sem_take(&audio_sem, K_FOREVER);
        
        switch (audio.state) {
            case AUDIO_IDLE:
                k_sleep(K_MSEC(100));
                break;

            case AUDIO_START:
                if (!pm_device_is_powered(tlv_dev)) {
                    pm_device_action_run(tlv_dev, PM_DEVICE_ACTION_RESUME);
                    
                    // Configure I2S
                    if (i2s_configure(i2s_dev, I2S_DIR_RX, &i2s_cfg) == 0) {
                        if (i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START) == 0) {
                            audio.state = AUDIO_SAMPLING;
                            audio.blocksTransferred = 0;
                            fftIsBusy = false;
                        }
                    }
                }
                break;

            case AUDIO_SAMPLING:
                if (!fftIsBusy) {
                    // Read I2S data
                    if (i2s_read(i2s_dev, &m_buffer_rx[0], sizeof(uint32_t) * I2S_DATA_BLOCK_WORDS) == 0) {
                        memcpy(fftSamples, m_buffer_rx[0], sizeof(fftSamples));
                        fftIsBusy = true;
                        audio.blocksTransferred++;

                        // Process FFT
                        arm_cfft_instance_f32 fft_instance;
                        arm_cfft_init_f32(&fft_instance, I2S_DATA_BLOCK_WORDS);
                        
                        // Convert to complex numbers and process FFT
                        for (int i = 0; i < I2S_DATA_BLOCK_WORDS; i++) {
                            m_fft_input_f32[2*i] = (float)((int32_t)fftSamples[i] >> 8);
                            m_fft_input_f32[2*i + 1] = 0.0f;
                        }

                        arm_cfft_f32(&fft_instance, m_fft_input_f32, 0, 1);
                        arm_cmplx_mag_f32(m_fft_input_f32, m_fft_output_f32, I2S_DATA_BLOCK_WORDS);

                        // Create result
                        m_fft_output_f32[0] = 0.0f; // Remove DC
                        fft_create_result(&fft_result,
                                        m_fft_output_f32,
                                        settings.param.audio_config.fft_count,
                                        settings.param.audio_config.fft_start * 2,
                                        settings.param.audio_config.fft_stop * 2,
                                        FFT_OUTPUT_SIZE);

                        if (audio.callback != NULL) {
                            fft_result.param.meas_result.source = audio.source;
                            audio.callback(&fft_result.param.meas_result);
                        }

                        fftIsBusy = false;

                        // Check if we need to stop
                        if (audio.blocksTransferred >= audio.blocksOffset && !audio.loop) {
                            i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_STOP);
                            audio.state = AUDIO_STOP;
                        }
                    }
                }
                break;

            case AUDIO_STOP:
                pm_device_action_run(tlv_dev, PM_DEVICE_ACTION_SUSPEND);
                audio.state = AUDIO_IDLE;
                break;

            default:
                break;
        }
    }
}

bool audio_app_busy(void)
{
    #if !TLV_ENABLE
        return false;
    #else
        return (audio.state != AUDIO_IDLE);
    #endif
}

bool audio_app_sleep(void)
{
    #if !TLV_ENABLE
        return false;
    #else
        return ((audio.state == AUDIO_IDLE && !fftIsBusy) || (audio.state == AUDIO_SAMPLING)) ? false : true;
    #endif
}

uint32_t audio_app_get_result(MEASUREMENT_RESULT_s * result)
{
    if (result == NULL) {
        return -EINVAL;
    }

    k_mutex_lock(&audio_mutex, K_FOREVER);
    memset(result, 0, sizeof(MEASUREMENT_RESULT_s));
    memcpy(result, &fft_result.param.meas_result, sizeof(MEASUREMENT_RESULT_s));
    k_mutex_unlock(&audio_mutex);

    return 0;
}

uint32_t audio_app_start(const bool en, CONTROL_SOURCE source)
{
    #if !TLV_ENABLE
        return -ENOTSUP;
    #else
        if (en) {
            if (audio.state == AUDIO_IDLE) {
                audio.source = source;
                audio.state = AUDIO_START;
                k_sem_give(&audio_sem);
                return 0;
            }
            return -EBUSY;
        } else {
            if (audio.state != AUDIO_IDLE) {
                audio.state = AUDIO_STOP;
                k_sem_give(&audio_sem);
                return 0;
            }
            return -EALREADY;
        }
    #endif
}

void audio_app_init(measurement_callback measurement_handler)
{
    k_mutex_init(&audio_mutex);
    k_sem_init(&audio_sem, 0, 1);

    i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s0));
    i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    tlv_dev = DEVICE_DT_GET(DT_NODELABEL(tlv320adc3100));

    if (!device_is_ready(i2s_dev) || !device_is_ready(i2c_dev) || !device_is_ready(tlv_dev)) {
        LOG_ERR("Audio devices not ready");
        return;
    }

    audio.state = AUDIO_IDLE;
    audio.loop = false;
    audio.callback = measurement_handler;
    audio.blocksOffset = BLOCKS_TO_TRANSFER;

    k_thread_create(&audio_thread,
                   audio_stack,
                   AUDIO_STACK_SIZE,
                   audio_thread_entry,
                   NULL, NULL, NULL,
                   AUDIO_PRIORITY,
                   0,
                   K_NO_WAIT);
    k_thread_name_set(&audio_thread, "audio");
}
