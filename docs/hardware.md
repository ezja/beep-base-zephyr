# Hardware Setup

This document details the hardware configuration and connections for the BEEP Base with nRF9161 cellular support.

## System Overview

![System Block Diagram](images/system_block.png)

The system consists of:
- nRF52840 main controller
- nRF9161 cellular modem
- Sensor array
- Power management system
- Storage system

## Pin Assignments

### Main Controller (nRF52840)

#### I2C Bus (Primary)
| Signal | Pin | Description |
|--------|-----|-------------|
| SCL | P0.05 | I2C clock |
| SDA | P0.04 | I2C data |

Connected devices:
- BME280 environmental sensor
- TLV320ADC3100 audio ADC
- DS3231 RTC

#### UART (nRF9161 Communication)
| Signal | Pin | Description |
|--------|-----|-------------|
| TX | P0.06 | UART transmit |
| RX | P0.08 | UART receive |
| RTS | P0.07 | Request to send |
| CTS | P0.09 | Clear to send |

#### nRF9161 Control
| Signal | Pin | Description |
|--------|-----|-------------|
| PWR_CTRL | P0.12 | Modem power control |
| RESET | P0.19 | Modem reset |
| MODE | P0.23 | Network mode selection |
| PSM_CTRL | P0.24 | PSM control |
| EDRX_CTRL | P0.25 | eDRX control |
| STATUS | P0.26 | Network status |
| SLEEP | P0.27 | Sleep indication |

#### Sensors
| Signal | Pin | Description |
|--------|-----|-------------|
| 1-WIRE | P0.26 | DS18B20 temperature sensors |
| HX711_SCK | P0.27 | Weight sensor clock |
| HX711_DOUT | P0.28 | Weight sensor data |

#### SPI (Flash Memory)
| Signal | Pin | Description |
|--------|-----|-------------|
| SCK | P0.16 | SPI clock |
| MOSI | P0.17 | Master out slave in |
| MISO | P0.18 | Master in slave out |
| CS | P0.29 | Chip select |

#### Status LEDs
| Signal | Pin | Description |
|--------|-----|-------------|
| LED_RED | P0.13 | Error indication |
| LED_GREEN | P0.14 | Status indication |
| LED_BLUE | P0.15 | Activity indication |

## Power Supply

### Main Power
- Input voltage: 3.3V - 5.5V
- Operating current: 200mA typical
- Peak current: 400mA (during cellular transmission)

### Battery Backup
- Battery type: CR2032
- Backup current: < 1µA
- Backed up systems:
  * RTC
  * Critical settings
  * Sensor calibration

### Power Domains
1. **Always On**
   - RTC
   - Wake-up circuitry
   - Battery monitor

2. **Switchable**
   - Sensors
   - Flash memory
   - Cellular modem

3. **Conditional**
   - Audio system
   - External interfaces

## Hardware Configuration

### nRF9161 Modem Setup
```c
// Power sequence timing
#define MODEM_POWER_ON_DELAY_MS   100
#define MODEM_RESET_PULSE_MS      10
#define MODEM_BOOT_DELAY_MS       1000

// Voltage levels
#define MODEM_VDD_MIN_MV          3000
#define MODEM_VDD_MAX_MV          3600
```

### Sensor Configuration

#### BME280
```c
// I2C Address: 0x76
const struct i2c_dt_spec bme280 = {
    .bus = I2C_DT_SPEC_GET(DT_NODELABEL(i2c0)),
    .addr = 0x76
};
```

#### TLV320ADC3100
```c
// I2C Address: 0x18
const struct i2c_dt_spec audio_adc = {
    .bus = I2C_DT_SPEC_GET(DT_NODELABEL(i2c0)),
    .addr = 0x18
};
```

#### HX711
```c
const struct gpio_dt_spec hx711_sck = GPIO_DT_SPEC_GET(
    DT_NODELABEL(hx711), sck_gpios);
const struct gpio_dt_spec hx711_dout = GPIO_DT_SPEC_GET(
    DT_NODELABEL(hx711), dout_gpios);
```

## PCB Layout Considerations

### Critical Routes
1. **UART Lines**
   - Keep traces short
   - Maintain impedance control
   - Avoid crossing power planes

2. **Sensor Bus**
   - Star topology for I2C
   - Proper pull-up sizing
   - Signal integrity protection

3. **Power Distribution**
   - Separate analog/digital grounds
   - Decoupling capacitors
   - Power plane splits

### EMI/EMC Considerations
1. **Cellular Section**
   - RF keep-out area
   - Ground plane continuity
   - Component placement

2. **Sensitive Circuits**
   - Audio ADC isolation
   - Sensor shielding
   - Ground plane strategy

## Assembly Notes

### Component Placement
1. **RF Considerations**
   - Antenna placement
   - Ground plane requirements
   - Component clearance

2. **Thermal Management**
   - Component spacing
   - Thermal reliefs
   - Heat dissipation

### Testing Points
| Signal | Location | Purpose |
|--------|----------|---------|
| VBAT | TP1 | Battery voltage |
| 3V3 | TP2 | Main supply |
| GND | TP3 | Ground reference |
| UART_TX | TP4 | Communication test |
| UART_RX | TP5 | Communication test |

## Troubleshooting

### Power Issues
1. Check supply voltage
2. Verify power sequencing
3. Monitor current consumption
4. Check power good signals

### Communication Problems
1. Verify UART connections
2. Check modem power state
3. Monitor status signals
4. Verify signal integrity

### Sensor Issues
1. Check I2C bus voltage
2. Verify pull-up resistors
3. Test sensor power
4. Check signal routing

## Bill of Materials

### Critical Components
| Component | Part Number | Description |
|-----------|-------------|-------------|
| MCU | nRF52840 | Main controller |
| Modem | nRF9161 | Cellular modem |
| Sensors | BME280 | Environmental |
| ADC | TLV320ADC3100 | Audio conversion |
| RTC | DS3231 | Real-time clock |
| Flash | MX25R6435F | 64Mb flash memory |

### Power Components
| Component | Rating | Purpose |
|-----------|--------|---------|
| Regulator | 3.3V, 500mA | Main power |
| Battery | CR2032 | Backup power |
| Supervisor | 3.0V threshold | Reset control |

## Safety Considerations

### ESD Protection
- Input protection on all external connections
- Ground plane strategy
- Component selection

### Environmental
- Operating temperature: -40°C to +85°C
- Humidity: 0-95% non-condensing
- IP54 enclosure recommended

### Regulatory
- CE marking requirements
- FCC compliance
- Cellular certification
