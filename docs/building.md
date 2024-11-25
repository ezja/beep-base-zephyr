# Building and Flashing

This document details the build process, configuration options, and flashing procedures for the BEEP Base firmware.

## Prerequisites

### Required Software
- nRF Connect SDK v2.4.0
- CMake 3.20.0 or newer
- Python 3.8 or newer
- GNU Arm Embedded Toolchain 10.3-2021.10
- nrfjprog and related tools
- west tool

### Environment Setup
```bash
# Set up environment variables
export ZEPHYR_BASE=/path/to/zephyr
export GNUARMEMB_TOOLCHAIN_PATH=/path/to/toolchain

# Initialize west workspace
west init -m https://github.com/your-repo/beep-base-zephyr
west update
```

## Build System

### Directory Structure
```
beep-base-zephyr/
├── src/                    # Application source files
├── drivers/                # Custom drivers
├── dts/                    # Device tree files
├── boards/                # Board configuration
├── include/               # Header files
├── scripts/               # Build and utility scripts
└── docs/                  # Documentation
```

### Build Configuration

#### Project Configuration
```bash
# Default release build
./scripts/build.sh -f

# Debug build with verbose output
./scripts/build.sh -d -v

# Clean build with modem update
./scripts/build.sh -c -f -m modem_firmware.hex
```

#### Build Types
1. **Release Build**
   ```bash
   west build -b nrf52840dk_nrf52840 -- -DCONF_FILE=prj.conf
   ```

2. **Debug Build**
   ```bash
   west build -b nrf52840dk_nrf52840 -- -DCONF_FILE=prj_debug.conf
   ```

3. **Production Build**
   ```bash
   west build -b nrf52840dk_nrf52840 -- -DCONF_FILE=prj_prod.conf
   ```

### Configuration Options

#### Kconfig Options
```
# Core Configuration
CONFIG_BEEP_BASE=y
CONFIG_VERSION_MAJOR=1
CONFIG_VERSION_MINOR=0
CONFIG_VERSION_SUB=0

# Communication Options
CONFIG_LORAWAN=y
CONFIG_CELLULAR=y
CONFIG_BLE=y

# Power Management
CONFIG_PM=y
CONFIG_PM_DEVICE=y
CONFIG_LTE_PSM_REQ=y

# Debug Options
CONFIG_DEBUG=y
CONFIG_SHELL=y
CONFIG_THREAD_ANALYZER=y
```

#### Device Tree Overlays
```dts
/* boards/nrf52840dk_nrf52840.overlay */

/ {
    chosen {
        zephyr,console = &uart0;
        zephyr,shell-uart = &uart0;
    };
};

&uart0 {
    status = "okay";
    current-speed = <115200>;
};
```

## Building

### Basic Build
```bash
# Clean build directory
west build -p

# Build firmware
west build -b nrf52840dk_nrf52840
```

### Advanced Build Options
```bash
# Build with size optimization
west build -- -DCONFIG_SIZE_OPTIMIZATIONS=y

# Build with speed optimization
west build -- -DCONFIG_SPEED_OPTIMIZATIONS=y

# Build with debug symbols
west build -- -DCONFIG_DEBUG_OPTIMIZATIONS=y
```

### Build Variants
```bash
# Development build
./scripts/build.sh --variant dev

# Production build
./scripts/build.sh --variant prod

# Test build
./scripts/build.sh --variant test
```

## Flashing

### Basic Flashing
```bash
# Flash application
west flash

# Flash with reset
west flash --reset
```

### Advanced Flashing
```bash
# Flash specific partition
west flash --erase --file build/zephyr/zephyr.hex

# Flash modem firmware
./scripts/build.sh -m modem_firmware.hex
```

### Multiple Components
```bash
# Flash all components
./scripts/build.sh --flash-all

# Flash application and modem
./scripts/build.sh -f -m modem_firmware.hex
```

## Debugging

### Debug Build
```bash
# Build with debug symbols
west build -b nrf52840dk_nrf52840 -- -DCONFIG_DEBUG_OPTIMIZATIONS=y

# Enable debug logging
west build -- -DCONFIG_LOG_DEFAULT_LEVEL=4
```

### Debug Tools
```bash
# Start debug session
west debug

# Start J-Link RTT viewer
JLinkRTTViewer

# Start system trace
west trace
```

## Verification

### Build Verification
```bash
# Verify nRF libraries
./scripts/verify_nrf.py --verbose

# Check build configuration
./scripts/verify_build.py

# Validate firmware
./scripts/validate_firmware.py
```

### Testing
```bash
# Run unit tests
west test

# Run integration tests
./scripts/test.sh --integration

# Run system tests
./scripts/test.sh --system
```

## Common Issues

### Build Issues
1. **Missing Dependencies**
   ```bash
   # Install missing packages
   west update
   pip install -r scripts/requirements.txt
   ```

2. **Compilation Errors**
   - Check toolchain version
   - Verify environment variables
   - Clean build directory

3. **Flash Failures**
   - Check board connection
   - Verify nrfjprog installation
   - Reset board manually

## Best Practices

1. **Version Control**
   - Tag releases
   - Use semantic versioning
   - Document changes

2. **Build Management**
   - Use clean builds
   - Version configurations
   - Maintain build scripts

3. **Testing**
   - Run verification
   - Test all variants
   - Validate functionality

4. **Documentation**
   - Update build docs
   - Document configurations
   - Maintain changelog

## Continuous Integration

### GitHub Actions
```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Set up environment
        run: |
          ./scripts/setup_env.sh
      - name: Build firmware
        run: |
          ./scripts/build.sh
      - name: Run tests
        run: |
          ./scripts/test.sh
```

### Local CI
```bash
# Run local CI pipeline
./scripts/ci_local.sh

# Run specific checks
./scripts/ci_local.sh --build --test --verify
```

## Release Process

1. **Prepare Release**
   ```bash
   ./scripts/prepare_release.sh 1.0.0
   ```

2. **Build Release**
   ```bash
   ./scripts/build.sh --release
   ```

3. **Verify Release**
   ```bash
   ./scripts/verify_release.sh
   ```

4. **Package Release**
   ```bash
   ./scripts/package_release.sh
