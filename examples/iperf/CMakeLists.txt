cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

project(command_line C CXX ASM)

# add_compile_options(-Os) # Optimize for size (place before pico_sdk_init)
set(PARAM_ASSERTIONS_ENABLE_ALL 1)

# Initialise the Pico SDK
pico_sdk_init()

add_subdirectory(../../src/FreeRTOS+FAT+CLI build)

add_executable(picow_freertos_iperf_server_sys
        picow_freertos_iperf.c
        hw_config.c
        )
target_compile_definitions(picow_freertos_iperf_server_sys PRIVATE
        WIFI_SSID=\"$ENV{WIFI_SSID}\"
        WIFI_PASSWORD=\"$ENV{WIFI_PASSWORD}\"
        NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
        USE_PRINTF
        USE_DBG_PRINTF
        )
target_include_directories(picow_freertos_iperf_server_sys PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        )
target_link_libraries(picow_freertos_iperf_server_sys
        FreeRTOS+FAT+CLI
        pico_cyw43_arch_lwip_sys_freertos
        pico_lwip_iperf
        )
pico_add_extra_outputs(picow_freertos_iperf_server_sys)
