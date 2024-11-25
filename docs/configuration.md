# Configuration Guide

This document details all configuration options available in the BEEP Base firmware.

## Core Configuration

### Kconfig Options

```kconfig
# Core System
CONFIG_BEEP_BASE=y
CONFIG_VERSION_MAJOR=1
CONFIG_VERSION_MINOR=0
CONFIG_VERSION_SUB=0

# Memory Configuration
CONFIG_HEAP_MEM_POOL_SIZE=8192
CONFIG_MAIN_STACK_SIZE=2048
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2048

# Debug Options
CONFIG_DEBUG=y
CONFIG_DEBUG_THREAD_INFO=y
CONFIG_THREAD_RUNTIME_STATS=y
```

### Device Tree Configuration

```dts
/* boards/nrf52840dk_nrf52840.overlay */

/ {
    chosen {
        zephyr,console = &uart0;
        zephyr,shell-uart = &uart0;
        zephyr,audio-in = &i2s0;
    };
};
```

## Communication Configuration

### LoRaWAN Settings
```kconfig
# LoRaWAN Configuration
CONFIG_LORAWAN=y
CONFIG_LORAWAN_REGION_EU868=y
CONFIG_NET_L2_LORAWAN=y

# Network Parameters
CONFIG_LORAWAN_DATARATE=DR_0
CONFIG_LORAWAN_TX_POWER=14
CONFIG_LORAWAN_ADR=y
```

### Cellular Settings
```kconfig
# Cellular (nRF9161)
CONFIG_NRF_MODEM_LIB=y
CONFIG_LTE_LINK_CONTROL=y
CONFIG_LTE_NETWORK_MODE_LTE_M=y
CONFIG_LTE_PSM_REQ=y
CONFIG_LTE_EDRX_REQ=y
CONFIG_LTE_RAI_REQ=y

# Power Saving Parameters
CONFIG_LTE_PSM_REQ_RPTAU="10100101"  # 12 hours
CONFIG_LTE_PSM_REQ_RAT="00000000"    # 0 seconds
CONFIG_LTE_EDRX_REQ_VALUE_LTE_M="0101"  # 81.92 seconds
```

### BLE Configuration
```kconfig
# Bluetooth Settings
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_DEVICE_NAME="BEEP-BASE"
CONFIG_BT_MAX_CONN=1
CONFIG_BT_MAX_PAIRED=1
```

## Sensor Configuration

### Environmental Sensors
```kconfig
# BME280 Configuration
CONFIG_BME280=y
CONFIG_BME280_MODE_FORCED=y
CONFIG_BME280_TEMP_OVER_2X=y
CONFIG_BME280_PRESS_OVER_16X=y
CONFIG_BME280_HUMID_OVER_1X=y

# DS18B20 Configuration
CONFIG_W1=y
CONFIG_W1_GPIO=y
CONFIG_DS18B20=y
CONFIG_DS18B20_RESOLUTION=12
```

### Weight Sensor
```kconfig
# HX711 Configuration
CONFIG_HX711=y
CONFIG_HX711_GAIN_128=y
CONFIG_HX711_SAMPLES=10
```

### Audio Configuration
```kconfig
# Audio Settings
CONFIG_I2S=y
CONFIG_TLV320ADC3100=y
CONFIG_AUDIO_SAMPLE_RATE_16000=y
CONFIG_AUDIO_FRAME_SIZE_MS=20
```

## Power Management

### Sleep Modes
```kconfig
# Power Management
CONFIG_PM=y
CONFIG_PM_DEVICE=y
CONFIG_PM_DEVICE_RUNTIME=y

# Sleep States
CONFIG_PM_STATE_SOFT_OFF=y
CONFIG_PM_STATE_STANDBY=y
CONFIG_PM_MIN_RESIDENCY_SOFT_OFF=5000
```

### Wake Sources
```dts
&gpio0 {
    status = "okay";
    
    wake-pins {
        compatible = "gpio-keys";
        button0: button_0 {
            gpios = <&gpio0 11 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
            label = "Wake button";
        };
    };
};
```

## Storage Configuration

### Flash Memory
```kconfig
# Flash Configuration
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y
CONFIG_FLASH_PAGE_LAYOUT=y

# File System
CONFIG_FILE_SYSTEM=y
CONFIG_FILE_SYSTEM_LITTLEFS=y
```

### Partition Layout
```dts
&flash0 {
    partitions {
        compatible = "fixed-partitions";
        #address-cells = <1>;
        #size-cells = <1>;

        storage_partition: partition@f8000 {
            label = "storage";
            reg = <0xf8000 0x8000>;
        };
    };
};
```

## Debug Configuration

### Logging Levels
```kconfig
# Logging Configuration
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3
CONFIG_LOG_BACKEND_UART=y
CONFIG_LOG_BUFFER_SIZE=4096

# Module-specific Logging
CONFIG_CELLULAR_LOG_LEVEL_DBG=y
CONFIG_LORAWAN_LOG_LEVEL_INF=y
CONFIG_SENSOR_LOG_LEVEL_WRN=y
```

### Shell Commands
```kconfig
# Shell Configuration
CONFIG_SHELL=y
CONFIG_SHELL_BACKEND_UART=y
CONFIG_SHELL_HISTORY=y
CONFIG_SHELL_VT100_COLORS=y
```

## Performance Configuration

### Thread Priorities
```c
/* src/config/thread_config.h */
#define SENSOR_PRIORITY      5
#define AUDIO_PRIORITY      6
#define DATA_PRIORITY       7
#define STORAGE_PRIORITY    8
```

### Stack Sizes
```c
/* src/config/memory_config.h */
#define SENSOR_STACK_SIZE   2048
#define AUDIO_STACK_SIZE    4096
#define DATA_STACK_SIZE     2048
#define STORAGE_STACK_SIZE  2048
```

## Build Configuration

### Optimization Levels
```kconfig
# Build Options
CONFIG_SIZE_OPTIMIZATIONS=y
CONFIG_SPEED_OPTIMIZATIONS=y
CONFIG_COMPILER_OPT="-Os"
```

### Debug Build
```kconfig
# Debug Build Options
CONFIG_DEBUG_OPTIMIZATIONS=y
CONFIG_DEBUG_THREAD_INFO=y
CONFIG_THREAD_ANALYZER=y
CONFIG_STACK_SENTINEL=y
```

## Example Configurations

### Power-Optimized
```kconfig
# Maximum Power Saving
CONFIG_PM_DEVICE=y
CONFIG_LTE_PSM_REQ=y
CONFIG_LTE_EDRX_REQ=y
CONFIG_PM_STATE_SOFT_OFF=y
CONFIG_SIZE_OPTIMIZATIONS=y
```

### Development
```kconfig
# Development Configuration
CONFIG_DEBUG=y
CONFIG_SHELL=y
CONFIG_LOG_LEVEL_DBG=y
CONFIG_THREAD_ANALYZER=y
CONFIG_DEBUG_OPTIMIZATIONS=y
```

### Production
```kconfig
# Production Configuration
CONFIG_SIZE_OPTIMIZATIONS=y
CONFIG_PM_DEVICE=y
CONFIG_LOG_LEVEL_WRN=y
CONFIG_SHELL=n
CONFIG_DEBUG=n
```

## Configuration Files

### Project Structure
```
config/
├── prj.conf           # Main configuration
├── prj_debug.conf     # Debug configuration
├── prj_prod.conf      # Production configuration
└── boards/
    └── nrf52840dk_nrf52840.overlay
```

### Build Variants
```bash
# Debug build
west build -b nrf52840dk_nrf52840 -- -DCONF_FILE=prj_debug.conf

# Production build
west build -b nrf52840dk_nrf52840 -- -DCONF_FILE=prj_prod.conf
