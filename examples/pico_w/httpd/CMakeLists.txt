cmake_minimum_required(VERSION 3.13)

set(PROGRAM_NAME pico_w_webserver)
set(PICO_BOARD pico_w)

include(pico_sdk_import.cmake)

project(pico_w_webserver)

pico_sdk_init()

message("Running makefsdata python script")
execute_process(COMMAND
    py makefsdata.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

add_executable(${PROGRAM_NAME}
    main.c
)

target_include_directories(${PROGRAM_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROGRAM_NAME}
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_http
    pico_stdlib
    hardware_adc
)

target_compile_definitions(${PROGRAM_NAME} PRIVATE
    # lwip
    WIFI_SSID=\"$ENV{WIFI_SSID}\"
    WIFI_PASSWORD=\"$ENV{WIFI_PASSWORD}\"
)

# pico_enable_stdio_usb(${PROGRAM_NAME} TRUE)
# pico_enable_stdio_uart(${PROGRAM_NAME} FALSE)

pico_add_extra_outputs(${PROGRAM_NAME})