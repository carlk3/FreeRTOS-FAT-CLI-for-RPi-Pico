# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
#set(PICO_SDK_PATH "/home/carlk/pi/pico/pico-sdk")

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(example C CXX ASM)

#set(PICO_CXX_ENABLE_EXCEPTIONS 1)
#set(PICO_CXX_ENABLE_RTTI 1)

set(PARAM_ASSERTIONS_ENABLE_ALL 1)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")

# Initialise the Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(example
        example.c
        tests/app4-IO_module_function_checker.c        
        tests/filesystem_test_suite.c
        tests/CreateAndVerifyExampleFiles.c        
        tests/big_file_test.c
        tests/ff_stdio_tests_with_cwd.c
        tests/my_test.c
        tests/mt_lliot.c
        data_log_demo.c
)
add_subdirectory(../FreeRTOS+FAT+CLI build)

pico_set_program_name(example "example")
pico_set_program_version(example "0.1")
#Temporary work-around until FreeRTOS is fixed:
pico_set_divider_implementation(example compiler)

pico_enable_stdio_uart(example 1)
pico_enable_stdio_usb(example 0)

# Add the standard library to the build
target_link_libraries(example pico_stdlib)
target_include_directories(example PUBLIC 
        include/ 
)
# Add any user requested libraries
target_link_libraries(example
        FreeRTOS+FAT+CLI
)

#target_compile_options(example PRIVATE -save-temps)
#target_compile_options(example PRIVATE -Wall, -Wextra -Wshadow -Wdouble-promotion)
target_compile_options(example PUBLIC -Wall -Wextra -Wno-unused-function -Wno-unused-parameter)
target_compile_definitions(example PUBLIC DEBUG)

pico_add_extra_outputs(example)

