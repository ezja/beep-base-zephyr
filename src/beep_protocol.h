#ifndef BEEP_PROTOCOL_H
#define BEEP_PROTOCOL_H

#include <zephyr/kernel.h>
#include "beep_types.h"

/* Protocol Command IDs */
typedef enum {
    RESPONSE = 0,
    READ_FIRMWARE_VERSION = 1,
    READ_HARDWARE_VERSION = 2,
    READ_DS18B20_STATE = 3,
    WRITE_DS18B20_STATE = CID_WRITE | READ_DS18B20_STATE,
    READ_DS18B20_CONVERSION = 4,
    WRITE_DS18B20_CONVERSION = CID_WRITE | READ_DS18B20_CONVERSION,
    READ_DS18B20_CONFIG = 5,
    BME280_CONFIG_READ = 6,
    BME280_CONFIG_WRITE = CID_WRITE | BME280_CONFIG_READ,
    BME280_CONVERSION_READ = 7,
    BME280_CONVERSION_START = CID_WRITE | BME280_CONVERSION_READ,
    READ_BME280_I2C = 8,
    READ_HX711_STATE = 9,
    WRITE_HX711_STATE = CID_WRITE | READ_HX711_STATE,
    READ_HX711_CONVERSION = 10,
    WRITE_HX711_CONVERSION = CID_WRITE | READ_HX711_CONVERSION,
    READ_AUDIO_ADC_CONFIG = 11,
    WRITE_AUDIO_ADC_CONFIG = CID_WRITE | READ_AUDIO_ADC_CONFIG,
    READ_AUDIO_ADC_CONVERSION = 12,
    START_AUDIO_ADC_CONVERSION = 13,
    READ_ATECC_READ_ID = 14,
    READ_ATECC_I2C = 15,
    READ_BUZZER_STATE = 16,
    WRITE_BUZZER_DEFAULT_TUNE = 17 | CID_WRITE,
    WRITE_BUZZER_CUSTOM_TUNE = 18 | CID_WRITE,
    READ_SQ_MIN_STATE = 19,
    WRITE_SQ_MIN_STATE = READ_SQ_MIN_STATE | CID_WRITE,
    READ_LORAWAN_STATE = 20,
    WRITE_LORAWAN_STATE = CID_WRITE | READ_LORAWAN_STATE,
    READ_LORAWAN_DEVEUI = 21,
    WRITE_LORAWAN_DEVEUI = CID_WRITE | READ_LORAWAN_DEVEUI,
    READ_LORAWAN_APPEUI = 22,
    WRITE_LORAWAN_APPEUI = CID_WRITE | READ_LORAWAN_APPEUI,
    READ_LORAWAN_APPKEY = 23,
    WRITE_LORAWAN_APPKEY = CID_WRITE | READ_LORAWAN_APPKEY,
    WRITE_LORAWAN_TRANSMIT = CID_WRITE | 24,
    READ_CID_nRF_FLASH = 25,
    READ_nRF_ADC_CONFIG = 26,
    READ_nRF_ADC_CONVERSION = 27,
    WRITE_nRF_ADC_CONVERSION = CID_WRITE | READ_nRF_ADC_CONVERSION,
    READ_APPLICATION_STATE = 28,
    READ_APPLICATION_CONFIG = 29,
    WRITE_APPLICATION_CONFIG = CID_WRITE | READ_APPLICATION_CONFIG,
    READ_PINCODE = 30,
    WRITE_PINCODE = CID_WRITE | READ_PINCODE,
    READ_BOOT_COUNT = 31,
    WRITE_BOOT_COUNT = CID_WRITE | READ_BOOT_COUNT,
    READ_MX_FLASH = 32,
    ERASE_MX_FLASH = 33,
    SIZE_MX_FLASH = 34,
    ALARM_CONFIG_READ = 35,
    ALARM_CONFIG_WRITE = CID_WRITE | ALARM_CONFIG_READ,
    ALARM_STATUS_READ = 36,
    READ_TIME = 37,
    WRITE_TIME = CID_WRITE | READ_TIME,
    READ_REED_STATE = 38,
    WRITE_REED_STATE = CID_WRITE | READ_REED_STATE,
    READ_ON_STATE = 39,
    WRITE_ON_STATE = READ_ON_STATE | CID_WRITE,
    READ_LOG_OFFSET = 40,
    WRITE_LOG_OFFSET = READ_LOG_OFFSET | CID_WRITE,
    START_FILL_LOG = 41,
    STOP_FILL_LOG = 42,
    RESET_REASON = 43,
    LORAWAN_OTAA_COMPLETE = 44,
    READ_TIME_RTC = 45,
    CID_UNKNOWN,
} BEEP_CID;

/* Protocol Status Codes */
typedef enum {
    BEEP_SENSOR_OFF = 1,
    BEEP_SENSOR_ON = 2,
    BEEP_KEEP_ALIVE = 3,
    BEEP_ALARM = 4,
    BEEP_BLE_CUSTOM = 5,
    BEEP_DOWNLINK_RESPONSE = 6,
    BEEP_TIME_CHANGE = 7,
    BEEP_LOG_FILL = 8,
    BEEP_UNKNOWN = 0xFF,
} BEEP_STATUS;

/* Protocol Parameters Union */
typedef union {
    MEASUREMENT_RESULT_s meas_result;
    AUDIO_CONFIG_s audio_config;
    ALARM_CONFIG_s alarm_config;
    uint8_t raw[BEEP_MAX_LENGTH];
} BEEP_parameters_u;

/* Main Protocol Structure */
typedef struct {
    BEEP_CID command;
    BEEP_parameters_u param;
} BEEP_protocol_s;

/* Response Structure */
typedef struct {
    BEEP_CID ErrorCmd;
    uint32_t errorCode;
} BEEP_response_s;

/* Version Structure */
typedef struct {
    uint16_t major;
    uint16_t minor;
    uint16_t sub;
    uint32_t id;
} BEEP_version_s;

#endif /* BEEP_PROTOCOL_H */