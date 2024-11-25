# BEEP Base Firmware - Zephyr Port

This repository contains the Zephyr RTOS port of the BEEP Base firmware, with enhanced features including cellular connectivity via the nRF9161 modem.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Getting Started](getting_started.md)
- [Hardware Setup](hardware.md)
- [Building and Flashing](building.md)
- [Configuration](configuration.md)
- [Power Management](power_management.md)
- [Communication](communication.md)
- [Differences from Original](differences.md)
- [Contributing](contributing.md)

## Overview

The BEEP Base firmware has been ported to Zephyr RTOS to leverage its robust features, power management capabilities, and extensive driver support. This port maintains all original functionality while adding new features and improvements.

### Key Improvements

- RTOS-based multithreading
- Enhanced power management
- Dual communication paths (LoRaWAN/Cellular)
- Improved error handling
- Comprehensive debugging support

## Features

### Sensors
- TLV320ADC3100 audio ADC with FFT processing
- BME280 environmental monitoring
- DS18B20 temperature sensors (1-Wire)
- HX711 weight sensor
- DS3231 RTC with temperature compensation

### Communication
- LoRaWAN connectivity
- Cellular connectivity (nRF9161)
- Bluetooth Low Energy
- Automatic communication failover

### Storage
- MX25 flash memory
- LittleFS filesystem
- Power-safe operations
- Wear leveling

### Power Management
- Multiple sleep modes
- Intelligent wake-up sources
- Power consumption optimization
- Battery monitoring

### System Features
- Real-time operating system
- Multi-threaded architecture
- Comprehensive error handling
- Debug logging system

## Quick Start

1. Clone the repository:
```bash
git clone https://github.com/your-repo/beep-base-zephyr
cd beep-base-zephyr
```

2. Initialize west workspace:
```bash
west init -l .
west update
```

3. Build the firmware:
```bash
./scripts/build.sh -f
```

## Documentation Structure

- [Getting Started](getting_started.md): Initial setup and basic usage
- [Hardware Setup](hardware.md): Detailed hardware connections and configuration
- [Building and Flashing](building.md): Build system and flashing instructions
- [Configuration](configuration.md): System configuration options
- [Power Management](power_management.md): Power saving features and configuration
- [Communication](communication.md): LoRaWAN and Cellular communication
- [Differences from Original](differences.md): Detailed comparison with original firmware
- [Contributing](contributing.md): Guidelines for contributing to the project

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](../LICENSE) file for details.

## Acknowledgments

- Original BEEP Base firmware developers
- Nordic Semiconductor for nRF Connect SDK
- Zephyr Project contributors
