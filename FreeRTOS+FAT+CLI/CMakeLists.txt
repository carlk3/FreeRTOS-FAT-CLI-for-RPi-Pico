add_library(FreeRTOS+FAT+CLI INTERFACE)
target_sources(FreeRTOS+FAT+CLI INTERFACE
        FreeRTOS-Kernel/event_groups.c
        FreeRTOS-Kernel/list.c
        FreeRTOS-Kernel/queue.c
        FreeRTOS-Kernel/stream_buffer.c
        FreeRTOS-Kernel/tasks.c
        FreeRTOS-Kernel/timers.c 
        #FreeRTOS-Kernel/portable/GCC/ARM_CM0_RP2040/port.c 
        FreeRTOS-Kernel/portable/GCC/ARM_CM0/port.c 
        FreeRTOS-Kernel/portable/MemMang/heap_4.c 
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
        portable/RP2040/sd_spi.c
        portable/RP2040/demo_logging.c
        portable/RP2040/ff_sddisk.c
        portable/RP2040/hw_config.c
        portable/RP2040/spi.c
        portable/RP2040/sd_card.c
        portable/RP2040/crc.c
        src/my_debug.c
        src/FreeRTOS_time.c
        src/stdio_cli.c
        src/crash.c
        src/CLI-commands.c
        src/ff_utils.c
        src/File-related-CLI-commands.c
        src/FreeRTOS_CLI.c
)
target_link_libraries(FreeRTOS+FAT+CLI INTERFACE
        pico_multicore
        hardware_adc
        hardware_spi
        hardware_dma
        hardware_clocks
        hardware_rtc
        hardware_timer
        pico_stdlib
)
target_include_directories(FreeRTOS+FAT+CLI INTERFACE 
        include/ 
        FreeRTOS-Kernel/include 
        FreeRTOS-Kernel/portable/GCC/ARM_CM0 #_RP2040
        Lab-Project-FreeRTOS-FAT/include/
        portable/RP2040/
)