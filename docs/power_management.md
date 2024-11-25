# Power Management

This document details the power management features implemented in the Zephyr port of the BEEP Base firmware, with special focus on cellular optimizations.

## Overview

The power management system is designed to maximize battery life while maintaining reliable operation. It implements multiple power-saving strategies across all system components.

## Power States

### System Power States
1. **Active**
   - Full system operation
   - All peripherals enabled
   - Maximum power consumption
   - Used during measurements and transmission

2. **Light Sleep**
   - CPU suspended
   - Peripherals in low power
   - Quick wake-up capability
   - Used between measurements

3. **Deep Sleep**
   - Most systems powered down
   - RTC and wake-up sources active
   - Longest battery life
   - Used during inactive periods

4. **Standby**
   - Minimal power consumption
   - Only RTC active
   - RAM retention
   - Used for long-term storage

## Cellular Power Optimization

### PSM (Power Saving Mode)
```c
psm_config_t psm_config = {
    .enabled = true,
    .tau_sec = 43200,    // 12 hours
    .active_sec = 60     // 1 minute
};
```

Features:
- Configurable TAU (Tracking Area Update) interval
- Adjustable active time
- Network-synchronized timing
- Automatic registration maintenance

### eDRX (Extended Discontinuous Reception)
```c
edrx_config_t edrx_config = {
    .enabled = true,
    .mode = 0,           // LTE-M mode
    .ptw = 0,           // 1.28 seconds PTW
    .cycle = 5          // 81.92 seconds cycle
};
```

Benefits:
- Reduced power during idle
- Maintained network connection
- Configurable paging windows
- Balance between latency and power

### RAI (Release Assistance Indication)
```c
rai_config_t rai_config = {
    .enabled = true,
    .no_more_data = true,
    .more_data = false
};
```

Advantages:
- Faster connection release
- Reduced network overhead
- Power optimization
- Network resource efficiency

## Wake-up Sources

### Hardware Wake-up
- RTC alarm
- Button press
- Sensor interrupts
- Modem activity

### Software Wake-up
- Measurement schedule
- Communication events
- System maintenance
- Error conditions

## Power Monitoring

### Current Consumption
| State | Typical Current |
|-------|----------------|
| Active TX | 200mA |
| Active RX | 50mA |
| Light Sleep | 3mA |
| eDRX | 0.5mA |
| PSM | 10µA |
| Standby | 5µA |

### Battery Management
```c
power_stats_t stats;
power_mgmt_get_stats(&stats);

printf("Average current: %u mA\n", stats.avg_current_ma);
printf("Battery voltage: %u mV\n", stats.battery_mv);
printf("Estimated runtime: %u hours\n", stats.estimated_runtime_h);
```

## Optimization Strategies

### Measurement Optimization
```c
// Configure power-aware sampling
sensor_config_t config = {
    .sample_rate = SAMPLE_RATE_ADAPTIVE,
    .power_mode = POWER_MODE_EFFICIENT,
    .wake_threshold = 100
};
```

### Communication Optimization
```c
// Configure power-efficient transmission
comm_config_t config = {
    .method = COMM_METHOD_AUTO,
    .batch_size = 10,
    .retry_count = 3,
    .power_threshold = -90
};
```

### Storage Optimization
```c
// Configure power-safe storage
storage_config_t config = {
    .write_buffer_size = 256,
    .flush_threshold = 128,
    .power_safe = true
};
```

## Implementation Example

### Power State Transitions
```c
void handle_measurement_cycle(void)
{
    // Wake up necessary peripherals
    power_mgmt_set_state(POWER_STATE_ACTIVE);
    
    // Perform measurements
    take_measurements();
    
    // Process and store data
    process_data();
    
    // Transmit if needed
    if (should_transmit()) {
        transmit_data();
    }
    
    // Calculate next wake-up
    uint32_t next_wake = calculate_next_wakeup();
    
    // Configure cellular PSM
    configure_psm_for_interval(next_wake);
    
    // Enter appropriate sleep mode
    power_mgmt_set_state(POWER_STATE_DEEP_SLEEP);
}
```

### Power-Aware Transmission
```c
void transmit_data(void)
{
    // Check signal conditions
    if (get_signal_strength() < SIGNAL_THRESHOLD) {
        // Defer transmission
        schedule_retry();
        return;
    }
    
    // Prepare transmission
    prepare_data_packet();
    
    // Enable cellular module
    cellular_power_up();
    
    // Wait for network
    wait_for_network();
    
    // Send data
    send_data_packet();
    
    // Configure PSM
    enable_power_saving();
    
    // Return to sleep
    cellular_power_down();
}
```

## Configuration Options

### Kconfig Options
```
CONFIG_PM=y
CONFIG_PM_DEVICE=y
CONFIG_PM_DEVICE_RUNTIME=y
CONFIG_LTE_PSM_REQ=y
CONFIG_LTE_EDRX_REQ=y
CONFIG_LTE_RAI_REQ=y
```

### Device Tree Configuration
```dts
&cpu0 {
    cpu-power-states = <&state_sleep &state_deep_sleep>;
};

&uart0 {
    pinctrl-0 = <&uart0_default>;
    pinctrl-1 = <&uart0_sleep>;
    pinctrl-names = "default", "sleep";
};
```

## Debugging and Analysis

### Power Profiling
```bash
# Enable power profiling
west build -b nrf52840dk_nrf52840 -- -DCONFIG_PM_DEBUG=y

# View power statistics
west debug --power-profile
```

### Current Measurement
```c
// Get power consumption estimate
uint16_t avg_ma, peak_ma;
cellular_app_get_power_consumption(&avg_ma, &peak_ma);
```

## Best Practices

1. **Batch Operations**
   - Group measurements
   - Combine transmissions
   - Minimize wake-ups

2. **Signal Awareness**
   - Monitor signal strength
   - Adapt transmission power
   - Use appropriate timing

3. **Power State Management**
   - Minimize active time
   - Use appropriate sleep modes
   - Handle wake-up sources

4. **Error Handling**
   - Power-safe recovery
   - Maintain state information
   - Efficient retry mechanisms
