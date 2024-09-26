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

set(PICO_BOARD pico2 CACHE STRING "Board type")

set(FREERTOS_KERNEL_PATH $ENV{FREERTOS_KERNEL_PATH})

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

project(simple_example)

# Initialise the Pico SDK
pico_sdk_init()

add_subdirectory(../../src/FreeRTOS+FAT+CLI build)

# Add executable. Default name is the project name, version 0.1
add_executable(simple_example
        main.cpp
        hw_config.c
)


target_link_libraries(simple_example
    pico_stdlib
    pico_time
    FreeRTOS+FAT+CLI
)
target_include_directories(simple_example PUBLIC 
        include/ 
)

target_compile_options(simple_example PUBLIC 
  -Wall 
  -Wextra 
  -Wshadow 
  -Wstack-usage=2048 
  -fanalyzer 
)
target_compile_definitions(simple_example PUBLIC
    # pico-sdk
    #PARAM_ASSERTIONS_ENABLE_ALL=1
    PICO_USE_STACK_GUARDS=1
    PICO_MALLOC_PANIC=1
    PICO_MAX_SHARED_IRQ_HANDLERS=6
)
target_compile_definitions(simple_example PRIVATE
    USE_PRINTF
)

set_property(TARGET simple_example APPEND_STRING PROPERTY LINK_FLAGS
    "-Wl,--print-memory-usage"
)

pico_set_program_name(simple_example "simple_example")

# See 4.1. Serial input and output on Raspberry Pi Pico in Getting started with Raspberry Pi Pico (https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
# and 2.7.1. Standard Input/Output (stdio) Support in Raspberry Pi Pico C/C++ SDK (https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf):
# pico_enable_stdio_uart(simple_example 1)
pico_enable_stdio_usb(simple_example 1)

pico_add_extra_outputs(simple_example)
