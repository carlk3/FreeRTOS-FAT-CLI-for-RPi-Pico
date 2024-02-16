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
/* This test simultaneously writes to multiple files,
then simultaneously reads the files and verifies the contents.

    void mtbft(const size_t parallelism, const size_t size_MiB,
            const char *pathnames[parallelism]);

It starts the number of tasks specifed in "parallelism";
one for each of the files specified in the "pathnames" array.

The "pathnames" must each be an absolute path to a file
and can be on multiple different "drives"
(as determined by the "mount points" in the pathnames).
 */

#include <stdint.h>
#include <string.h>
//
#include "FreeRTOS_strerror.h"
#include "my_debug.h"
//
#include "FreeRTOS.h"
#include "event_groups.h"
#include "ff_stdio.h"
#include "ff_utils.h"
//
#include "tests.h"

// Optimization:
//  Choose this to be the size (or at least a factor of the size)
//  of an erasable sector ("smallest erasable block"):
// #define BUFFSZ (32768)  // In bytes. Should be a factor of 1 Mebibyte.
// TODO: Divide, say, 64 kiB by the number of tasks.
#define BUFFSZ (16384)  // In bytes. Should be a factor of 1 Mebibyte.

typedef struct task_args_t {
    // Inputs
    size_t task_ix;  // local task index
    TaskHandle_t task_hdl;
    const char *pathname;
    uint64_t file_size;
    unsigned int seed;
    // Output:
    bool ok;
} task_args_t;

static void report(const char *title, uint64_t size, int64_t elapsed_us) {
    double elapsed = (double)elapsed_us / 1000 / 1000;
    IMSG_PRINTF("%s: Elapsed seconds %.3g\n", title, elapsed);
    if ((double)size / elapsed / 1024 / 1024 > 1.0) {
        IMSG_PRINTF("%s: Transfer rate %.3g MiB/s (%.3g MB/s)\n",
                    title,
                    (double)size / elapsed / 1024 / 1024,
                    (double)size / elapsed / 1000 / 1000);
    } else {
        IMSG_PRINTF("%s: Transfer rate %.3g KiB/s (%.3g kB/s) (%.3g kb/s)\n",
                    title,
                    (double)size / elapsed / 1024,
                    (double)size / elapsed / 1000,
                    8.0 * size / elapsed / 1000);
    }
}

/* Create a file of size "size" bytes */
static bool create_big_file(task_args_t *args_p, int *buf, FF_FILE *file_p) {
    unsigned int rand_st = args_p->seed;
    ff_rewind(file_p);
    task_printf("Writing...\n");
    absolute_time_t xStart = get_absolute_time();
    for (uint64_t i = 0; i < args_p->file_size / BUFFSZ; ++i) {
        /* Fast way to fill buffer */
        int fill = rand_r(&rand_st);
        memset(buf, fill, BUFFSZ);
        size_t bw = ff_fwrite(buf, 1, BUFFSZ, file_p);
        if (bw < BUFFSZ) {
            task_printf("ff_fwrite(,,%d,): only wrote %d bytes\n", BUFFSZ, bw);
            task_printf("ff_fwrite: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
            return false;
        }
    }
    int64_t elapsed = absolute_time_diff_us(xStart, get_absolute_time());
    report(pcTaskGetName(NULL), args_p->file_size, elapsed);
    return true;
}

// Read a file of size "size" bytes filled with random data seeded with "seed"
// and verify the data
static bool check_big_file(task_args_t *args_p, int *buf, FF_FILE *file_p) {
    unsigned int rand_st = args_p->seed;

    ff_rewind(file_p);
    task_printf("Reading...\n");
    absolute_time_t xStart = get_absolute_time();
    for (uint64_t i = 0; i < args_p->file_size / BUFFSZ; ++i) {
        size_t br = ff_fread(buf, 1, BUFFSZ, file_p);
        if (br < BUFFSZ) {
            task_printf("ff_fread(,,%d,):only read %u bytes\n", BUFFSZ, br);
            task_printf("ff_fread: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
            return false;
        }

        /* Check the buffer is filled with the expected data. */
        int fill = rand_r(&rand_st);
        for (size_t n = 0; n < BUFFSZ; ++n) {
            uint8_t expected = fill;
            uint8_t val = ((uint8_t *)buf)[n];
            if (val != expected) {
                task_printf("Data mismatch at byte %llu: expected=0x%8x val=0x%8x\n",
                            (i * sizeof(buf)) + n, expected, val);
                return false;
            }
        }
    }
    int64_t elapsed = absolute_time_diff_us(xStart, get_absolute_time());
    report(pcTaskGetName(NULL), args_p->file_size, elapsed);
    return true;
}

/* Declare a variable to hold the handle of the created event group. */
static EventGroupHandle_t main_sequence_evt_grp, task_gate_evt_grp;

static void pace(task_args_t *args_p, int *buf, FF_FILE *file_p) {
    // Notify the main task that this task is ready
    xEventGroupSetBits(main_sequence_evt_grp, /* The event group being updated. */
                       1 << args_p->task_ix); /* The bits being set. */

    // Wait for signal to start writing
    xEventGroupWaitBits(
        task_gate_evt_grp,    /* The event group being tested. */
        1 << args_p->task_ix, /* The bits within the event group to wait for. */
        pdTRUE,               /* xClearOnExit */
        pdFALSE,              /* xWaitForAllBits */
        pdMS_TO_TICKS(5000));
    // Write
    if (args_p->ok) {
        args_p->ok = create_big_file(args_p, buf, file_p);
    }
    // Notify main task that this task is done writing
    xEventGroupSetBits(main_sequence_evt_grp, /* The event group being updated. */
                       1 << args_p->task_ix); /* The bits being set. */

    // Wait for signal to start reading
    xEventGroupWaitBits(
        task_gate_evt_grp,    /* The event group being tested. */
        1 << args_p->task_ix, /* The bits within the event group to wait for. */
        pdTRUE,               /* xClearOnExit */
        pdFALSE,              /* xWaitForAllBits */
        portMAX_DELAY);
    // Read
    if (args_p->ok) {
        args_p->ok = check_big_file(args_p, buf, file_p);
    }
    // Notify main task that this task is done reading
    xEventGroupSetBits(main_sequence_evt_grp, /* The event group being updated. */
                       1 << args_p->task_ix); /* The bits being set. */
}

static void open_close(task_args_t *args_p, int *buf) {
    FF_FILE *file_p = NULL;
    if (args_p->ok) {
        /* Open the file, creating the file if it does not already exist. */
        FF_Stat_t xStat;
        size_t fsz = 0;
        if (ff_stat(args_p->pathname, &xStat) == 0)
            fsz = xStat.st_size;
        if (0 < fsz && fsz <= args_p->file_size) {
            // This is an attempt at optimization:
            // rewriting the file should be faster than
            // writing it from scratch.
            file_p = ff_fopen(args_p->pathname, "+");  // FF_MODE_READ|FF_MODE_WRITE
        } else {
            file_p = ff_fopen(args_p->pathname, "w+");  // FF_MODE_READ|FF_MODE_WRITE|FF_MODE_CREATE|FF_MODE_TRUNCATE
        }
        if (!file_p) {
            EMSG_PRINTF("ff_fopen(%s,): %s (%d)\n", args_p->pathname,
                        FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
            args_p->ok = false;
        }
    }

    pace(args_p, buf, file_p);

    /* Close the file */
    if (file_p) {
        int rc = ff_fclose(file_p);
        if (-1 == rc) {
            EMSG_PRINTF("ff_fclose: %s\n", FreeRTOS_strerror(stdioGET_ERRNO()));
        }
    }
}

static void Task(void *arg) {
    task_args_t *args_p = arg;

    /* Working buffer */
    int *buf = pvPortMalloc(BUFFSZ);
    if (!buf) {
        task_printf("pvPortMalloc(%zu) failed\n", BUFFSZ);
        args_p->ok = false;
    }
    open_close(args_p, buf);
    vPortFree(buf);
    args_p->task_hdl = NULL;
    vTaskDelete(NULL);
}

void killall(const size_t parallelism, task_args_t args_a[parallelism]) {
    for (size_t i = 0; i < parallelism; ++i) {
        if (args_a[i].task_hdl) {
            args_a[i].task_hdl = NULL;
            vTaskDelete(args_a[i].task_hdl);
        }
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstack-usage="
// Specify size in Mebibytes (1024x1024 bytes)
void mtbft(const size_t parallelism, const size_t size_MiB,
           const char *pathnames[]) {
    myASSERT(size_MiB);
    myASSERT(parallelism <= 10); // Arbitrary limit

    task_args_t args_a[parallelism];

    memset(args_a, 0, sizeof args_a);

    if (4095 < size_MiB) {
        task_printf("Warning: Maximum file size: 2^32 - 1 bytes on FAT volume\n");
    }

    uint64_t size_B = (uint64_t)size_MiB * 1024 * 1024;

    {
        /* Declare a variable to hold the data associated with the created
        event group. */
        static StaticEventGroup_t xCreatedEventGroup;
        // if (!main_sequence_evt_grp)
        /* Attempt to create the event group. */
        main_sequence_evt_grp = xEventGroupCreateStatic(&xCreatedEventGroup);
        configASSERT(main_sequence_evt_grp);
    }
    {
        /* Declare a variable to hold the data associated with the created
        event group. */
        static StaticEventGroup_t xCreatedEventGroup;
        // if (!task_gate_evt_grp)
        /* Attempt to create the event group. */
        task_gate_evt_grp = xEventGroupCreateStatic(&xCreatedEventGroup);
        configASSERT(main_sequence_evt_grp);
    }
    unsigned int rand_st = xTaskGetTickCount();
    EventBits_t tasks_mask = 0;

    for (size_t i = 0; i < parallelism; ++i) {
        args_a[i].task_ix = i;
        args_a[i].pathname = pathnames[i];
        args_a[i].file_size = size_B;
        args_a[i].seed = rand_r(&rand_st);
        args_a[i].ok = true;

        xTaskCreate(Task, pathnames[i], 768, &args_a[i],
                    uxTaskPriorityGet(xTaskGetCurrentTaskHandle()) - 1, &args_a[i].task_hdl);

        tasks_mask |= 1 << i;
    }

    // Wait for tasks to become ready
    EventBits_t bits = xEventGroupWaitBits(
        main_sequence_evt_grp, /* The event group being tested. */
        tasks_mask,            /* The bits within the event group to wait for. */
        pdTRUE,                /* xClearOnExit */
        pdTRUE,                /* xWaitForAllBits */
        pdMS_TO_TICKS(5000));  /* Timeout, ms */
    if (bits != tasks_mask) {
        EMSG_PRINTF("Timed out waiting for tasks to start\n");
        killall(parallelism, args_a);
        return;
    }
    absolute_time_t xStart = get_absolute_time();
    // Release the tasks
    xEventGroupSetBits(task_gate_evt_grp, tasks_mask);
    // Wait for subtasks to complete write
    bits = xEventGroupWaitBits(
        main_sequence_evt_grp, /* The event group being tested. */
        tasks_mask,            /* The bits within the event group to wait for. */
        pdTRUE,                /* xClearOnExit */
        pdTRUE,                /* xWaitForAllBits */
        portMAX_DELAY);        /* Timeout, ms */
    if (bits != tasks_mask) {
        EMSG_PRINTF("Timed out waiting for tasks to finish writing\n");
        killall(parallelism, args_a);
        return;
    }
    int64_t elapsed = absolute_time_diff_us(xStart, get_absolute_time());
    uint64_t bytes_xfered = 0;
    for (size_t i = 0; i < parallelism; ++i)
        if (args_a[i].ok)
            bytes_xfered += args_a[i].file_size;
    if (bytes_xfered)
        report("Summary", bytes_xfered, elapsed);

    xStart = get_absolute_time();
    // Release the tasks
    xEventGroupSetBits(task_gate_evt_grp, tasks_mask);
    // Wait for all tasks to complete read
    bits = xEventGroupWaitBits(
        main_sequence_evt_grp, /* The event group being tested. */
        tasks_mask,            /* The bits within the event group to wait for. */
        pdTRUE,                /* xClearOnExit */
        pdTRUE,                /* xWaitForAllBits */
        portMAX_DELAY);        /* Timeout, ms */
    if (bits != tasks_mask) {
        EMSG_PRINTF("Timed out waiting for subtasks to complete read\n");
        killall(parallelism, args_a);
        return;
    }
    elapsed = absolute_time_diff_us(xStart, get_absolute_time());
    bytes_xfered = 0;
    for (size_t i = 0; i < parallelism; ++i)
        if (args_a[i].ok)
            bytes_xfered += args_a[i].file_size;
    if (bytes_xfered)
        report("Summary", bytes_xfered, elapsed);
}
#pragma GCC diagnostic pop

/* [] END OF FILE */
