/* Previous includes remain */
#include "comm_mgr.h"

/* Previous thread and configuration definitions remain */

/* Add communication configuration */
static const COMM_CONFIG_s comm_config = {
    .method = COMM_METHOD_AUTO,
    .auto_fallback = true,
    .retry_count = 3,
    .retry_interval = 60
};

/* Previous thread implementations remain */

/* Update data thread to use communication manager */
static void data_thread(void *p1, void *p2, void *p3)
{
    MEASUREMENT_RESULT_s result;
    COMM_STATUS_s comm_status;
    int ret;

    while (1) {
        /* Wait for measurement data */
        if (k_msgq_get(&measurement_msgq, &result, K_FOREVER) == 0) {
            /* Try to send measurement */
            ret = comm_mgr_send_measurement(&result);
            if (ret < 0 && ret != -EAGAIN) {
                LOG_ERR("Failed to send measurement: %d", ret);
            }

            /* Get communication status for debugging */
            if (comm_mgr_get_status(&comm_status) == 0) {
                DEBUG_DBG(DEBUG_CAT_COMM, "Active method: %d, LoRa: %d/%ddBm, Cell: %d/%ddBm",
                         comm_status.active_method,
                         comm_status.lorawan_available,
                         comm_status.lorawan_rssi,
                         comm_status.cellular_available,
                         comm_status.cellular_rssi);
            }
        }
    }
}

/* Update BLE command handling */
static void ble_config(const uint8_t *data, uint16_t len)
{
    if (len < 2) {
        return;
    }

    BEEP_CID cmd = data[0];
    switch (cmd) {
        /* Previous cases remain */

        case READ_CELLULAR_CONFIG:
            {
                cellular_config_t cell_cfg;
                if (cellular_app_get_config(&cell_cfg) == 0 && callbacks && callbacks->measurement) {
                    callbacks->measurement((const MEASUREMENT_RESULT_s *)&cell_cfg);
                }
            }
            break;

        case WRITE_CELLULAR_CONFIG:
            if (len >= sizeof(cellular_config_t)) {
                cellular_config_t cell_cfg;
                memcpy(&cell_cfg, &data[1], sizeof(cell_cfg));
                cellular_app_config(&cell_cfg);
                flash_fs_store_config("cell_cfg", &cell_cfg, sizeof(cell_cfg));
            }
            break;

        case READ_CELLULAR_STATUS:
            {
                COMM_STATUS_s status;
                if (comm_mgr_get_status(&status) == 0 && callbacks && callbacks->measurement) {
                    callbacks->measurement((const MEASUREMENT_RESULT_s *)&status);
                }
            }
            break;

        case READ_CELLULAR_SIGNAL:
            {
                int8_t rssi;
                if (comm_mgr_get_signal_strength(COMM_METHOD_CELLULAR, &rssi) == 0 && 
                    callbacks && callbacks->measurement) {
                    callbacks->measurement((const MEASUREMENT_RESULT_s *)&rssi);
                }
            }
            break;

        case READ_COMM_METHOD:
            {
                COMM_CONFIG_s cfg;
                if (comm_mgr_get_config(&cfg) == 0 && callbacks && callbacks->measurement) {
                    callbacks->measurement((const MEASUREMENT_RESULT_s *)&cfg);
                }
            }
            break;

        case WRITE_COMM_METHOD:
            if (len >= sizeof(COMM_CONFIG_s)) {
                COMM_CONFIG_s cfg;
                memcpy(&cfg, &data[1], sizeof(cfg));
                comm_mgr_configure(&cfg);
                flash_fs_store_config("comm_cfg", &cfg, sizeof(cfg));
            }
            break;

        default:
            break;
    }
}

/* Update power management */
static void prepare_for_sleep(void)
{
    /* Power down communication interfaces */
    comm_mgr_power_down();
    
    /* Previous power down steps remain */
}

static void wake_from_sleep(void)
{
    /* Previous wake-up steps remain */
    
    /* Power up communication interfaces */
    comm_mgr_power_up();
}

int main(void)
{
    int ret;

    LOG_INF("BEEP Base Firmware v%d.%d.%d", 
            FIRMWARE_MAJOR, FIRMWARE_MINOR, FIRMWARE_SUB);

    /* Initialize debug first */
    DEBUG_INIT();

    /* Initialize power management */
    ret = power_mgmt_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize power management: %d", ret);
        return ret;
    }

    /* Initialize flash filesystem */
    ret = flash_fs_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize flash filesystem: %d", ret);
        return ret;
    }

    /* Initialize communication manager */
    ret = comm_mgr_init(&comm_config);
    if (ret < 0) {
        LOG_ERR("Failed to initialize communication manager: %d", ret);
        return ret;
    }

    /* Initialize RTC */
    ret = rtc_app_init(rtc_alarm_handler, rtc_time_handler);
    if (ret < 0) {
        LOG_ERR("Failed to initialize RTC: %d", ret);
        return ret;
    }

    /* Initialize alarm system */
    ret = alarm_app_init(alarm_handler);
    if (ret < 0) {
        LOG_ERR("Failed to initialize alarm system: %d", ret);
        return ret;
    }

    /* Initialize BLE */
    ret = ble_app_init(&ble_cb);
    if (ret < 0) {
        LOG_ERR("Failed to initialize BLE: %d", ret);
        return ret;
    }

    /* Start BLE advertising */
    ret = ble_app_start_adv();
    if (ret < 0) {
        LOG_ERR("Failed to start BLE advertising: %d", ret);
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

    k_thread_create(&storage_thread_data, storage_stack,
                   K_THREAD_STACK_SIZEOF(storage_stack),
                   storage_thread, NULL, NULL, NULL,
                   STORAGE_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&storage_thread_data, "storage");

    /* Enable systems */
    alarm_app_enable(true);
    power_mgmt_auto_sleep(true);

    /* Set up daily measurement schedule */
    struct tm current_time;
    rtc_app_get_time(&current_time);
    current_time.tm_hour = measurement_schedule.hour;
    current_time.tm_min = measurement_schedule.minute;
    current_time.tm_sec = 0;
    rtc_app_set_alarm(2, &current_time);
    rtc_app_enable_alarm(2, true);

    /* Notify activity to start sleep timer */
    power_mgmt_notify_activity();

    return 0;
}
