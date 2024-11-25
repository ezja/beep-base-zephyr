# BEEP Base Firmware (Zephyr Port)

This is a port of the BEEP Base beehive monitoring system firmware from nRF SDK 15.3 to the Zephyr RTOS. The system provides comprehensive beehive monitoring capabilities including audio analysis, environmental monitoring, and weight measurement.

## Features

- Audio monitoring using TLV320ADC3100 ADC with FFT processing
- Environmental monitoring (BME280 sensor)
- Temperature monitoring (DS18B20 sensors)
- Weight monitoring (HX711 load cell)
- Bluetooth Low Energy connectivity
- LoRaWAN communication
- Power management and battery monitoring

## Requirements

- Zephyr RTOS (tested with version 3.4.0)
- Nordic nRF52840 DK or compatible hardware
- West build tool
- ARM GCC toolchain

## Building

1. Initialize west workspace:
```bash
west init -m https://github.com/your-repo/beep-base-zephyr --mr main
west update
```

2. Build the project:
```bash
west build -b nrf52840dk_nrf52840
```

3. Flash to the board:
```bash
west flash
```

## Project Structure

```
beep-base-zephyr/
├── src/                    # Application source files
│   ├── audio_app.c        # Audio processing application
│   ├── beep_protocol.h    # Communication protocol definitions
│   └── beep_types.h       # Common type definitions
├── drivers/               # Custom drivers
│   └── sensor/
│       └── tlv320adc3100/ # Audio ADC driver
├── dts/                   # Device Tree files
│   └── bindings/         # Device Tree bindings
├── boards/               # Board-specific files
│   └── nrf52840dk_nrf52840.overlay
├── CMakeLists.txt        # Build system configuration
├── prj.conf              # Project configuration
└── west.yml             # West manifest file
```

## Configuration

The system can be configured through the following methods:

1. Device Tree Overlay (`boards/nrf52840dk_nrf52840.overlay`):
   - Pin assignments
   - I2C/SPI/I2S configuration
   - Sensor configurations

2. Project Configuration (`prj.conf`):
   - Enable/disable features
   - Configure system parameters
   - Set communication options

## Hardware Connections

- TLV320ADC3100 (Audio ADC):
  - I2C: SCL = P0.30, SDA = P0.31
  - I2S: SCK = P0.26, LRCK = P0.27, SDIN = P0.28
  - Reset: P0.25

- BME280 (Environmental Sensor):
  - I2C: Shared with TLV320ADC3100
  - Address: 0x76

- HX711 (Weight Sensor):
  - SCK = P0.20
  - DOUT = P0.22

## Power Management

The system implements sophisticated power management using Zephyr's Power Management subsystem:
- Automatic sleep when idle
- Peripheral power control
- Battery voltage monitoring

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is licensed under the Apache License 2.0 - see the LICENSE file for details.
