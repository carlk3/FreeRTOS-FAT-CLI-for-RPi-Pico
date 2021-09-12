# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)
# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

project(simple_example C CXX ASM)

##set(PICO_CXX_ENABLE_EXCEPTIONS 1)
##set(PICO_CXX_ENABLE_RTTI 1)

set(PARAM_ASSERTIONS_ENABLE_ALL 1)

##set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")

# Initialise the Pico SDK
pico_sdk_init()

add_subdirectory(../FreeRTOS+FAT+CLI build)

# Add executable. Default name is the project name, version 0.1
add_executable(simple_example
        simple_example.cpp
        hw_config.c
)

pico_set_program_name(simple_example "simple_example")
pico_set_program_version(simple_example "0.1")

pico_enable_stdio_uart(simple_example 1)
pico_enable_stdio_usb(simple_example 1)

# Add the standard library to the build
target_link_libraries(simple_example 
        pico_stdlib
        FreeRTOS+FAT+CLI
)
target_include_directories(simple_example PUBLIC 
        include/ 
        ../FreeRTOS+FAT+CLI/Lab-Project-FreeRTOS-FAT/include
        ../FreeRTOS+FAT+CLI/portable/RP2040
        ../FreeRTOS+FAT+CLI/include
)

#target_compile_options(simple_example PRIVATE -save-temps)
#target_compile_options(simple_example PRIVATE -Wall, -Wextra -Wshadow -Wdouble-promotion)
target_compile_options(simple_example PUBLIC -Wall -Wextra -Wno-unused-function -Wno-unused-parameter)
target_compile_definitions(simple_example PUBLIC DEBUG)

IF (NOT DEFINED N_SD_CARDS)
    SET(N_SD_CARDS 1)
ENDIF()
target_compile_definitions(simple_example PUBLIC DEBUG N_SD_CARDS=${N_SD_CARDS})

pico_add_extra_outputs(simple_example)

