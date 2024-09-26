# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico CACHE STRING "Board type")

set(FREERTOS_KERNEL_PATH $ENV{FREERTOS_KERNEL_PATH})
set(PROGRAM_NAME command_line)

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Pull in FreeRTOS
if ("${PICO_PLATFORM}" STREQUAL "rp2040")
    include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)
elseif("${PICO_PLATFORM}" STREQUAL "rp2350-arm-s")
    include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2350_ARM_NTZ/FreeRTOS_Kernel_import.cmake)
elseif("${PICO_PLATFORM}" STREQUAL "rp2350-riscv")
    include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2350_RISC-V/FreeRTOS_Kernel_import.cmake)
endif()

project(${PROGRAM_NAME})

# Initialise the Pico SDK
pico_sdk_init()

add_subdirectory(../../src/FreeRTOS+FAT+CLI build)

# Add executable. Default name is the project name, version 0.1
add_executable(${PROGRAM_NAME} 
    config/hw_config.c
    src/command.cpp
    src/data_log_demo.c
    src/main.cpp 
    src/stdio_cli.c
    src/unmounter.c
    tests/app4-IO_module_function_checker.c
    tests/bench.c
    tests/big_file_test.c
    tests/mtbft.c
    tests/CreateAndVerifyExampleFiles.c
    tests/ff_stdio_tests_with_cwd.c
    tests/simple.c
    ../../src/FreeRTOS+FAT+CLI/src/crash.c
)

target_compile_options(${PROGRAM_NAME} PUBLIC 
  -Wall 
  -Wextra 
  -Wshadow 
  -Wstack-usage=2048 
  -fanalyzer 
)
target_compile_definitions(${PROGRAM_NAME} PUBLIC
    # pico-sdk
    #PARAM_ASSERTIONS_ENABLE_ALL=1
    PICO_USE_STACK_GUARDS=1
    PICO_MALLOC_PANIC=1
    PICO_MAX_SHARED_IRQ_HANDLERS=6
)
target_compile_definitions(${PROGRAM_NAME} PRIVATE
    # This program is useless without standard input and output.
    USE_PRINTF
    #USE_DBG_PRINTF

    #SPI_SD0
)

# Disable CRC checking for SPI-attached cards.
# add_compile_definitions(SD_CRC_ENABLED=0)

# Use Pico's LED to show drive activity. 
# Ensure that PICO_DEFAULT_LED_PIN is set correctly.
# Note that Pico W uses GPIO 25 for SPI communication to the CYW43439.
# add_compile_definitions(USE_LED=1)

set_property(TARGET ${PROGRAM_NAME} APPEND_STRING PROPERTY LINK_FLAGS 
  "-Wl,--print-memory-usage"
)

pico_set_program_name(${PROGRAM_NAME} "${PROGRAM_NAME}")
pico_set_program_version(${PROGRAM_NAME} "2.13.0")

# See 4.1. Serial input and output on Raspberry Pi Pico in Getting started with Raspberry Pi Pico (https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
# and 2.7.1. Standard Input/Output (stdio) Support in Raspberry Pi Pico C/C++ SDK (https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf):
#pico_enable_stdio_uart(${PROGRAM_NAME} 1)
pico_enable_stdio_usb(${PROGRAM_NAME} 1)

target_include_directories(${PROGRAM_NAME} PUBLIC 
        include/ 
)
target_link_libraries(${PROGRAM_NAME}
    FreeRTOS+FAT+CLI
    hardware_clocks
    hardware_adc
)

pico_add_extra_outputs(${PROGRAM_NAME})
