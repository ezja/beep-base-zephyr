# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Find Zephyr and nRF Connect SDK
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
include(${ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

# Set project name and version
project(beep_base)

# Add nRF Connect SDK modules
include(${ZEPHYR_BASE}/../nrf/cmake/modules.cmake)

# nRF Libraries
add_subdirectory(${ZEPHYR_BASE}/../nrf/lib/nrf_modem_lib)
add_subdirectory(${ZEPHYR_BASE}/../nrf/lib/at_monitor)
add_subdirectory(${ZEPHYR_BASE}/../nrf/lib/at_cmd_parser)
add_subdirectory(${ZEPHYR_BASE}/../nrf/lib/lte_link_control)
add_subdirectory(${ZEPHYR_BASE}/../nrf/lib/modem_info)
add_subdirectory(${ZEPHYR_BASE}/../nrf/lib/modem_key_mgmt)

# Add custom drivers
add_subdirectory(drivers)

# Application source files
target_sources(app PRIVATE
    src/main.c
    src/audio_app.c
    src/alarm_app.c
    src/ble_app.c
    src/cellular_app.c
    src/comm_mgr.c
    src/debug.c
    src/flash_fs.c
    src/lorawan_app.c
    src/power_mgmt.c
    src/rtc_app.c
)

# Include directories
target_include_directories(app PRIVATE
    src
    drivers/flash
    drivers/rtc
    drivers/sensor/tlv320adc3100
    drivers/sensor/bme280
    drivers/sensor/ds18b20
    drivers/sensor/hx711
    drivers/w1
    ${ZEPHYR_BASE}/../nrf/include
    ${ZEPHYR_BASE}/../nrf/subsys/net/lib/nrf_modem_lib/include
    ${ZEPHYR_BASE}/../nrf/lib/lte_link_control
    ${ZEPHYR_BASE}/../nrf/lib/at_monitor
    ${ZEPHYR_BASE}/../nrf/lib/at_cmd_parser
    ${ZEPHYR_BASE}/../nrf/lib/modem_info
    ${ZEPHYR_BASE}/../nrf/lib/modem_key_mgmt
)

# Link nRF libraries
target_link_libraries(app PRIVATE
    nrf_modem_lib
    at_monitor
    at_cmd_parser
    lte_link_control
    modem_info
    modem_key_mgmt
)

# Generate version information
execute_process(
    COMMAND git describe --tags --long --dirty
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT GIT_VERSION)
    set(GIT_VERSION "unknown")
endif()

target_compile_definitions(app PRIVATE
    -DAPP_VERSION="${GIT_VERSION}"
    -DAPP_BUILD_DATE="${CMAKE_CURRENT_BINARY_DIR}/build_date.h"
    -DCONFIG_NRF_MODEM_LIB=1
    -DCONFIG_LTE_LINK_CONTROL=1
    -DCONFIG_AT_MONITOR=1
    -DCONFIG_MODEM_INFO=1
)

# Generate build date header
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/build_date.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/build_date.h
    @ONLY
)

# Add drivers subdirectory CMakeLists.txt
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/drivers/CMakeLists.txt
"# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_W1 w1)
add_subdirectory_ifdef(CONFIG_TLV320ADC3100 sensor/tlv320adc3100)
add_subdirectory_ifdef(CONFIG_BME280 sensor/bme280)
add_subdirectory_ifdef(CONFIG_DS18B20 sensor/ds18b20)
add_subdirectory_ifdef(CONFIG_HX711 sensor/hx711)
add_subdirectory_ifdef(CONFIG_DS3231_RTC rtc)
add_subdirectory_ifdef(CONFIG_MX25_FLASH flash)
")

# Create build date template
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/build_date.h.in
"#ifndef BUILD_DATE_H
#define BUILD_DATE_H

#define BUILD_DATE \"@BUILD_DATE@\"

#endif /* BUILD_DATE_H */
")

# Set required compile options for nRF libraries
target_compile_options(app PRIVATE
    -DCONFIG_NRF_MODEM_LIB_TRACE=1
    -DCONFIG_NRF_MODEM_LIB_SHMEM_TRACE=1
    -DCONFIG_NRF_MODEM_LIB_DEBUG=1
    -DCONFIG_AT_CMD_LOG_LEVEL=4
    -DCONFIG_AT_NOTIF_LOG_LEVEL=4
)

# Set linker options for nRF libraries
target_link_options(app PRIVATE
    -Wl,--whole-archive
    -l:libnrf_modem_lib.a
    -Wl,--no-whole-archive
)

# Add custom targets for nRF tools
add_custom_target(flash_modem
    COMMAND ${ZEPHYR_BASE}/../nrf/scripts/nrf_modem_flash.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Flashing modem firmware..."
)

add_custom_target(flash_all
    COMMAND west flash
    COMMAND ${CMAKE_MAKE_PROGRAM} flash_modem
    COMMENT "Flashing application and modem firmware..."
)
