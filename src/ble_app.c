#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include "ble_app.h"
#include "beep_protocol.h"
#include "rtc_app.h"
#include "flash_fs.h"

LOG_MODULE_REGISTER(ble_app, CONFIG_APP_LOG_LEVEL);

/* System status */
static uint8_t system_status;
static struct bt_conn *current_conn;
static const struct ble_callbacks *callbacks;
static uint8_t measurement_notify_enabled;
static uint8_t control_indicate_enabled;

/* Command response buffer */
static uint8_t response_buffer[256];

/* Command handlers */
static int handle_read_fw_version(uint8_t *response, uint16_t *len)
{
    response[0] = FIRMWARE_MAJOR;
    response[1] = FIRMWARE_MINOR;
    response[2] = FIRMWARE_SUB;
    *len = 3;
    return 0;
}

static int handle_read_status(uint8_t *response, uint16_t *len)
{
    response[0] = system_status;
    *len = 1;
    return 0;
}

static int handle_write_status(const uint8_t *data, uint16_t len)
{
    if (len < 1) {
        return -EINVAL;
    }
    system_status = data[0];
    return 0;
}

static int handle_read_battery(uint8_t *response, uint16_t *len)
{
    /* TODO: Implement battery reading */
    response[0] = 100; /* Placeholder: 100% */
    *len = 1;
    return 0;
}

static int handle_read_temperature(uint8_t *response, uint16_t *len)
{
    int16_t temp;
    int ret = rtc_app_get_temperature(&temp);
    if (ret == 0) {
        memcpy(response, &temp, sizeof(temp));
        *len = sizeof(temp);
    }
    return ret;
}

static int handle_read_rtc_time(uint8_t *response, uint16_t *len)
{
    struct tm time;
    int ret = rtc_app_get_time(&time);
    if (ret == 0) {
        uint32_t timestamp = rtc_app_tm_to_timestamp(&time);
        memcpy(response, &timestamp, sizeof(timestamp));
        *len = sizeof(timestamp);
    }
    return ret;
}

static int handle_write_rtc_time(const uint8_t *data, uint16_t len)
{
    if (len < sizeof(uint32_t)) {
        return -EINVAL;
    }
    uint32_t timestamp;
    struct tm time;
    memcpy(&timestamp, data, sizeof(timestamp));
    rtc_app_timestamp_to_tm(timestamp, &time);
    return rtc_app_set_time(&time);
}

static int handle_read_storage_info(uint8_t *response, uint16_t *len)
{
    size_t total, used;
    int ret = flash_fs_get_stats(&total, &used);
    if (ret == 0) {
        memcpy(response, &total, sizeof(total));
        memcpy(response + sizeof(total), &used, sizeof(used));
        *len = sizeof(total) + sizeof(used);
    }
    return ret;
}

static int handle_read_measurement_data(uint8_t *response, uint16_t *len)
{
    uint32_t index;
    MEASUREMENT_RESULT_s result;
    
    if (*len < sizeof(uint32_t)) {
        return -EINVAL;
    }
    
    memcpy(&index, response, sizeof(index));
    int ret = flash_fs_read_measurement(index, &result);
    if (ret == 0) {
        memcpy(response, &result, sizeof(result));
        *len = sizeof(result);
    }
    return ret;
}

static int handle_clear_measurement_data(void)
{
    return flash_fs_clear_measurements();
}

static int handle_system_reset(void)
{
    /* Schedule system reset */
    k_work_schedule(NULL, K_MSEC(100));
    return 0;
}

/* GATT write callback */
static ssize_t ble_write_callback(struct bt_conn *conn,
                                const struct bt_gatt_attr *attr,
                                const void *buf, uint16_t len,
                                uint16_t offset, uint8_t flags)
{
    const uint8_t *data = buf;
    uint16_t response_len = sizeof(response_buffer);
    int ret = 0;

    if (len < 1) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    BEEP_CID cmd = data[0];
    data++; /* Skip command byte */
    len--;

    switch (cmd) {
        case READ_FW_VERSION:
            ret = handle_read_fw_version(response_buffer, &response_len);
            break;
        case READ_STATUS:
            ret = handle_read_status(response_buffer, &response_len);
            break;
        case WRITE_STATUS:
            ret = handle_write_status(data, len);
            break;
        case READ_BATTERY:
            ret = handle_read_battery(response_buffer, &response_len);
            break;
        case READ_TEMPERATURE:
            ret = handle_read_temperature(response_buffer, &response_len);
            break;
        case READ_RTC_TIME:
            ret = handle_read_rtc_time(response_buffer, &response_len);
            break;
        case WRITE_RTC_TIME:
            ret = handle_write_rtc_time(data, len);
            break;
        case READ_STORAGE_INFO:
            ret = handle_read_storage_info(response_buffer, &response_len);
            break;
        case READ_MEASUREMENT_DATA:
            ret = handle_read_measurement_data(response_buffer, &response_len);
            break;
        case CLEAR_MEASUREMENT_DATA:
            ret = handle_clear_measurement_data();
            break;
        case SYSTEM_RESET:
            ret = handle_system_reset();
            break;
        default:
            if (callbacks && callbacks->control) {
                callbacks->control(cmd, data, len);
            }
            break;
    }

    if (ret < 0) {
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    }

    /* Send response if available */
    if (response_len > 0) {
        bt_gatt_notify(conn, attr, response_buffer, response_len);
    }

    return len;
}

/* Rest of BLE implementation remains the same */
/* ... (Previous BLE implementation from earlier) ... */
