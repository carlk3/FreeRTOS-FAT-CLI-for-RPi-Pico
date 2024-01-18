cmake_minimum_required(VERSION 3.13)

set(PROGRAM_NAME httpd)
set(PICO_BOARD pico_w)

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

project(${PROGRAM_NAME})

# Initialise the Pico SDK
pico_sdk_init()

add_subdirectory(../../../src/FreeRTOS+FAT+CLI build)

add_executable(${PROGRAM_NAME}
    main.c
    sd_filesystem.c
    hw_config.c
)

target_include_directories(${PROGRAM_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROGRAM_NAME}
    FreeRTOS+FAT+CLI
    pico_cyw43_arch_lwip_sys_freertos
    pico_lwip_http
)

target_compile_definitions(${PROGRAM_NAME} PRIVATE

    # lwip
    WIFI_SSID=\"$ENV{WIFI_SSID}\"
    WIFI_PASSWORD=\"$ENV{WIFI_PASSWORD}\"

    # FreeRTOS+FAT+CLI
    USE_PRINTF
    USE_DBG_PRINTF
    # USE_SPI_INTERFACE

    # pico-sdk
    PARAM_ASSERTIONS_ENABLE_ALL=1
    PICO_USE_STACK_GUARDS=1
    PICO_MALLOC_PANIC=1
)

set_property(TARGET ${PROGRAM_NAME} APPEND_STRING PROPERTY LINK_FLAGS
    "-Wl,--print-memory-usage"
)

# pico_enable_stdio_usb(${PROGRAM_NAME} TRUE)
# pico_enable_stdio_uart(${PROGRAM_NAME} FALSE)
pico_add_extra_outputs(${PROGRAM_NAME})