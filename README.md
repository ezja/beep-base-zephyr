# BEEP Base Firmware - Zephyr Port


### Sensors
- DS18B20 temperature sensors (1-Wire)
- HX711 weight sensor with Bosche H40A load cell
- BME280 environmental monitoring
- TLV320ADC3100 audio ADC with FFT processing
- DS3231 RTC with temperature compensation

### Communication
- LoRaWAN connectivity
- Cellular connectivity (nRF9161)
- Bluetooth Low Energy
- Automatic communication failover

### Added in Zephyr version:

- RTOS-based multithreading
- Enhanced power management
- Dual communication paths (LoRaWAN/Cellular)

### Storage
- MX25 flash memory
- LittleFS filesystem
- Power-safe operations
- Wear leveling

### Power Management
- Multiple sleep modes


## Setup:

1. Clone the repository:
```bash
git clone https://github.com/ezja/beep-base-zephyr
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
