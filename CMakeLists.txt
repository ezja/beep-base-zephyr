cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(beep_base)

# Add include directories
zephyr_include_directories(
    src/
    drivers/
    config/
)

# Add source files
target_sources(app PRIVATE
    src/main.c
    src/audio_app.c
    src/beep_protocol.c
    src/beep_types.c
    drivers/tlv320adc3100.c
    drivers/hx711.c
    drivers/bme280.c
    drivers/ds18b20.c
)
