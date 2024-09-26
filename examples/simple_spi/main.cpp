#include "FreeRTOS.h"
//
#include <stdio.h>
//
#include "pico/stdlib.h"
//
#include "ff_headers.h"
#include "ff_sddisk.h"
#include "ff_stdio.h"
#include "ff_utils.h"
//
#include "hw_config.h"

/**
 * @brief Aborts the program when called.
 *
 * @note This function should never return.
 */
static inline void stop() {
    fflush(stdout);
    abort();
}

/**
 * @brief Task to demonstrate the usage of the FreeRTOS+FAT library.
 *
 * This task opens a file on the SD card, writes a message to it, and then
 * closes the file. It then unmounts the SD card and deletes the disk.
 * 
 * See https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/Standard_File_System_API.html
 *
 * @param arg Unused argument.
 */
static void SimpleTask(void *arg) {
    (void)arg;

    printf("\n%s: Hello, world!\n", pcTaskGetName(NULL));

    // Initialize the SD card disk
    FF_Disk_t *pxDisk = FF_SDDiskInit("sd0");
    configASSERT(pxDisk);
    FF_Error_t xError = FF_SDDiskMount(pxDisk);
    if (FF_isERR(xError) != pdFALSE) {
        FF_PRINTF("FF_SDDiskMount: %s\n",
                  (const char *)FF_GetErrMessage(xError));
        stop();
    }
    FF_FS_Add("/sd0", pxDisk);

    // Open a file on the SD card
    FF_FILE *pxFile = ff_fopen("/sd0/filename.txt", "a");
    if (!pxFile) {
        FF_PRINTF("ff_fopen failed: %s (%d)\n", strerror(stdioGET_ERRNO()),
                  stdioGET_ERRNO());
        stop();
    }
    // Write a message to the file
    if (ff_fprintf(pxFile, "Hello, world!\n") < 0) {
        FF_PRINTF("ff_fprintf failed: %s (%d)\n", strerror(stdioGET_ERRNO()),
                  stdioGET_ERRNO());
        stop();
    }
    // Close the file
    if (-1 == ff_fclose(pxFile)) {
        FF_PRINTF("ff_fclose failed: %s (%d)\n", strerror(stdioGET_ERRNO()),
                  stdioGET_ERRNO());
        stop();
    }
    // Unmount the SD card
    FF_FS_Remove("/sd0");
    FF_Unmount(pxDisk);
    // Delete the SD card disk
    FF_SDDiskDelete(pxDisk);

    puts("Goodbye, world!");

    vTaskDelete(NULL);
}
/**
 * @brief The main entry point for the program.
 *
 * The main function initializes the stdio with `stdio_init_all()`, creates a
 * task with `xTaskCreate()`, and starts the scheduler with
 * `vTaskStartScheduler()`.
 */
int main() {
    // Initialize the stdio.
    stdio_init_all();

    // Create a task with the highest priority.
    xTaskCreate(SimpleTask, "SimpleTask", 1024, NULL, configMAX_PRIORITIES - 1,
                NULL);

    // Start the scheduler.
    vTaskStartScheduler();

    /* The program should never reach here. */
    panic_unsupported();
}
