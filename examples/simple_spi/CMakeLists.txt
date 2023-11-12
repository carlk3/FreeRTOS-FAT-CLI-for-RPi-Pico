cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

project(simple_example C CXX ASM)

set(PARAM_ASSERTIONS_ENABLE_ALL 1)

##set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")
# add_compile_options(-Os) # Optimize for size (place before pico_sdk_init)

# Initialise the Pico SDK
pico_sdk_init()

add_subdirectory(../../src/FreeRTOS+FAT+CLI build)

# Add executable. Default name is the project name, version 0.1
add_executable(simple_example
        main.cpp
        hw_config.c
)

# Add the standard library to the build
target_link_libraries(simple_example 
        pico_stdlib
        FreeRTOS+FAT+CLI
)
target_include_directories(simple_example PUBLIC 
        include/ 
)

# target_compile_options(simple_example PRIVATE -save-temps)
# target_compile_options(simple_example PRIVATE -Wall, -Wextra -Wshadow -Wdouble-promotion)
# target_compile_options(simple_example PUBLIC -Wall -Wextra -Wno-unused-function -Wno-unused-parameter)
target_compile_options(simple_example PUBLIC -Wall -Wextra -Wno-unused-parameter -fanalyzer)

# target_compile_definitions(simple_example PUBLIC DEBUG)
# add_compile_definitions(USE_PRINTF USE_DBG_PRINTF)
add_compile_definitions(USE_PRINTF)

# Disable CRC checking for SPI-attached cards.
# add_compile_definitions(SD_CRC_ENABLED=0)

# Use Pico's LED to show drive activity. 
# Ensure that PICO_DEFAULT_LED_PIN is set correctly.
# Note that Pico W uses GPIO 25 for SPI communication to the CYW43439.
# add_compile_definitions(USE_LED=1)

# target_compile_definitions(<TARGET> PRIVATE PARAM_ASSERTIONS_ENABLE_ALL=1)
add_compile_definitions(PICO_MAX_SHARED_IRQ_HANDLERS=6 PARAM_ASSERTIONS_ENABLE_ALL=1 PICO_USE_STACK_GUARDS=1)

set_property(TARGET simple_example APPEND_STRING PROPERTY LINK_FLAGS "-Wl,--print-memory-usage")

pico_set_program_name(simple_example "simple_example")
pico_set_program_version(simple_example "0.1")

# See 4.1. Serial input and output on Raspberry Pi Pico in Getting started with Raspberry Pi Pico (https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
# and 2.7.1. Standard Input/Output (stdio) Support in Raspberry Pi Pico C/C++ SDK (https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf):
# pico_enable_stdio_uart(simple_example 1)
pico_enable_stdio_usb(simple_example 1)

target_link_libraries(simple_example
    FreeRTOS+FAT+CLI
    hardware_clocks
    hardware_adc
)

pico_add_extra_outputs(simple_example)
