/* FreeRTOS_time.c
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
#include <stdio.h>
//
#include "FreeRTOS.h"
#include "ff_time.h"
#include "timers.h"
//
#include "hardware/rtc.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/types.h"
//
#include "util.h"  // calculate_checksum
//
#include "FreeRTOS_time.h"

time_t epochtime;

typedef struct rtc_save {
    uint32_t signature;
    datetime_t datetime;
    uint32_t checksum;  // last, not included in checksum
} rtc_save_t;
static rtc_save_t rtc_save __attribute__((section(".uninitialized_data")));

static void update_epochtime() {
    bool rc = rtc_get_datetime(&rtc_save.datetime);
    if (rc) {
        rtc_save.signature = 0xBABEBABE;
        struct tm timeinfo = {
            .tm_sec = rtc_save.datetime
                          .sec,                       /* Seconds.	[0-60] (1 leap second) */
            .tm_min = rtc_save.datetime.min,          /* Minutes.	[0-59] */
            .tm_hour = rtc_save.datetime.hour,        /* Hours.	[0-23] */
            .tm_mday = rtc_save.datetime.day,         /* Day.		[1-31] */
            .tm_mon = rtc_save.datetime.month - 1,    /* Month.	[0-11] */
            .tm_year = rtc_save.datetime.year - 1900, /* Year	- 1900.  */
            .tm_wday = 0,                             /* Day of week.	[0-6] */
            .tm_yday = 0,                             /* Days in year.[0-365]	*/
            .tm_isdst = -1                            /* DST.		[-1/0/1]*/
        };
        rtc_save.checksum = calculate_checksum((uint32_t *)&rtc_save,
                                               offsetof(rtc_save_t, checksum));
        epochtime = mktime(&timeinfo);
        rtc_save.datetime.dotw = timeinfo.tm_wday;
        // configASSERT(-1 != epochtime);
    }
}

time_t FreeRTOS_time(time_t *pxTime) {
    if (pxTime) {
        *pxTime = epochtime;
    }
    return epochtime;
}

static TimerHandle_t TimeUpdateTimer;

static void TimeUpdateTimerCallback(TimerHandle_t) {
    update_epochtime();
}

void FreeRTOS_time_init() {
    rtc_init();
    {
        datetime_t t = {0, 0, 0, 0, 0, 0, 0};
        rtc_get_datetime(&t);
        if (!t.year && rtc_save.datetime.year) {
            uint32_t xor_checksum = calculate_checksum(
                (uint32_t *)&rtc_save, offsetof(rtc_save_t, checksum));
            if (rtc_save.signature == 0xBABEBABE &&
                rtc_save.checksum == xor_checksum) {
                // Set rtc
                rtc_set_datetime(&rtc_save.datetime);
            }
        }
    }
    static StaticTimer_t TimerState;
    TimeUpdateTimer = xTimerCreateStatic("TimeUpdate",             // pcTimerName
                                         pdMS_TO_TICKS(1000),      // xTimerPeriod
                                         pdTRUE,                   // uxAutoReload
                                         (void *)0,                // pvTimerID
                                         TimeUpdateTimerCallback,  // callback
                                         &TimerState);             // pxTimerBuffer
    if (rtc_running()) {
        xTimerStart(TimeUpdateTimer, portMAX_DELAY);
    }
}

bool setrtc(datetime_t *t) {
    bool ok = rtc_set_datetime(t);
    // Start callback timer if it's not already running
    if (ok && pdFALSE == xTimerIsTimerActive(TimeUpdateTimer)) {
        xTimerStart(TimeUpdateTimer, portMAX_DELAY);
    }
    return ok;
}
