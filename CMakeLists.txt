# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(beep_base)

# Add custom drivers
add_subdirectory(drivers)

# Add application source files
target_sources(app PRIVATE
    src/main.c
    src/audio_app.c
    src/beep_protocol.c
    src/beep_types.c
)

# Include directories
target_include_directories(app PRIVATE
    src
    drivers/sensor/tlv320adc3100
    drivers/sensor/bme280
    drivers/sensor/ds18b20
    drivers/sensor/hx711
    drivers/w1
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
")

# Create build date template
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/build_date.h.in
"#ifndef BUILD_DATE_H
#define BUILD_DATE_H

#define BUILD_DATE \"@BUILD_DATE@\"

#endif /* BUILD_DATE_H */
")
