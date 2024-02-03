/* big_file_test.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
//
#include "FreeRTOS.h"
#include "FreeRTOS_strerror.h"
#include "ff_utils.h"
#include "my_debug.h"
#include "task.h"
//
#include "ff_stdio.h"
//
#include "tests.h"

#ifdef NDEBUG
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

// Optimization:
//  choose this to be size of an erasable sector ("smallest erasable block"):
#define BUFFSZ (65536)  // In bytes. Should be a factor of 1 Mebibyte.

static void report(uint64_t size, uint64_t elapsed_us) {
    double elapsed = (double)elapsed_us / 1000 / 1000;
    IMSG_PRINTF("Elapsed seconds %.3g\n", elapsed);
    IMSG_PRINTF("Transfer rate ");
    if ((double)size / elapsed / 1024 / 1024 > 1.0) {
        IMSG_PRINTF("%.3g MiB/s (%.3g MB/s), or ",
                    (double)size / elapsed / 1024 / 1024,
                    (double)size / elapsed / 1000 / 1000);
    }
    IMSG_PRINTF("%.3g KiB/s (%.3g kB/s) (%.3g kbit/s)\n",
                (double)size / elapsed / 1024, (double)size / elapsed / 1000, 8.0 * size / elapsed / 1000);
}

// Create a file of size "size" bytes filled with random data seeded with "seed"
static bool create_big_file(const char *const pathname, uint64_t size,
                            unsigned seed, int *buff) {
    /* Open the file, creating the file if it does not already exist. */
    FF_Stat_t xStat;
    size_t fsz = 0;
    if (ff_stat(pathname, &xStat) == 0)
        fsz = xStat.st_size;
    FF_FILE *file_p;
    if (0 < fsz && fsz <= size) {
        // This is an attempt at optimization:
        // rewriting the file should be faster than
        // writing it from scratch.
        file_p = ff_fopen(pathname, "+");  // FF_MODE_READ | FF_MODE_WRITE
    } else {
        file_p = ff_fopen(pathname, "w");  // FF_MODE_WRITE | FF_MODE_CREATE | FF_MODE_TRUNCATE
    }
    if (!file_p) {
        EMSG_PRINTF("ff_fopen(%s): %s (%d)\n",
                    pathname, FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
        return false;
    }
    ff_rewind(file_p);

    IMSG_PRINTF("Writing...\n");

    uint64_t cum_time = 0;

    for (uint64_t i = 0; i < size / BUFFSZ; ++i) {
        for (size_t n = 0; n < BUFFSZ / sizeof(int); n++) buff[n] = rand_r(&seed);

        absolute_time_t xStart = get_absolute_time();
        size_t bw = ff_fwrite(buff, 1, BUFFSZ, file_p);
        if (bw < BUFFSZ) {
            EMSG_PRINTF("ff_fwrite(%s,,%d,): only wrote %d bytes\n", pathname, BUFFSZ, bw);
            EMSG_PRINTF("ff_fwrite: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
            ff_fclose(file_p);
            return false;
        }
        cum_time += absolute_time_diff_us(xStart, get_absolute_time());
    }
    /* Close the file */
    int rc = ff_fclose(file_p);
    if (-1 == rc) {
        EMSG_PRINTF("ff_fclose: %s\n", FreeRTOS_strerror(stdioGET_ERRNO()));
        return false;
    }
    report(size, cum_time);
    return true;
}

// Read a file of size "size" bytes filled with random data seeded with "seed"
// and verify the data
static bool check_big_file(char *pathname, uint64_t size,
                           unsigned int seed, int *buff) {
    FF_FILE *file_p = ff_fopen(pathname, "r");
    if (!file_p) {
        EMSG_PRINTF("ff_fopen: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
        return false;
    }
    IMSG_PRINTF("Reading...\n");

    uint64_t cum_time = 0;

    for (uint64_t i = 0; i < size / BUFFSZ; ++i) {
        absolute_time_t xStart = get_absolute_time();
        size_t br = ff_fread(buff, 1, BUFFSZ, file_p);
        if (br < BUFFSZ) {
            EMSG_PRINTF("ff_fread(,%s,%d,):only read %u bytes\n", pathname, BUFFSZ, br);
            EMSG_PRINTF("ff_fread: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
            ff_fclose(file_p);
            return false;
        }
        cum_time += absolute_time_diff_us(xStart, get_absolute_time());

        /* Check the buffer is filled with the expected data. */
        size_t n;
        for (n = 0; n < BUFFSZ / sizeof(int); n++) {
            int expected = rand_r(&seed);
            int val = buff[n];
            if (val != expected) {
                EMSG_PRINTF("Data mismatch at dword %llu: expected=0x%8x val=0x%8x\n",
                            (i * sizeof(buff)) + n, expected, val);
                ff_fclose(file_p);
                return false;
            }
        }
    }
    /* Close the file */
    int rc = ff_fclose(file_p);
    if (-1 == rc) {
        EMSG_PRINTF("ff_fclose: %s\n", FreeRTOS_strerror(stdioGET_ERRNO()));
        return false;
    }
    report(size, cum_time);
    return true;
}
// Specify size in Mebibytes (1024x1024 bytes)
static void run_big_file_test(char *pathname, size_t size_MiB, uint32_t seed) {
    //  /* Working buffer */
    int *buff = pvPortMalloc(BUFFSZ);
    if (!buff) {
        EMSG_PRINTF("pvPortMalloc(%zu) failed\n", BUFFSZ);
        return;
    }
    myASSERT(buff);

    myASSERT(size_MiB);
    if (4095 < size_MiB) {
        EMSG_PRINTF("Warning: Maximum file size: 2^32 - 1 bytes on FAT volume\n");
    }
    uint64_t size_B = (uint64_t)size_MiB * 1024 * 1024;

    if (create_big_file(pathname, size_B, seed, buff))
        check_big_file(pathname, size_B, seed, buff);

    vPortFree(buff);
}
typedef struct bft_args_t {
    char cwdbuf[128];
    char pathname[256];
    size_t size;
    uint32_t seed;
} bft_args_t;
static void big_file_test_task(void *vp) {
    bft_args_t *args_p = vp;
    ff_chdir(args_p->cwdbuf);
    run_big_file_test(args_p->pathname, args_p->size, args_p->seed);
    vPortFree(args_p);
    vTaskDelete(NULL);
}
void big_file_test(char const *pathname, size_t size_MiB, uint32_t seed) {
    // Can't have the args on the stack because they might
    // go away before the task starts
    bft_args_t *args_p = pvPortMalloc(sizeof(bft_args_t));
    // Each task maintains its own CWD
    char *ret = ff_getcwd(args_p->cwdbuf, sizeof args_p->cwdbuf);
    if (!ret) {
        EMSG_PRINTF("ff_getcwd failed\n");
    }
    size_t rc = strlcpy(args_p->pathname, pathname, sizeof args_p->pathname);
    assert(rc < sizeof args_p->pathname);
    args_p->size = size_MiB;
    args_p->seed = seed;
    xTaskCreate(big_file_test_task, "big_file_test", 768, args_p,
                uxTaskPriorityGet(xTaskGetCurrentTaskHandle()) - 1, NULL);
}

/* [] END OF FILE */
