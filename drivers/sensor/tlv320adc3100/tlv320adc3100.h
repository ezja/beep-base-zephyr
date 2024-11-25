#ifndef ZEPHYR_DRIVERS_SENSOR_TLV320ADC3100_H_
#define ZEPHYR_DRIVERS_SENSOR_TLV320ADC3100_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

/* TLV320ADC3100 Register Map */
#define TLV320ADC3100_PAGE_CTL          0x00
#define TLV320ADC3100_RESET             0x01
#define TLV320ADC3100_CLK_GEN           0x04
#define TLV320ADC3100_PLL_P_R           0x05
#define TLV320ADC3100_PLL_J             0x06
#define TLV320ADC3100_PLL_D_MSB         0x07
#define TLV320ADC3100_PLL_D_LSB         0x08
#define TLV320ADC3100_NDAC              0x0B
#define TLV320ADC3100_MDAC              0x0C
#define TLV320ADC3100_NADC              0x12
#define TLV320ADC3100_MADC              0x13
#define TLV320ADC3100_AOSR              0x14
#define TLV320ADC3100_IADC              0x15
#define TLV320ADC3100_ADC_FLAG          0x24
#define TLV320ADC3100_ROUTE_PIN         0x25
#define TLV320ADC3100_INT1_PIN          0x26
#define TLV320ADC3100_INT2_PIN          0x27
#define TLV320ADC3100_INT3_PIN          0x28
#define TLV320ADC3100_INT4_PIN          0x29
#define TLV320ADC3100_INT5_PIN          0x2A
#define TLV320ADC3100_GPIO1_PIN         0x2B
#define TLV320ADC3100_IN1L_2_LADC_CTL   0x37
#define TLV320ADC3100_IN1R_2_RADC_CTL   0x38
#define TLV320ADC3100_IN2L_2_LADC_CTL   0x39
#define TLV320ADC3100_IN2R_2_RADC_CTL   0x3A
#define TLV320ADC3100_IN3L_2_LADC_CTL   0x3B
#define TLV320ADC3100_IN3R_2_RADC_CTL   0x3C
#define TLV320ADC3100_LADC_VOL          0x3D
#define TLV320ADC3100_RADC_VOL          0x3E
#define TLV320ADC3100_ADC_DIGITAL       0x51
#define TLV320ADC3100_AGC_MAX_GAIN      0x56
#define TLV320ADC3100_AGC_ATTACK_TIME   0x57
#define TLV320ADC3100_AGC_DECAY_TIME    0x58
#define TLV320ADC3100_AGC_NOISE_DEB     0x59
#define TLV320ADC3100_AGC_SIGNAL_DEB    0x5A
#define TLV320ADC3100_AGC_GAIN          0x5B

/* Configuration Constants */
#define TLV320ADC3100_I2C_ADDR          0x18
#define TLV320ADC3100_RESET_DELAY_MS    1
#define TLV320ADC3100_STARTUP_DELAY_MS  10

struct tlv320adc3100_config {
    struct i2c_dt_spec i2c;
    struct gpio_dt_spec reset_gpio;
};

struct tlv320adc3100_data {
    uint8_t current_page;
    uint8_t channel;
    int8_t volume;
    uint8_t gain;
    bool min6db;
};

/**
 * @brief Write to a register on the TLV320ADC3100
 *
 * @param dev Pointer to the device structure
 * @param reg Register address
 * @param value Value to write
 * @return 0 if successful, negative errno code on failure
 */
int tlv320adc3100_reg_write(const struct device *dev, uint8_t reg, uint8_t value);

/**
 * @brief Read from a register on the TLV320ADC3100
 *
 * @param dev Pointer to the device structure
 * @param reg Register address
 * @param value Pointer to store read value
 * @return 0 if successful, negative errno code on failure
 */
int tlv320adc3100_reg_read(const struct device *dev, uint8_t reg, uint8_t *value);

/**
 * @brief Configure the TLV320ADC3100
 *
 * @param dev Pointer to the device structure
 * @param channel Input channel selection
 * @param volume Volume setting (-127 to 48)
 * @param gain Gain setting
 * @param min6db Enable -6dB mode
 * @return 0 if successful, negative errno code on failure
 */
int tlv320adc3100_configure(const struct device *dev, uint8_t channel,
                           int8_t volume, uint8_t gain, bool min6db);

/**
 * @brief Reset the TLV320ADC3100
 *
 * @param dev Pointer to the device structure
 * @return 0 if successful, negative errno code on failure
 */
int tlv320adc3100_reset(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_TLV320ADC3100_H_ */
