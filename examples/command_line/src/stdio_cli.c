/* stdio_cli.c
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
// FreeRTOS
#include "FreeRTOS.h"
//
#include "task.h"
// Pico
#include "crash.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
//
#include "command.h"
#include "sd_card.h"
#include "task_config.h"
#include "unmounter.h"
//
#include "stdio_cli.h"

static void callback(void *ptr) {
    TaskHandle_t task = ptr;
    myASSERT(task);
    /* The xHigherPriorityTaskWoken parameter must be initialized to pdFALSE as
     it will get set to pdTRUE inside the interrupt safe API function if a
     context switch is required. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Send a notification directly to the task to which interrupt processing is
     being deferred. */
    vTaskNotifyGiveIndexedFromISR(
        task,                   // The handle of the task to which the
                                // notification is being sent.
        NOTIFICATION_IX_STDIO,  // uxIndexToNotify: The index within the target task's array of
                                // notification values to which the notification is to be sent.
        &xHigherPriorityTaskWoken);

    /* Pass the xHigherPriorityTaskWoken value into portYIELD_FROM_ISR().
    If xHigherPriorityTaskWoken was set to pdTRUE inside
     vTaskNotifyGiveFromISR() then calling portYIELD_FROM_ISR() will
     request a context switch. If xHigherPriorityTaskWoken is still
     pdFALSE then calling portYIELD_FROM_ISR() will have no effect. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// stdioTask - the function which handles input
static void stdioTask(void *arg) {
    (void)arg;

    // Check fault capture from RAM:
    crash_info_t const *const pCrashInfo = crash_handler_get_info();
    if (pCrashInfo) {
        printf("*** Fault Capture Analysis (RAM): ***\n");
        int n = 0;
        do {
            char buf[256] = {0};
            n = dump_crash_info(pCrashInfo, n, buf, sizeof(buf));
            if (buf[0]) printf("\t%s", buf);
        } while (n != 0);
    }

    // Called here to set up the GPIOs
    // before enabling the card detect interrupt:
    sd_init_driver();
    unmounter_init();

    // get notified when there are input characters available
    stdio_set_chars_available_callback(callback, xTaskGetCurrentTaskHandle());

    if (!rtc_running()) printf("RTC is not running.\n");
    datetime_t t = {0, 0, 0, 0, 0, 0, 0};
    rtc_get_datetime(&t);
    char datetime_buf[256] = {0};
    datetime_to_str(datetime_buf, sizeof(datetime_buf), &t);
    printf("%s\n", datetime_buf);
    printf("FreeRTOS+CLI> ");
    stdio_flush();

    for (;;) {
        uint32_t nv = ulTaskNotifyTakeIndexed(NOTIFICATION_IX_STDIO,  // uxIndexToWaitOn
                                              pdTRUE,                 // xClearCountOnExit
                                              portMAX_DELAY);         // xTicksToWait
        if (nv < 1) continue;

        for (;;) {
            /* Get the character from terminal */
            int cRxedChar = getchar_timeout_us(0);
            if (PICO_ERROR_TIMEOUT == cRxedChar) {
                break;
            }
            process_stdio(cRxedChar);
        }
    }
}

/* Start UART operation. */
void CLI_Start() {
    static StackType_t xStack[1024];
    static StaticTask_t xTaskBuffer;
    TaskHandle_t th =
        xTaskCreateStatic(stdioTask, "stdio Task", sizeof xStack / sizeof xStack[0], 0,
                          PRIORITY_stdioTask, /* Priority at which the task is created. */
                          xStack, &xTaskBuffer);
    configASSERT(th);
}

/* [] END OF FILE */
