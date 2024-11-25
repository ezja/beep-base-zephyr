#ifndef BEEP_TYPES_H
#define BEEP_TYPES_H

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* Firmware and Hardware Versions */
#define FIRMWARE_MAJOR              1
#define FIRMWARE_MINOR              6
#define FIRMWARE_SUB               0
#define FIRMWARE_TO_UINT32_T(major, minor, sub) ((((uint32_t)major) << 16) | (((uint32_t)minor) << 8) | (((uint32_t)sub) << 0))

#define HARDWARE_MAJOR             1
#define HARDWARE_MINOR             0
#define HARDWARE_ID                190222

/* System Constants */
#define PIN_CODE_BLE_LENGTH        6
#define PIN_CODE_LENGTH_MIN        7
#define PIN_CODE_LENGTH_MAX        16
#define PIN_CODE_DEFAULT           "123456"

#define MAX_TEMP_SENSORS           10
#define BEEP_MAX_LENGTH            30
#define BEEP_LORAWAN_MAX_LENGTH    52
#define BEEP_MIN_LENGTH            1
#define ATECC_ID_LENGTH            9
#define FFT_MAX_BINS               12
#define HX711_N_CHANNELS           3

/* Command Identifiers */
#define CID_WRITE                  0x80
#define CID_READ                   0x00

typedef enum {
    INTERNAL_SOURCE,
    BLE_SOURCE,
    LORAWAN_SOURCE,
    UNKNOWN_SOURCE,
} CONTROL_SOURCE;

typedef enum {
    AIN_IN3LM    = 0,
    AIN_IN2LP    = 1,
    AIN_IN2RP    = 2,
    AIN_OFF      = 3,
} AUDIO_INPUTe;

typedef enum {
    DS18B20     = 0,
    BME280      = 1,
    HX711       = 2,
    AUDIO_ADC   = 3,
    nRF_ADC     = 4,
    SQ_MIN      = 5,
    ATECC       = 6,
    BUZZER      = 7,
    LORAWAN     = 8,
    MX_FLASH    = 9,
    nRF_FLASH   = 10,
    APPLICATIE  = 11,
} SENSOR_TYPE;

typedef struct {
    uint8_t     bins;
    uint8_t     start;
    uint8_t     stop;
    uint16_t    values[FFT_MAX_BINS];
} FFT_RESULTS;

typedef struct {
    uint8_t     devices;
    int16_t     temperatures[MAX_TEMP_SENSORS];
} DS18B20_RESULTS_s;

typedef struct {
    uint16_t    humidity;
    int16_t     temperature;
    uint16_t    airPressure;
} BME280_RESULT_s;

typedef struct {
    uint8_t     channel;
    uint16_t    samples;
    int32_t     value[HX711_N_CHANNELS];
} HX711_CONV_s;

typedef struct {
    int16_t     battADC;
    uint16_t    battVoltage_mV;
    int16_t     vccADC;
    uint16_t    vccVoltage_mV;
    uint8_t     battPercentage;
} ADC_s;

typedef struct {
    SENSOR_TYPE     type;
    CONTROL_SOURCE  source;
    union {
        DS18B20_RESULTS_s      ds18B20;
        BME280_RESULT_s        bme280;
        ADC_s                  saadc;  
        HX711_CONV_s          hx711;
        FFT_RESULTS           fft;
    } result;
} MEASUREMENT_RESULT_s;

typedef void (*measurement_callback)(MEASUREMENT_RESULT_s *result);
typedef void (*HX711_callback)(HX711_CONV_s *result);

/* Audio Configuration */
typedef struct {
    uint8_t     channel;
    uint8_t     gain;
    int8_t      volume;
    uint8_t     fft_count;
    uint8_t     fft_start;
    uint8_t     fft_stop;    
    bool        min6dB;    
} AUDIO_CONFIG_s;

/* Alarm Thresholds */
typedef struct {
    int16_t     Max;
    int16_t     Min;
    uint16_t    Diff;
} DS_ALARM_s;

typedef struct {
    uint16_t    Min;
    uint16_t    Max;
    uint16_t    Diff;
} SUPPLY_ALARM_s;

typedef struct {
    int32_t     Max;
    int32_t     Min;
    uint32_t    Diff;    
} HX711_ALARM_s;

typedef struct {
    int16_t     Temp_Max;
    int16_t     Temp_Min;
    uint16_t    Temp_Diff;
    uint16_t    humidity_Max;
    uint16_t    humidity_Min;
    uint16_t    humidity_Diff;
    uint16_t    press_Max;
    uint16_t    press_Min;
    uint16_t    press_Diff;
} BME_ALARM_s;

typedef struct {
    SENSOR_TYPE  type;
    union {
        DS_ALARM_s      ds;
        SUPPLY_ALARM_s  supply;
        HX711_ALARM_s   hx;
        BME_ALARM_s     bme;
    } thr;
} ALARM_CONFIG_s;

#endif /* BEEP_TYPES_H */
