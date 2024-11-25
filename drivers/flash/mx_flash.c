/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT macronix_mx25

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "mx_flash.h"

LOG_MODULE_REGISTER(mx_flash, CONFIG_FLASH_LOG_LEVEL);

/* Maximum timeout for flash operations (in ms) */
#define MX_FLASH_TIMEOUT_MS 1000

/* Internal functions */
static int mx_flash_wait_ready(const struct device *dev)
{
    const struct mx_flash_config *config = dev->config;
    uint8_t cmd = MX25_CMD_READ_STATUS;
    uint8_t status;
    int64_t timeout = k_uptime_get() + MX_FLASH_TIMEOUT_MS;

    struct spi_buf tx_buf = {
        .buf = &cmd,
        .len = 1
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1
    };

    struct spi_buf rx_buf = {
        .buf = &status,
        .len = 1
    };
    const struct spi_buf_set rx = {
        .buffers = &rx_buf,
        .count = 1
    };

    do {
        int ret = spi_transceive_dt(&config->spi, &tx, &rx);
        if (ret < 0) {
            return ret;
        }

        if (!(status & BIT(MX25_STATUS_WIP_BIT))) {
            return 0;
        }

        k_sleep(K_MSEC(1));
    } while (k_uptime_get() < timeout);

    return -ETIMEDOUT;
}

static int mx_flash_write_enable(const struct device *dev)
{
    const struct mx_flash_config *config = dev->config;
    uint8_t cmd = MX25_CMD_WRITE_ENABLE;
    const struct spi_buf tx_buf = {
        .buf = &cmd,
        .len = 1
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1
    };

    return spi_write_dt(&config->spi, &tx);
}

static int mx_flash_read_id(const struct device *dev, uint8_t *id)
{
    const struct mx_flash_config *config = dev->config;
    uint8_t cmd = MX25_CMD_READ_ID;

    struct spi_buf tx_buf = {
        .buf = &cmd,
        .len = 1
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1
    };

    struct spi_buf rx_buf = {
        .buf = id,
        .len = 3
    };
    const struct spi_buf_set rx = {
        .buffers = &rx_buf,
        .count = 1
    };

    return spi_transceive_dt(&config->spi, &tx, &rx);
}

/* API Implementation */
int mx_flash_read(const struct device *dev, off_t offset, void *data, size_t len)
{
    const struct mx_flash_config *config = dev->config;
    struct mx_flash_data *flash_data = dev->data;
    uint8_t cmd[4] = {MX25_CMD_READ_DATA,
                      (offset >> 16) & 0xFF,
                      (offset >> 8) & 0xFF,
                      offset & 0xFF};

    struct spi_buf tx_buf[] = {
        {
            .buf = cmd,
            .len = sizeof(cmd)
        }
    };
    const struct spi_buf_set tx = {
        .buffers = tx_buf,
        .count = 1
    };

    struct spi_buf rx_buf[] = {
        {
            .buf = data,
            .len = len
        }
    };
    const struct spi_buf_set rx = {
        .buffers = rx_buf,
        .count = 1
    };

    k_sem_take(&flash_data->lock, K_FOREVER);
    int ret = spi_transceive_dt(&config->spi, &tx, &rx);
    k_sem_give(&flash_data->lock);

    return ret;
}

int mx_flash_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
    const struct mx_flash_config *config = dev->config;
    struct mx_flash_data *flash_data = dev->data;
    int ret;

    if (flash_data->write_protection) {
        return -EACCES;
    }

    k_sem_take(&flash_data->lock, K_FOREVER);

    /* Write page by page */
    while (len > 0) {
        size_t page_offset = offset & (config->page_size - 1);
        size_t write_len = MIN(len, config->page_size - page_offset);

        uint8_t cmd[4] = {MX25_CMD_PAGE_PROGRAM,
                         (offset >> 16) & 0xFF,
                         (offset >> 8) & 0xFF,
                         offset & 0xFF};

        struct spi_buf tx_buf[] = {
            {
                .buf = cmd,
                .len = sizeof(cmd)
            },
            {
                .buf = (void *)data,
                .len = write_len
            }
        };
        const struct spi_buf_set tx = {
            .buffers = tx_buf,
            .count = 2
        };

        ret = mx_flash_write_enable(dev);
        if (ret < 0) {
            break;
        }

        ret = spi_write_dt(&config->spi, &tx);
        if (ret < 0) {
            break;
        }

        ret = mx_flash_wait_ready(dev);
        if (ret < 0) {
            break;
        }

        offset += write_len;
        data = (const uint8_t *)data + write_len;
        len -= write_len;
    }

    k_sem_give(&flash_data->lock);
    return ret;
}

int mx_flash_erase(const struct device *dev, off_t offset)
{
    const struct mx_flash_config *config = dev->config;
    struct mx_flash_data *flash_data = dev->data;
    int ret;

    if (flash_data->write_protection) {
        return -EACCES;
    }

    k_sem_take(&flash_data->lock, K_FOREVER);

    uint8_t cmd[4] = {MX25_CMD_SECTOR_ERASE,
                      (offset >> 16) & 0xFF,
                      (offset >> 8) & 0xFF,
                      offset & 0xFF};

    struct spi_buf tx_buf = {
        .buf = cmd,
        .len = sizeof(cmd)
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1
    };

    ret = mx_flash_write_enable(dev);
    if (ret == 0) {
        ret = spi_write_dt(&config->spi, &tx);
        if (ret == 0) {
            ret = mx_flash_wait_ready(dev);
        }
    }

    k_sem_give(&flash_data->lock);
    return ret;
}

size_t mx_flash_size(const struct device *dev)
{
    const struct mx_flash_config *config = dev->config;
    return config->size;
}

int mx_flash_write_protection_set(const struct device *dev, bool enable)
{
    struct mx_flash_data *data = dev->data;
    data->write_protection = enable;
    return 0;
}

bool mx_flash_write_protection_get(const struct device *dev)
{
    const struct mx_flash_data *data = dev->data;
    return data->write_protection;
}

int mx_flash_power_down(const struct device *dev)
{
    const struct mx_flash_config *config = dev->config;
    uint8_t cmd = MX25_CMD_POWER_DOWN;
    const struct spi_buf tx_buf = {
        .buf = &cmd,
        .len = 1
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1
    };

    return spi_write_dt(&config->spi, &tx);
}

int mx_flash_power_up(const struct device *dev)
{
    const struct mx_flash_config *config = dev->config;
    uint8_t cmd = MX25_CMD_RELEASE_POWER_DOWN;
    const struct spi_buf tx_buf = {
        .buf = &cmd,
        .len = 1
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1
    };

    return spi_write_dt(&config->spi, &tx);
}

static int mx_flash_init(const struct device *dev)
{
    const struct mx_flash_config *config = dev->config;
    struct mx_flash_data *data = dev->data;
    uint8_t id[3];
    int ret;

    if (!spi_is_ready_dt(&config->spi)) {
        LOG_ERR("SPI bus not ready");
        return -ENODEV;
    }

    k_sem_init(&data->lock, 1, 1);
    data->write_protection = false;

    /* Configure GPIOs if available */
    if (config->reset_gpio.port) {
        ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            return ret;
        }
    }

    if (config->wp_gpio.port) {
        ret = gpio_pin_configure_dt(&config->wp_gpio, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            return ret;
        }
    }

    if (config->hold_gpio.port) {
        ret = gpio_pin_configure_dt(&config->hold_gpio, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            return ret;
        }
    }

    /* Read and verify chip ID */
    ret = mx_flash_read_id(dev, id);
    if (ret < 0) {
        LOG_ERR("Failed to read chip ID");
        return ret;
    }

    LOG_INF("MX25 Flash ID: %02x %02x %02x", id[0], id[1], id[2]);
    return 0;
}

/* Driver API structure */
static const struct flash_driver_api mx_flash_api = {
    .read = mx_flash_read,
    .write = mx_flash_write,
    .erase = mx_flash_erase,
    .get_size = mx_flash_size,
};

/* Device instantiation */
#define MX_FLASH_INIT(n)                                                  \
    static struct mx_flash_data mx_flash_data_##n;                       \
                                                                         \
    static const struct mx_flash_config mx_flash_config_##n = {          \
        .spi = SPI_DT_SPEC_INST_GET(n, SPI_WORD_SET(8), 0),            \
        .reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),    \
        .wp_gpio = GPIO_DT_SPEC_INST_GET_OR(n, wp_gpios, {0}),         \
        .hold_gpio = GPIO_DT_SPEC_INST_GET_OR(n, hold_gpios, {0}),     \
        .size = DT_INST_PROP(n, size),                                  \
        .sector_size = DT_INST_PROP(n, sector_size),                    \
        .block_size = DT_INST_PROP(n, block_size),                      \
        .page_size = DT_INST_PROP(n, page_size),                        \
    };                                                                   \
                                                                         \
    DEVICE_DT_INST_DEFINE(n,                                            \
                         mx_flash_init,                                  \
                         NULL,                                           \
                         &mx_flash_data_##n,                            \
                         &mx_flash_config_##n,                          \
                         POST_KERNEL,                                    \
                         CONFIG_FLASH_INIT_PRIORITY,                     \
                         &mx_flash_api);

DT_INST_FOREACH_STATUS_OKAY(MX_FLASH_INIT)
