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
#include <stdbool.h>
#include <time.h>
//
#include "FreeRTOS.h"
#include "ff_time.h"
#include "timers.h"
//
#include "pico/aon_timer.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#if HAS_RP2040_RTC
#  include "hardware/rtc.h"
#endif
//
#include "crc.h"
//
#include "FreeRTOS_time.h"

time_t epochtime;

// Make an attempt to save a recent time stamp across reset:
typedef struct rtc_save {
    uint32_t signature;
    struct timespec ts;
    char checksum;  // last, not included in checksum
} rtc_save_t;
static rtc_save_t rtc_save __attribute__((section(".uninitialized_data")));

static bool get_time(struct timespec *ts) {
    if (!aon_timer_is_running()) return false;
    aon_timer_get_time(ts);
    return true;
}

/**
 * @brief Update the epochtime variable from the always-on timer
 *
 * If the always-on timer is running, copy its current value to the epochtime variable.
 * Also, copy the current always-on timer value to the rtc_save structure and
 * calculate a checksum of the structure.
 */
static void update_epochtime() {
    bool ok = get_time(&rtc_save.ts);
    if (!ok) return;
    // Set the signature to the magic number
    rtc_save.signature = 0xBABEBABE;
    // Calculate the checksum of the structure
    rtc_save.checksum = crc7((uint8_t *)&rtc_save, offsetof(rtc_save_t, checksum));
    // Copy the seconds part of the always-on timer to the epochtime variable
    epochtime = rtc_save.ts.tv_sec;
}

/**
 * @brief Get the current time in seconds since the Epoch.
 *
 * @param[in] pxTime If not NULL, the current time is copied here.
 * @return The current time in seconds since the Epoch.
 */
time_t FreeRTOS_time(time_t *pxTime) {
    if (pxTime) {
        *pxTime = epochtime;
    }
    return epochtime;
}
time_t time(time_t *pxTime) {
    return FreeRTOS_time(pxTime);
}

static TimerHandle_t TimeUpdateTimer;
static void TimeUpdateTimerCallback(TimerHandle_t) { update_epochtime(); }

/**
 * @brief Initialize the FreeRTOS_time system
 *
 * @details This function is called once during system initialization.
 *          It initializes the always-on timer and sets up a repeating timer
 *          to update the epochtime variable every second. If the always-on
 *          timer is already running, this function does nothing.
 */
void FreeRTOS_time_init() {
    // If the always-on timer is already running, return immediately
    if (aon_timer_is_running()) return;

    // Initialize the RTC if it is available
#if HAS_RP2040_RTC
    rtc_init();
#endif

    // Check if the saved time is valid
    char xor_checksum = crc7((uint8_t *)&rtc_save, offsetof(rtc_save_t, checksum));
    bool ok = rtc_save.signature == 0xBABEBABE && rtc_save.checksum == xor_checksum;

    // If the saved time is valid, set the always-on timer
    if (ok) aon_timer_set_time(&rtc_save.ts);

    // Set up a repeating timer to update the epochtime variable every second
    static StaticTimer_t TimerState;
    TimeUpdateTimer = xTimerCreateStatic("TimeUpdate",             // pcTimerName
                                         pdMS_TO_TICKS(1000),      // xTimerPeriod
                                         pdTRUE,                   // uxAutoReload
                                         (void *)0,                // pvTimerID
                                         TimeUpdateTimerCallback,  // callback
                                         &TimerState);             // pxTimerBuffer
    if (aon_timer_is_running()) xTimerStart(TimeUpdateTimer, portMAX_DELAY);
}

/**
 * @brief Set the current time from a timespec structure
 *
 * @details This function sets the current time using the always-on timer.
 *          If the callback timer is not already running, it starts a repeating
 *          timer to update the epochtime variable every second.
 *
 * @param[in] ts_p Pointer to a timespec structure containing the new time
 */
void setrtc(struct timespec *ts_p) {
    aon_timer_set_time(ts_p);
    // Start callback timer if it's not already running
    if (pdFALSE == xTimerIsTimerActive(TimeUpdateTimer)) {
        xTimerStart(TimeUpdateTimer, portMAX_DELAY);
    }
}
