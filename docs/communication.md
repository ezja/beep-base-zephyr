# Communication Systems

This document details the dual communication system implemented in the BEEP Base firmware, featuring both LoRaWAN and Cellular (nRF9161) connectivity.

## Overview

The communication system provides redundant data transmission paths through LoRaWAN and Cellular networks, with automatic selection and failover capabilities.

## Communication Methods

### LoRaWAN
```c
lorawan_config_t lorawan_config = {
    .dev_eui = LORAWAN_DEV_EUI,
    .app_eui = LORAWAN_APP_EUI,
    .app_key = LORAWAN_APP_KEY,
    .adr_enabled = true,
    .data_rate = DR_0,
    .tx_power = 14
};
```

Features:
- EU868 frequency band
- OTAA activation
- Adaptive data rate
- Duty cycle compliance
- Power-efficient operation

### Cellular (nRF9161)
```c
cellular_config_t cell_config = {
    .apn = "your.apn",
    .band = 20,
    .mode = LTE_MODE_M1,
    .power_class = 3,
    .enable_psm = true
};
```

Features:
- LTE-M connectivity
- Power saving mode
- Extended DRX
- Release assistance
- Signal monitoring

## Automatic Method Selection

### Selection Criteria
```c
typedef struct {
    int8_t lorawan_rssi;
    int8_t cellular_rssi;
    uint8_t battery_level;
    uint16_t data_size;
    bool time_critical;
} comm_criteria_t;
```

Decision Matrix:
| Condition | LoRaWAN | Cellular |
|-----------|---------|----------|
| Battery Low | ✓ | - |
| Large Data | - | ✓ |
| Time Critical | - | ✓ |
| Good Signal | ✓ | ✓ |
| Poor Signal | - | ✓ |

### Implementation
```c
COMM_METHOD_e select_method(const comm_criteria_t *criteria)
{
    // Battery optimization
    if (criteria->battery_level < 20) {
        return COMM_METHOD_LORAWAN;
    }

    // Data size optimization
    if (criteria->data_size > 51) {
        return COMM_METHOD_CELLULAR;
    }

    // Signal strength based
    if (criteria->lorawan_rssi > -100) {
        return COMM_METHOD_LORAWAN;
    }

    return COMM_METHOD_CELLULAR;
}
```

## Data Transmission

### Payload Format
```c
typedef struct {
    uint32_t timestamp;
    uint8_t device_id[8];
    uint8_t msg_type;
    uint8_t data_len;
    uint8_t data[MAX_PAYLOAD_SIZE];
    uint16_t crc;
} transmission_payload_t;
```

### LoRaWAN Transmission
```c
int send_lorawan_data(const uint8_t *data, uint8_t len)
{
    // Configure transmission
    lorawan_tx_config_t tx_config = {
        .port = LORAWAN_PORT,
        .type = LORAWAN_MSG_CONFIRMED,
        .data_rate = DR_0
    };

    // Send data
    return lorawan_send(data, len, &tx_config);
}
```

### Cellular Transmission
```c
int send_cellular_data(const uint8_t *data, uint8_t len)
{
    // Prepare connection
    cellular_conn_config_t conn = {
        .rai_enabled = true,
        .release_assist = true
    };

    // Send data
    return cellular_send(data, len, &conn);
}
```

## Error Handling

### Transmission Retry
```c
typedef struct {
    uint8_t retry_count;
    uint16_t retry_delay_ms;
    bool fallback_enabled;
    COMM_METHOD_e fallback_method;
} retry_config_t;
```

### Implementation
```c
int handle_transmission_error(transmission_context_t *ctx)
{
    // Check retry count
    if (ctx->retries < ctx->config.retry_count) {
        // Schedule retry
        ctx->retries++;
        return schedule_retry(ctx);
    }

    // Try fallback method
    if (ctx->config.fallback_enabled) {
        ctx->method = ctx->config.fallback_method;
        ctx->retries = 0;
        return transmit_data(ctx);
    }

    return -EFAULT;
}
```

## Power Optimization

### LoRaWAN Power Save
```c
lorawan_power_config_t power_config = {
    .adr_enabled = true,
    .tx_power_max = 14,
    .duty_cycle = true,
    .join_backoff = true
};
```

### Cellular Power Save
```c
cellular_power_config_t power_config = {
    .psm_enabled = true,
    .edrx_enabled = true,
    .rai_enabled = true,
    .reduced_mobility = true
};
```

## Monitoring and Debugging

### Signal Monitoring
```c
void monitor_signal_strength(void)
{
    int8_t lora_rssi, cell_rssi;
    
    // Get signal strengths
    lorawan_get_rssi(&lora_rssi);
    cellular_get_rssi(&cell_rssi);
    
    // Log signal levels
    LOG_INF("LoRa RSSI: %d, Cell RSSI: %d", 
            lora_rssi, cell_rssi);
}
```

### Status Reporting
```c
typedef struct {
    uint32_t messages_sent;
    uint32_t messages_failed;
    uint32_t retries;
    uint32_t fallbacks;
    uint32_t bytes_sent;
} comm_statistics_t;
```

## Configuration Examples

### Time-Critical Data
```c
comm_config_t config = {
    .method = COMM_METHOD_CELLULAR,
    .priority = PRIORITY_HIGH,
    .timeout_ms = 5000,
    .retry_count = 3
};
```

### Battery-Efficient Mode
```c
comm_config_t config = {
    .method = COMM_METHOD_LORAWAN,
    .priority = PRIORITY_LOW,
    .batch_size = 10,
    .power_save = true
};
```

## Best Practices

1. **Method Selection**
   - Consider battery level
   - Check signal strength
   - Evaluate data size
   - Assess time criticality

2. **Power Management**
   - Use appropriate power modes
   - Batch transmissions
   - Implement backoff strategies
   - Monitor power consumption

3. **Error Handling**
   - Implement retry logic
   - Use fallback methods
   - Monitor success rates
   - Log error conditions

4. **Data Management**
   - Optimize payload size
   - Implement compression
   - Batch when possible
   - Prioritize data

## Testing and Validation

### Coverage Testing
```bash
# Test LoRaWAN coverage
./scripts/test_coverage.sh --method lorawan

# Test Cellular coverage
./scripts/test_coverage.sh --method cellular
```

### Power Analysis
```bash
# Analyze power consumption
./scripts/power_profile.sh --duration 3600
