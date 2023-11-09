cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

project(command_line C CXX ASM)

set(PARAM_ASSERTIONS_ENABLE_ALL 1)

##set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")
# add_compile_options(-Os) # Optimize for size (place before pico_sdk_init)

# Initialise the Pico SDK
pico_sdk_init()

add_subdirectory(../../src/FreeRTOS+FAT+CLI build)

# Add executable. Default name is the project name, version 0.1
add_executable(command_line 
    config/hw_config.c
    src/command.c
    src/data_log_demo.c
    src/main.cpp 
    src/stdio_cli.c
    tests/app4-IO_module_function_checker.c
    tests/bench.c
    tests/big_file_test.c
    tests/CreateAndVerifyExampleFiles.c
    tests/ff_stdio_tests_with_cwd.c
    tests/simple.c
)
# Add the standard library to the build
    target_link_libraries(command_line 
    pico_stdlib
    FreeRTOS+FAT+CLI
)
target_include_directories(command_line PUBLIC 
        include/ 
)

# target_compile_options(command_line PUBLIC -Wall -Wextra -Wno-unused-function -Wno-unused-parameter)
target_compile_options(command_line PUBLIC -Wall -Wextra -Wno-unused-parameter -fanalyzer)

# This program is useless without standard standard input and output.
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

set_property(TARGET command_line APPEND_STRING PROPERTY LINK_FLAGS "-Wl,--print-memory-usage")

pico_set_program_name(command_line "command_line")
pico_set_program_version(command_line "0.1")

# See 4.1. Serial input and output on Raspberry Pi Pico in Getting started with Raspberry Pi Pico (https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
# and 2.7.1. Standard Input/Output (stdio) Support in Raspberry Pi Pico C/C++ SDK (https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf):
# pico_enable_stdio_uart(command_line 1)
pico_enable_stdio_usb(command_line 1)

target_link_libraries(command_line
    FreeRTOS+FAT+CLI
    hardware_clocks
    hardware_adc
)

pico_add_extra_outputs(command_line)

