#include "FreeRTOS.h" /* Must come first. */
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
#include "delays.h"
#include "file_stream.h"

#define DEVICE "sd0"
#define ITERATIONS 100000

static inline void stop() {
    fflush(NULL);
    exit(-1);
}

// See https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/Standard_File_System_API.html
static void unbuffered() {
    uint64_t start = micros();
    uint64_t then = start;

    FF_FILE *pxFile = ff_fopen("/" DEVICE "/times_ub.csv", "w");
    if (!pxFile) {
        printf("ff_fopen failed: %s\n", strerror(stdioGET_ERRNO()));
        stop();
    }
    for (size_t i = 0; i < ITERATIONS; ++i) {
        uint64_t now = micros();
        if (ff_fprintf(pxFile, "%llu, %llu\n", now - start, now - then) < 0) {
            printf("ff_fprintf failed: %s\n", strerror(stdioGET_ERRNO()));
            stop();
        }
        then = now;
    }    
    if (-1 == ff_fclose(pxFile)) {
        printf("ff_fclose failed: %s\n", strerror(stdioGET_ERRNO()));
        stop();
    }
    printf("Time without buffering: %.3g ms\n", (float)(micros() - start) / 1000);
}

// See https://sourceware.org/newlib/libc.html
static void buffered() {
    uint64_t start = micros();
    uint64_t then = start;

    FILE *file_p = open_file_stream("/" DEVICE "/times_b.csv", "w");
    if (!file_p) {
        //FIXME: does perror() work?
        printf("fopen failed: %s\n", strerror(stdioGET_ERRNO()));
        stop();
    }
    /* setvbuf() is optional, but alignment is critical for good
    performance with SDIO because it uses DMA with word width.
    Also, the buffer size should be a multiple of the SD block 
    size, 512 bytes. */
    static char vbuf[1024] __attribute__((aligned));
    int err = setvbuf(file_p, vbuf, _IOFBF, sizeof vbuf);
    if (err) stop();

    for (size_t i = 0; i < ITERATIONS; ++i) {
        uint64_t now = micros();
        if (fprintf(file_p, "%llu, %llu\n", now - start, now - then) < 0) {
            printf("fprintf failed: %s\n", strerror(stdioGET_ERRNO()));
            stop();
        }
        then = now;
    }    
    if (-1 == fclose(file_p)) {
        printf("fclose failed: %s\n", strerror(stdioGET_ERRNO()));
        stop();
    }
    printf("Time with buffering: %.3g ms\n", (float)(micros() - start) / 1000);
}


static void SimpleTask(void *arg) {
    (void)arg;

    printf("\n%s: Hello, world!\n", pcTaskGetName(NULL));

    FF_Disk_t *pxDisk = FF_SDDiskInit(DEVICE);
    if (!pxDisk) stop();
    FF_Error_t xError = FF_SDDiskMount(pxDisk);
    if (FF_isERR(xError) != pdFALSE) {
        printf("FF_SDDiskMount: %s\n",
                  (const char *)FF_GetErrMessage(xError));
        stop();
    }
    FF_FS_Add("/" DEVICE, pxDisk);

    unbuffered();
    buffered();

    FF_FS_Remove("/" DEVICE);
    FF_Unmount(pxDisk);
    FF_SDDiskDelete(pxDisk);
    puts("Goodbye, world!");

    vTaskDelete(NULL);
}

int main() {
    stdio_init_all();

    xTaskCreate(SimpleTask, "SimpleTask", 1024, NULL, configMAX_PRIORITIES - 1,
                NULL);
    vTaskStartScheduler();

    /* should never reach here */
    panic_unsupported();
}
