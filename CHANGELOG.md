# Changelog

All notable changes to the BEEP Base firmware will be documented in this file.

## [1.1.0] - 2023-12-14

### Added
- Enhanced PSM (Power Saving Mode) features:
  * Optimized wake-up timing
  * Automatic timing adjustment
  * Wake-up latency tracking
  * Configurable retry mechanism

- Flash write buffering:
  * 4KB write buffer
  * Configurable flush timing
  * Wear leveling support
  * Power-safe operations

- Automatic sensor calibration:
  * Temperature compensation
  * 24-hour calibration cycle
  * Reference sensor support
  * Manual offset configuration

- Thread synchronization improvements:
  * Priority inheritance
  * Timeout handling
  * Resource deadlock prevention
  * Multiple resource locking

### Enhanced
- Cellular communication:
  * Improved PSM handling
  * Better network reconnection
  * Signal strength monitoring
  * Power consumption tracking

- Power management:
  * Optimized sleep transitions
  * Reduced wake-up latency
  * Better power state tracking
  * Enhanced debugging support

- Memory management:
  * Reduced RAM usage
  * Optimized stack sizes
  * Better heap utilization
  * Memory monitoring tools

### Fixed
- PSM wake-up delays
- Flash write amplification
- Thread synchronization issues
- Memory fragmentation
- Power consumption spikes
- Network registration delays

### Changed
- PSM timing configuration
- Flash write strategy
- Thread priority scheme
- Power state transitions
- Error handling approach
- Debug logging format

### Optimized
- Power consumption:
  * Reduced active time
  * Optimized PSM cycles
  * Efficient data buffering
  * Smart transmission timing

- Flash operations:
  * Grouped writes
  * Wear distribution
  * Sector management
  * Power-safe commits

- Thread management:
  * Resource utilization
  * Lock handling
  * Priority management
  * Stack usage

### Security
- Enhanced error checking
- Improved state validation
- Better resource protection
- Secure configuration storage

### Documentation
- Updated power management docs
- Enhanced debugging guides
- Added configuration examples
- Improved troubleshooting guides

## [1.0.0] - 2023-11-30

### Initial Release
- Basic functionality
- LoRaWAN support
- Sensor integration
- Power management
- Flash storage
- BLE connectivity
