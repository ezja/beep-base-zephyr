# Troubleshooting Guide

This guide helps diagnose and resolve common issues in the BEEP Base firmware.

## Build Issues

### Missing Dependencies
```
Error: west: command not found
```
**Solution:**
```bash
pip3 install --user west
export PATH=$HOME/.local/bin:$PATH
```

### Compilation Errors
```
Error: 'CONFIG_NRF_MODEM_LIB' undeclared
```
**Solution:**
1. Check Kconfig:
```bash
west build -t menuconfig
# Enable: Networking -> nRF Modem Library
```

2. Verify west.yml:
```yaml
manifest:
  projects:
    - name: nrf
      url: https://github.com/nrfconnect/sdk-nrf
      revision: v2.4.0
```

### Linker Errors
```
undefined reference to `nrf_modem_lib_init'
```
**Solution:**
```cmake
# Add to CMakeLists.txt
target_link_libraries(app PRIVATE
    nrf_modem_lib
)
```

## Communication Issues

### Cellular Connection

#### No Network Registration
```
LOG_ERR: Network registration failed
```
**Diagnostic Steps:**
1. Check signal strength:
```bash
# Enable modem traces
west build -t menuconfig
# Enable: Modem -> Modem trace
```

2. Verify modem firmware:
```bash
nrfjprog --memrd 0x00 --n 64
```

3. Check configuration:
```c
cellular_config_t config;
cellular_app_get_config(&config);
printf("APN: %s, Band: %d\n", config.apn, config.band);
```

**Solutions:**
1. Update modem firmware:
```bash
./scripts/build.sh -m new_modem.hex
```

2. Adjust network parameters:
```c
config.band = 20;  // Try different bands
config.mode = LTE_MODE_M1;
cellular_app_config(&config);
```

### LoRaWAN Issues

#### Join Failed
```
LOG_ERR: OTAA join failed
```
**Diagnostic Steps:**
1. Verify keys:
```bash
./scripts/check_keys.sh
```

2. Check frequency plan:
```bash
west build -t guiconfig
# Verify: LoRaWAN -> Region
```

**Solutions:**
1. Reset join state:
```c
lorawan_app_reset();
```

2. Adjust parameters:
```c
lorawan_config.data_rate = DR_0;
lorawan_config.tx_power = 14;
```

## Power Management

### High Power Consumption

#### No PSM Entry
```
LOG_WRN: Failed to enter PSM
```
**Diagnostic Steps:**
1. Check current consumption:
```bash
./scripts/power_profile.sh --duration 3600
```

2. Monitor power states:
```bash
west debug
(gdb) mon power
```

**Solutions:**
1. Enable power debugging:
```kconfig
CONFIG_PM_DEBUG=y
CONFIG_PM_STATE_STATS=y
```

2. Adjust PSM timing:
```c
psm_config_t psm = {
    .tau_sec = 43200,    // 12 hours
    .active_sec = 60     // 1 minute
};
cellular_app_config_psm(&psm);
```

#### Unexpected Wake-ups
```
LOG_INF: Unexpected wake-up source: 0x04
```
**Diagnostic Steps:**
1. Monitor wake sources:
```bash
./scripts/monitor_wakeup.sh
```

2. Check GPIO configuration:
```bash
west build -t guiconfig
# Verify: GPIO -> Wake-up sources
```

**Solutions:**
1. Disable unused wake sources:
```c
power_mgmt_disable_wakeup(WAKEUP_SOURCE_UART);
```

2. Adjust sensitivity:
```dts
&gpio0 {
    pin-debouncing-interval-ms = <50>;
};
```

## Sensor Issues

### BME280 Not Responding
```
LOG_ERR: BME280 read failed: -5
```
**Diagnostic Steps:**
1. Check I2C bus:
```bash
./scripts/i2c_scan.sh
```

2. Verify power supply:
```bash
./scripts/check_voltage.sh
```

**Solutions:**
1. Reset sensor:
```c
sensor_attr_set(bme280_dev, SENSOR_CHAN_ALL,
                SENSOR_ATTR_RESET, NULL);
```

2. Adjust I2C timing:
```dts
&i2c0 {
    clock-frequency = <100000>;
};
```

### HX711 Readings Unstable
```
LOG_WRN: HX711 readings unstable
```
**Diagnostic Steps:**
1. Monitor raw values:
```bash
./scripts/monitor_adc.sh --channel weight
```

2. Check timing:
```bash
./scripts/measure_timing.sh --peripheral hx711
```

**Solutions:**
1. Increase samples:
```c
hx711_config.samples = 20;
sensor_attr_set(hx711_dev, SENSOR_CHAN_WEIGHT,
                SENSOR_ATTR_SAMPLING_FREQUENCY, &hx711_config);
```

2. Adjust gain:
```c
hx711_config.gain = 64;
sensor_attr_set(hx711_dev, SENSOR_CHAN_WEIGHT,
                SENSOR_ATTR_GAIN, &hx711_config);
```

## Storage Issues

### Flash Write Failed
```
LOG_ERR: Flash write failed: -5
```
**Diagnostic Steps:**
1. Check flash status:
```bash
./scripts/flash_info.sh
```

2. Monitor wear:
```bash
./scripts/check_wear.sh
```

**Solutions:**
1. Erase sector:
```c
flash_erase(flash_dev, addr, size);
```

2. Enable wear leveling:
```kconfig
CONFIG_FS_LITTLEFS_CACHE_SIZE=256
CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE=32
```

## Debug Tools

### Runtime Analysis
```bash
# Enable thread analysis
west build -- -DCONFIG_THREAD_ANALYZER=y

# Monitor stack usage
west build -- -DCONFIG_THREAD_STACK_INFO=y

# Check timing
west build -- -DCONFIG_TIMING_FUNCTIONS=y
```

### Power Profiling
```bash
# Enable power profiling
west build -- -DCONFIG_PM_DEBUG=y

# Monitor consumption
./scripts/power_profile.sh --output power.csv

# Analyze results
./scripts/analyze_power.py power.csv
```

### Communication Debug
```bash
# Monitor cellular
./scripts/monitor_cellular.sh --verbose

# Check LoRaWAN
./scripts/check_lorawan.sh --region EU868

# Test both
./scripts/test_communication.sh --all
```

## Recovery Procedures

### Factory Reset
```bash
# Reset all settings
./scripts/factory_reset.sh

# Reset specific component
./scripts/factory_reset.sh --component cellular
```

### Emergency Recovery
```bash
# Recover bootloader
nrfjprog --recover

# Reset modem
./scripts/reset_modem.sh --hard

# Clear flash
./scripts/clear_flash.sh --all
