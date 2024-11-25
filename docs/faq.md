# Frequently Asked Questions

## General Questions

### Q: What are the main differences between this port and the original firmware?
A: The main differences are:
- RTOS-based architecture for better resource management
- Dual communication support (LoRaWAN and Cellular)
- Enhanced power management features
- Improved error handling and recovery
- Comprehensive debugging capabilities

### Q: What hardware is required?
A: Required hardware:
- nRF52840-DK development board
- nRF9161 cellular modem
- BME280 environmental sensor
- DS18B20 temperature sensors
- HX711 weight sensor
- TLV320ADC3100 audio ADC
- DS3231 RTC
- MX25 flash memory

## Power Management

### Q: What is the expected battery life?
A: Battery life depends on configuration, but typical scenarios:
- Standard mode: 3-6 months
- Power optimized: 6-12 months
- With cellular disabled: 12+ months

### Q: How does PSM (Power Saving Mode) work with cellular?
A: PSM allows the modem to enter deep sleep while maintaining network registration:
- Configurable active time (default 1 minute)
- Adjustable TAU period (default 12 hours)
- Automatic wake-up for scheduled transmissions
- Power consumption in PSM < 10µA

## Communication

### Q: How does the system choose between LoRaWAN and Cellular?
A: The selection is based on multiple factors:
- Signal strength of both networks
- Battery level
- Data size to transmit
- Time criticality of data
- Power consumption requirements

### Q: What happens if communication fails?
A: The system implements a failover strategy:
1. Retry with primary method (configurable attempts)
2. Switch to alternate method if available
3. Store data for later transmission
4. Log failure for debugging

## Sensors

### Q: What is the maximum number of DS18B20 sensors supported?
A: The system supports up to 8 DS18B20 sensors on the 1-Wire bus.

### Q: How accurate is the weight measurement?
A: HX711 accuracy depends on configuration:
- 24-bit resolution
- Configurable gain (32, 64, 128)
- Typical accuracy: ±0.1% at 10 samples
- Temperature compensated

## Development

### Q: How do I debug power consumption issues?
A: Several tools are available:
```bash
# Enable power profiling
west build -- -DCONFIG_PM_DEBUG=y

# Monitor consumption
./scripts/power_profile.sh

# Analyze results
./scripts/analyze_power.py power.csv
```

### Q: How can I test cellular connectivity?
A: Use the provided test tools:
```bash
# Check modem status
./scripts/check_cellular.sh

# Test connectivity
./scripts/test_cellular.sh --full

# Monitor power consumption
./scripts/monitor_cellular_power.sh
```

## Troubleshooting

### Q: What should I do if the device doesn't enter PSM?
A: Check these common issues:
1. Network support for PSM
2. Correct PSM timing configuration
3. No active data transfers
4. No pending network operations

### Q: How do I recover from a failed firmware update?
A: Recovery procedure:
```bash
# 1. Enter recovery mode
nrfjprog --recover

# 2. Flash bootloader
west flash -d build/bootloader

# 3. Flash application
west flash
```

## Configuration

### Q: How do I optimize for maximum battery life?
A: Key configurations:
```kconfig
# Enable all power saving features
CONFIG_PM=y
CONFIG_PM_DEVICE=y
CONFIG_LTE_PSM_REQ=y
CONFIG_LTE_EDRX_REQ=y
CONFIG_PM_STATE_SOFT_OFF=y

# Adjust timing
CONFIG_LTE_PSM_REQ_RPTAU="10100101"  # 12 hours
CONFIG_LTE_PSM_REQ_RAT="00000000"    # 0 seconds
```

### Q: How do I configure for time-critical applications?
A: Recommended settings:
```kconfig
# Disable power saving features
CONFIG_LTE_PSM_REQ=n
CONFIG_LTE_EDRX_REQ=n

# Enable fast response
CONFIG_THREAD_PRIORITY_CELLULAR=5
CONFIG_HEAP_MEM_POOL_SIZE=16384
```

## Updates and Maintenance

### Q: How often should I update the modem firmware?
A: Recommendations:
- Check for updates quarterly
- Update when adding new features
- Update if experiencing connectivity issues
- Always test updates in development first

### Q: How do I backup device configuration?
A: Use the backup tools:
```bash
# Backup all settings
./scripts/backup_config.sh

# Backup specific components
./scripts/backup_config.sh --component cellular
```

## Integration

### Q: Can I add custom sensors?
A: Yes, through these steps:
1. Add sensor driver to drivers/
2. Update devicetree overlay
3. Implement sensor interface
4. Update power management

### Q: How do I integrate with custom backend systems?
A: Options available:
1. Modify payload format
2. Implement custom protocols
3. Use cellular for direct connection
4. Configure data forwarding

## Performance

### Q: What is the typical data throughput?
A: Depends on communication method:
- LoRaWAN: 0.3-50 kbps
- Cellular: Up to 375 kbps
- Local storage: >1 Mbps

### Q: How much data can be stored locally?
A: Storage capacity:
- 8MB total flash
- ~7MB available for data
- Circular buffer implementation
- Automatic cleanup of old data
