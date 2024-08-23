/* data_log_demo.c
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

#include "FreeRTOS.h" /* Must come first. */
//
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
//
#include "hardware/adc.h"
#include "pico/stdlib.h"
//
#include "ff_headers.h"
#include "ff_stdio.h"
#include "task.h"
//
//#include "sd_card.h"
#include "ff_utils.h"

#if defined(NDEBUG) || !USE_DBG_PRINTF
#  pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#define DEVICENAME "sd0"
#define MOUNTPOINT "/sd0"

#define TRACE_PRINTF(fmt, args...)
//#define TRACE_PRINTF printf

extern bool die_now;

static TaskHandle_t th;

static bool print_header(FF_FILE *pxFile) {
    configASSERT(pxFile);
    ff_fseek(pxFile, 0, FF_SEEK_END);
    if (0 == ff_ftell(pxFile)) {
        // Print header
        if (ff_fprintf(pxFile, "Date,Time,Temperature (°C)\n") < 0) {
            FAIL("ff_fprintf");
            return false;
        }
    }
    return true;
}

static FF_FILE *open_file() {
    const time_t timer = FreeRTOS_time(NULL);
    struct tm tmbuf;
    localtime_r(&timer, &tmbuf);
    char filename[64];
    //  tm_year	int	years since 1900
    //  tm_mon	int	months since January	0-11
    //  tm_mday	int	day of the month	1-31
    int n = snprintf(filename, sizeof filename, "%s/data/%04d-%02d-%02d",
                        MOUNTPOINT, tmbuf.tm_year + 1900, tmbuf.tm_mon + 1,
                        tmbuf.tm_mday);
    configASSERT(0 < n && n < (int)sizeof filename);
    if (-1 == mkdirhier(filename) &&
        stdioGET_ERRNO() != pdFREERTOS_ERRNO_EEXIST) {
        FF_FAIL("mkdirhier", filename);
        return NULL;
    }
    size_t nw = strftime(filename + n, sizeof filename - n, "/%H.csv", &tmbuf);
    configASSERT(nw);
    FF_FILE *pxFile = ff_fopen(filename, "a");
    if (!pxFile) {
        FF_FAIL("ff_fopen", filename);
        return NULL;
    }
    if (!print_header(pxFile))
        return NULL;
    return pxFile;
}

static void DemoTask(void *arg) {
    (void)arg;

    printf("%s started\n", pcTaskGetName(NULL));

    if (!mount(DEVICENAME)) goto quit;

    adc_init();
    adc_set_temp_sensor_enabled(true);

    /* The xLastWakeTime variable needs to be initialized with the current
     tick count. Note that this is the only time the variable is written to
     explicitly. After this xLastWakeTime is automatically updated within
     vTaskDelayUntil(). */
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (!die_now) {
        /* It's very inefficient to open and close the file for every record,
        but you're less likely to lose data that way. */

        FF_FILE *pxFile = open_file();
        if (!pxFile) break;

        // Form date-time string
        char buf[128];
        const time_t timer = FreeRTOS_time(NULL);
        struct tm tmbuf;
        struct tm *ptm = localtime_r(&timer, &tmbuf);
        size_t n = strftime(buf, sizeof buf, "%F,%T,", ptm);
        configASSERT(n);

        // The temperature sensor is on input 4:
        adc_select_input(4);
        uint16_t result = adc_read();
        // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
        const float conversion_factor = 3.3f / (1 << 12);
        float voltage = conversion_factor * result;
        TRACE_PRINTF("Raw value: 0x%03x, voltage: %f V\n", result,
                     (double)voltage);

        // Temperature sensor values can be approximated in centigrade as:
        //    T = 27 - (ADC_Voltage - 0.706)/0.001721
        float Tc = 27.0f - (voltage - 0.706f) / 0.001721f;
        TRACE_PRINTF("Temperature: %.1f °C\n", (double)Tc);
        int nw = snprintf(buf + n, sizeof buf - n, "%.3g\n", (double)Tc);
        configASSERT(0 < nw && nw < (int)sizeof buf);

        if (ff_fprintf(pxFile, "%s", buf) < 0) {
            FAIL("ff_fprintf");
            break;
        }
        if (-1 == ff_fclose(pxFile)) {
            FAIL("ff_fclose");
            break;
        }

        /* This task should execute every 1000 milliseconds exactly (once
         per second). As per the vTaskDelay() function, time is measured in
         ticks, and the pdMS_TO_TICKS() macro is used to convert
         milliseconds into ticks. xLastWakeTime is automatically updated
         within vTaskDelayUntil(), so is not explicitly updated by the task. */

        // BaseType_t xTaskDelayUntil( TickType_t *pxPreviousWakeTime, const
        // TickType_t xTimeIncrement );
        /* Delay a task until a specified time.  This function can be used
        by periodic  tasks to ensure a constant execution frequency. Whereas
        vTaskDelay () specifies a wake time relative to the time at which
        the function is called, xTaskDelayUntil () specifies the absolute
        (exact) time at which it wishes to unblock.  */
        BaseType_t xWasDelayed =
            xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
        if (!xWasDelayed)
            task_printf("%s is behind schedule\n", pcTaskGetName(NULL));
    }
quit:
    printf("%s ending\n", pcTaskGetName(NULL));
    th = NULL;
    vTaskDelete(NULL);
}

void data_log_demo() {
    static StackType_t xStack[768];
    static StaticTask_t xTaskBuffer;
    if (!th) {
        th = xTaskCreateStatic(
            DemoTask, "DemoTask", sizeof xStack / sizeof xStack[0], 0,
            configMAX_PRIORITIES - 1, /* Priority at which the task is created. */
            xStack, &xTaskBuffer);
    }
    configASSERT(th);
}
