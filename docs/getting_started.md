# Getting Started

This guide will help you get started with the BEEP Base firmware on Zephyr RTOS.

## Quick Start

### 1. Prerequisites Installation

```bash
# Install system dependencies (Ubuntu/Debian)
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
  make gcc gcc-multilib g++-multilib libsdl2-dev

# Install west tool
pip3 install --user -U west

# Install nRF Command Line Tools
# Download from Nordic website and install:
# https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Command-Line-Tools
```

### 2. Get the Source Code

```bash
# Create workspace directory
mkdir beep-workspace
cd beep-workspace

# Initialize west workspace
west init -m https://github.com/your-repo/beep-base-zephyr
west update

# Get toolchain
wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
tar xf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
```

### 3. Environment Setup

```bash
# Add to ~/.bashrc or ~/.zshrc
export ZEPHYR_BASE=$HOME/beep-workspace/zephyr
export GNUARMEMB_TOOLCHAIN_PATH=$HOME/beep-workspace/gcc-arm-none-eabi-10.3-2021.10

# Reload shell configuration
source ~/.bashrc  # or source ~/.zshrc
```

### 4. Basic Build and Flash

```bash
# Build firmware
./scripts/build.sh -c

# Flash firmware
./scripts/build.sh -f
```

## Hardware Setup

### 1. Basic Connections

```
nRF52840-DK:
├── USB (Programming/Debug)
├── nRF9161 Modem
│   ├── UART: P0.06 (TX), P0.08 (RX)
│   ├── Control: P0.12 (PWR), P0.19 (RST)
│   └── Status: P0.26 (NET), P0.27 (SLEEP)
└── Sensors
    ├── I2C: P0.04 (SDA), P0.05 (SCL)
    ├── 1-Wire: P0.26
    └── HX711: P0.27 (SCK), P0.28 (DOUT)
```

### 2. Power Supply

- USB power for development
- External 3.3V-5V for deployment
- Battery backup (CR2032) for RTC

## Initial Configuration

### 1. Communication Setup

```c
// Edit cellular configuration
#define CELLULAR_APN       "your.apn"
#define CELLULAR_BAND      20             // Your LTE band
```

### 2. Sensor Configuration

```c
// Edit src/config/sensor_config.h
#define TEMP_INTERVAL      300            // 5 minutes
#define WEIGHT_INTERVAL    300            // 5 minutes
#define AUDIO_INTERVAL     3600           // 1 hour
```

### 3. Power Management

```c
// Edit src/config/power_config.h
#define SLEEP_TIMEOUT_MS   300000         // 5 minutes
#define PSM_ACTIVE_TIME    60             // 1 minute
#define EDRX_CYCLE         5              // 81.92 seconds
```

## Basic Usage

### 1. Development Build

```bash
# Build with debug output
./scripts/build.sh -d -v

# Flash and monitor
./scripts/build.sh -f
minicom -D /dev/ttyACM0 -b 115200
```

### 2. Testing Communication

```bash
# Test LoRaWAN
./scripts/test_lorawan.sh

# Test Cellular
./scripts/test_cellular.sh

# Monitor both
./scripts/monitor_comm.sh
```

### 3. Power Profiling

```bash
# Enable power profiling
west build -- -DCONFIG_PM_DEBUG=y

# Monitor power
./scripts/power_profile.sh
```

## Common Operations

### 1. Firmware Update

```bash
# Update application
./scripts/build.sh -f

# Update modem firmware
./scripts/build.sh -m new_modem.hex
```

### 2. Configuration Changes

```bash
# Edit Kconfig options
west build -t menuconfig

# Edit device tree
west build -t guiconfig
```

### 3. Debugging

```bash
# Enable debug logging
west build -- -DCONFIG_LOG_DEFAULT_LEVEL=4

# Start debug session
west debug

# View RTT logs
JLinkRTTViewer
```

## Troubleshooting

### 1. Build Issues

```bash
# Clean build
west build -p

# Check environment
./scripts/verify_nrf.py

# Verify dependencies
west list
```

### 2. Flash Issues

```bash
# Reset board
nrfjprog --reset

# Recover board
nrfjprog --recover

# Verify connection
nrfjprog --ids
```

### 3. Communication Issues

```bash
# Check LoRaWAN status
./scripts/check_lorawan.sh

# Check cellular status
./scripts/check_cellular.sh

# Monitor all communication
./scripts/monitor_all.sh
```

## Next Steps

1. Read the detailed documentation:
   - [Hardware Setup](hardware.md)
   - [Power Management](power_management.md)
   - [Communication](communication.md)

2. Explore advanced features:
   - Custom sensor configurations
   - Power optimization
   - Data management

3. Contribute to development:
   - Check [CONTRIBUTING.md](../CONTRIBUTING.md)
   - Review coding standards
   - Submit pull requests

## Support

- GitHub Issues: Report bugs and request features
- Documentation: Check the docs/ directory
- Community: Join our Discord server

## Updates

Keep your system up to date:

```bash
# Update repositories
west update

# Update tools
pip3 install --user -U west

# Update documentation
./scripts/update_docs.sh
```

Remember to check the [changelog](../CHANGELOG.md) for important updates and breaking changes.
