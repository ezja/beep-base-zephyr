/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* Debug levels */
#define DEBUG_LEVEL_NONE    0
#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_WARN    2
#define DEBUG_LEVEL_INFO    3
#define DEBUG_LEVEL_DEBUG   4
#define DEBUG_LEVEL_VERBOSE 5

/* Debug categories */
#define DEBUG_CAT_SYSTEM   BIT(0)  /* System events */
#define DEBUG_CAT_SENSOR   BIT(1)  /* Sensor operations */
#define DEBUG_CAT_AUDIO    BIT(2)  /* Audio processing */
#define DEBUG_CAT_STORAGE  BIT(3)  /* Storage operations */
#define DEBUG_CAT_COMM     BIT(4)  /* Communication */
#define DEBUG_CAT_POWER    BIT(5)  /* Power management */
#define DEBUG_CAT_ALL      0xFFFF  /* All categories */

/* Debug configuration */
struct debug_config {
    uint8_t level;         /* Debug level */
    uint16_t categories;   /* Enabled categories */
    bool timestamp;        /* Include timestamp */
    bool thread_info;      /* Include thread info */
};

/* Debug macros - only active in debug builds */
#ifdef CONFIG_DEBUG_BUILD

#define DEBUG_INIT() debug_init()
#define DEBUG_CONFIG(cfg) debug_configure(cfg)

#define DEBUG_ERR(cat, fmt, ...) \
    if (debug_is_enabled(DEBUG_LEVEL_ERROR, cat)) { \
        debug_print(DEBUG_LEVEL_ERROR, cat, fmt, ##__VA_ARGS__); \
    }

#define DEBUG_WRN(cat, fmt, ...) \
    if (debug_is_enabled(DEBUG_LEVEL_WARN, cat)) { \
        debug_print(DEBUG_LEVEL_WARN, cat, fmt, ##__VA_ARGS__); \
    }

#define DEBUG_INF(cat, fmt, ...) \
    if (debug_is_enabled(DEBUG_LEVEL_INFO, cat)) { \
        debug_print(DEBUG_LEVEL_INFO, cat, fmt, ##__VA_ARGS__); \
    }

#define DEBUG_DBG(cat, fmt, ...) \
    if (debug_is_enabled(DEBUG_LEVEL_DEBUG, cat)) { \
        debug_print(DEBUG_LEVEL_DEBUG, cat, fmt, ##__VA_ARGS__); \
    }

#define DEBUG_VERBOSE(cat, fmt, ...) \
    if (debug_is_enabled(DEBUG_LEVEL_VERBOSE, cat)) { \
        debug_print(DEBUG_LEVEL_VERBOSE, cat, fmt, ##__VA_ARGS__); \
    }

#define DEBUG_DUMP(cat, data, len) \
    if (debug_is_enabled(DEBUG_LEVEL_DEBUG, cat)) { \
        debug_dump(cat, data, len); \
    }

#define DEBUG_ASSERT(cond) \
    if (!(cond)) { \
        debug_assert_failed(#cond, __FILE__, __LINE__); \
    }

#define DEBUG_ENTER(cat) \
    if (debug_is_enabled(DEBUG_LEVEL_VERBOSE, cat)) { \
        debug_print(DEBUG_LEVEL_VERBOSE, cat, "Enter %s", __func__); \
    }

#define DEBUG_EXIT(cat) \
    if (debug_is_enabled(DEBUG_LEVEL_VERBOSE, cat)) { \
        debug_print(DEBUG_LEVEL_VERBOSE, cat, "Exit %s", __func__); \
    }

#else /* CONFIG_DEBUG_BUILD */

#define DEBUG_INIT()
#define DEBUG_CONFIG(cfg)
#define DEBUG_ERR(cat, fmt, ...)
#define DEBUG_WRN(cat, fmt, ...)
#define DEBUG_INF(cat, fmt, ...)
#define DEBUG_DBG(cat, fmt, ...)
#define DEBUG_VERBOSE(cat, fmt, ...)
#define DEBUG_DUMP(cat, data, len)
#define DEBUG_ASSERT(cond)
#define DEBUG_ENTER(cat)
#define DEBUG_EXIT(cat)

#endif /* CONFIG_DEBUG_BUILD */

/**
 * @brief Initialize debug module
 *
 * @return 0 on success, negative errno code on failure
 */
int debug_init(void);

/**
 * @brief Configure debug options
 *
 * @param config Debug configuration
 * @return 0 on success, negative errno code on failure
 */
int debug_configure(const struct debug_config *config);

/**
 * @brief Check if debug level and category are enabled
 *
 * @param level Debug level
 * @param category Debug category
 * @return true if enabled, false if disabled
 */
bool debug_is_enabled(uint8_t level, uint16_t category);

/**
 * @brief Print debug message
 *
 * @param level Debug level
 * @param category Debug category
 * @param fmt Format string
 * @param ... Variable arguments
 */
void debug_print(uint8_t level, uint16_t category,
                const char *fmt, ...);

/**
 * @brief Dump binary data in hex format
 *
 * @param category Debug category
 * @param data Data to dump
 * @param len Data length
 */
void debug_dump(uint16_t category, const uint8_t *data, size_t len);

/**
 * @brief Handle assertion failure
 *
 * @param expr Failed expression
 * @param file Source file
 * @param line Line number
 */
void debug_assert_failed(const char *expr, const char *file,
                        unsigned int line);

#endif /* DEBUG_H */
