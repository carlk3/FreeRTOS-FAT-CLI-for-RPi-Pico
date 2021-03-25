# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
#set(PICO_SDK_PATH "/home/carlk/pi/pico/pico-sdk")

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(FreeRTOS+FAT+CLI C CXX ASM)

#set(PICO_CXX_ENABLE_EXCEPTIONS 1)

#set(PICO_CXX_ENABLE_RTTI 1)

set(PARAM_ASSERTIONS_ENABLE_ALL 1)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")

# Initialise the Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(FreeRTOS+FAT+CLI 
        FreeRTOS+FAT+CLI.c
        src/FreeRTOS_time.c
        src/my_debug.c
        src/crash.c
        src/FreeRTOS_CLI.c
        src/CLI-commands.c
        src/File-related-CLI-commands.c
        src/ff_utils.c
        portable/RP2040/hw_config.c
        portable/RP2040/spi.c
        portable/RP2040/sd_spi.c       
        portable/RP2040/sd_card.c       
        portable/RP2040/ff_sddisk.c       
        portable/RP2040/demo_logging.c
        portable/RP2040/crc.c
        src/stdio_cli.c
        src/tests/app4-IO_module_function_checker.c        
        src/tests/filesystem_test_suite.c
        src/tests/CreateAndVerifyExampleFiles.c        
        src/tests/big_file_test.c
        src/tests/ff_stdio_tests_with_cwd.c
        src/tests/my_test.c
        src/tests/mt_lliot.c
        src/data_log_demo.c
)
pico_set_program_name(FreeRTOS+FAT+CLI "FreeRTOS+FAT+CLI")
pico_set_program_version(FreeRTOS+FAT+CLI "0.1")
#Temporary work-around until FreeRTOS is fixed:
pico_set_divider_implementation(FreeRTOS+FAT+CLI compiler)

pico_enable_stdio_uart(FreeRTOS+FAT+CLI 1)
pico_enable_stdio_usb(FreeRTOS+FAT+CLI 0)
# Add the standard library to the build
target_link_libraries(FreeRTOS+FAT+CLI pico_stdlib)

add_library(FreeRTOS-Kernel STATIC 
        FreeRTOS-Kernel/event_groups.c
        FreeRTOS-Kernel/list.c
        FreeRTOS-Kernel/queue.c
        FreeRTOS-Kernel/stream_buffer.c
        FreeRTOS-Kernel/tasks.c
        FreeRTOS-Kernel/timers.c 
        #FreeRTOS-Kernel/portable/GCC/ARM_CM0_RP2040/port.c 
        FreeRTOS-Kernel/portable/GCC/ARM_CM0/port.c 
        FreeRTOS-Kernel/portable/MemMang/heap_4.c 
        src/FreeRTOS_Support_Files/IdleMemory.c
)
target_link_libraries(FreeRTOS-Kernel pico_stdlib)

add_library(Lab-Project-FreeRTOS-FAT STATIC
        Lab-Project-FreeRTOS-FAT/ff_crc.c
        Lab-Project-FreeRTOS-FAT/ff_dir.c
        Lab-Project-FreeRTOS-FAT/ff_error.c
        Lab-Project-FreeRTOS-FAT/ff_fat.c
        Lab-Project-FreeRTOS-FAT/ff_file.c
        Lab-Project-FreeRTOS-FAT/ff_format.c
        Lab-Project-FreeRTOS-FAT/ff_ioman.c
        Lab-Project-FreeRTOS-FAT/ff_locking.c
        Lab-Project-FreeRTOS-FAT/ff_memory.c
        Lab-Project-FreeRTOS-FAT/ff_stdio.c
        Lab-Project-FreeRTOS-FAT/ff_string.c
        Lab-Project-FreeRTOS-FAT/ff_sys.c
        Lab-Project-FreeRTOS-FAT/ff_time.c        
)
target_link_libraries(Lab-Project-FreeRTOS-FAT 
        pico_multicore
        hardware_adc
        hardware_spi
        hardware_dma
        hardware_clocks
        hardware_rtc
        hardware_timer
        pico_stdlib
)

target_include_directories(FreeRTOS-Kernel PUBLIC 
        include/ 
        FreeRTOS-Kernel/include 
        FreeRTOS-Kernel/portable/GCC/ARM_CM0 #_RP2040
)
target_include_directories(Lab-Project-FreeRTOS-FAT PUBLIC 
        include/ 
        FreeRTOS-Kernel/include 
        FreeRTOS-Kernel/portable/GCC/ARM_CM0 #_RP2040
        Lab-Project-FreeRTOS-FAT/include/
)
target_include_directories(FreeRTOS+FAT+CLI PUBLIC 
        include/ 
        FreeRTOS-Kernel/include 
        FreeRTOS-Kernel/portable/GCC/ARM_CM0 #_RP2040
        Lab-Project-FreeRTOS-FAT/include/
        portable/RP2040/
)

# Add any user requested libraries
target_link_libraries(FreeRTOS+FAT+CLI
        Lab-Project-FreeRTOS-FAT
        FreeRTOS-Kernel
        )

#target_compile_options(FreeRTOS+FAT+CLI PRIVATE -save-temps)
#target_compile_options(FreeRTOS+FAT+CLI PRIVATE -Wall, -Wextra -Wshadow -Wdouble-promotion)
target_compile_options(FreeRTOS+FAT+CLI PUBLIC -Wall -Wextra -Wno-unused-function -Wno-unused-parameter)
target_compile_definitions(FreeRTOS+FAT+CLI PUBLIC DEBUG)

pico_add_extra_outputs(FreeRTOS+FAT+CLI)

