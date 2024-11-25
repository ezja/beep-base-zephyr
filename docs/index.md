# BEEP Base Firmware Documentation

Welcome to the BEEP Base firmware documentation. This documentation covers all aspects of the Zephyr RTOS port with nRF9161 cellular support.

## Quick Links

- [Getting Started](getting_started.md) - First steps with the firmware
- [Building and Flashing](building.md) - Build system and deployment
- [Configuration Guide](configuration.md) - Available configuration options
- [Troubleshooting](troubleshooting.md) - Common issues and solutions

## Core Documentation

### System Architecture
- [Hardware Setup](hardware.md)
  * Pin assignments
  * Board configuration
  * Component connections
  * Power supply

- [Power Management](power_management.md)
  * Sleep modes
  * Power optimization
  * Wake-up sources
  * Battery management

- [Communication](communication.md)
  * LoRaWAN integration
  * Cellular (nRF9161) support
  * BLE functionality
  * Protocol details

### Development
- [Building](building.md)
  * Build system setup
  * Configuration options
  * Flashing procedures
  * Debug builds

- [Contributing](contributing.md)
  * Development workflow
  * Code style
  * Testing requirements
  * Pull request process

- [Configuration](configuration.md)
  * System settings
  * Communication options
  * Power management
  * Debug options

### Support
- [Troubleshooting](troubleshooting.md)
  * Common issues
  * Diagnostic procedures
  * Recovery steps
  * Debug tools

### Migration
- [Differences from Original](differences.md)
  * Architecture changes
  * Feature enhancements
  * API modifications
  * Configuration updates

## Feature Documentation

### Sensors
- Environmental Monitoring
  * BME280 temperature/humidity/pressure
  * DS18B20 temperature sensors
  * HX711 weight sensor
  * Audio sampling and processing

### Communication
- Dual Communication Paths
  * LoRaWAN for long-range, low-power
  * Cellular (nRF9161) for high-reliability
  * Automatic failover
  * Power-aware selection

### Power Management
- Advanced Power Features
  * Multiple sleep modes
  * PSM and eDRX support
  * Intelligent wake-up
  * Power monitoring

### Storage
- Data Management
  * Flash filesystem
  * Measurement storage
  * Configuration persistence
  * Firmware updates

## API Reference

### Core APIs
- [Communication Manager](api/comm_mgr.md)
- [Power Management](api/power_mgmt.md)
- [Sensor Interface](api/sensors.md)
- [Storage System](api/storage.md)

### Protocol APIs
- [LoRaWAN Interface](api/lorawan.md)
- [Cellular Interface](api/cellular.md)
- [BLE Services](api/ble.md)

### Utility APIs
- [Debug System](api/debug.md)
- [Configuration](api/config.md)
- [Error Handling](api/errors.md)

## Development Resources

### Tools
- [Build Scripts](tools/build.md)
- [Debug Tools](tools/debug.md)
- [Test Suite](tools/testing.md)
- [Power Analysis](tools/power.md)

### Examples
- [Basic Usage](examples/basic.md)
- [Power Optimization](examples/power.md)
- [Communication](examples/comm.md)
- [Sensor Integration](examples/sensors.md)

## Version Information

Current Version: 1.0.0

- [Changelog](CHANGELOG.md)
- [Release Notes](RELEASE_NOTES.md)
- [Migration Guide](MIGRATION.md)

## Additional Resources

- [FAQ](faq.md)
- [Known Issues](known_issues.md)
- [License](../LICENSE)

## Navigation

- [Back to Project Root](../README.md)
- [Report an Issue](https://github.com/your-repo/beep-base-zephyr/issues)
- [View Source](https://github.com/your-repo/beep-base-zephyr)
