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
#include "pico/util/datetime.h"
//
#include "command.h"
#include "sd_card.h"
#include "task_config.h"
#include "unmounter.h"
//
#include "stdio_cli.h"

#if defined(NDEBUG) || !USE_DBG_PRINTF
#  pragma GCC diagnostic ignored "-Wunused-variable"
#endif

/**
 * \brief Callback function for stdio interrupts.
 *
 * This function is called in interrupt context whenever a character is
 * available on the console input. It notifies the stdio task that a character
 * is available and requests a context switch if the stdio task is in a
 * different priority than the interrupted task.
 *
 * \param ptr The handle of the task to which interrupt processing is being
 * deferred.
 */
static void callback(void *ptr) {
    TaskHandle_t task = ptr;
    myASSERT(task);
    /* The xHigherPriorityTaskWoken parameter must be initialized to pdFALSE as
     it will get set to pdTRUE inside the interrupt safe API function if a
     context switch is required. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Send a notification directly to the task to which interrupt processing is
     being deferred. This notification will wake the task if it is in the
     blocked state. */
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

/**
 * \brief stdioTask - the function which handles input
 *
 * This function is started by \ref CLI_Start() and implements the
 * FreeRTOS+CLI command line interface. It waits for characters to be
 * available on the console, processes them, and displays the prompt
 * when ready for more input.
 *
 * \param arg Unused parameter
 */
static void stdioTask(void *arg) {
    (void)arg;

    // Clear the screen
    printf("\033[2J\033[H");
    stdio_flush();

    // Check for a fault capture from RAM:
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

    // Init the SD card driver and unmounter
    // NOTE: sd_init_driver is called here to set up the GPIOs
    //   before enabling the card detect interrupt:
    sd_init_driver();
    unmounter_init();

    // Get notified when there are input characters available
    stdio_set_chars_available_callback(callback, xTaskGetCurrentTaskHandle());

    // Display the prompt
    printf("FreeRTOS+CLI> ");
    stdio_flush();

    for (;;) {
        uint32_t nv = ulTaskNotifyTakeIndexed(NOTIFICATION_IX_STDIO,  // uxIndexToWaitOn
                                              pdTRUE,                 // xClearCountOnExit
                                              portMAX_DELAY);         // xTicksToWait
        if (nv < 1) continue;

        // Process all available characters
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

/**
 * @brief Start the CLI task.
 *
 * This function initializes the CLI task and starts its execution. The CLI task
 * is responsible for handling input from the UART and executing the
 * corresponding commands.
 *
 * @note This function does not return until the CLI task is deleted.
 *
 * @return None.
 */
void CLI_Start() {
    // Define the stack size for the CLI task.
    static StackType_t xStack[1024];

    // Define the task buffer for the CLI task.
    static StaticTask_t xTaskBuffer;

    // Create the CLI task.
    TaskHandle_t th = xTaskCreateStatic(
        stdioTask,                    // Pointer to the task function.
        "stdio Task",                 // Name of the task.
        sizeof xStack / sizeof xStack[0], // Stack size of the task.
        0,                            // Parameter to pass to the task.
        PRIORITY_stdioTask,           // Priority at which the task is created.
        xStack,                       // Pointer to the stack.
        &xTaskBuffer                  // Pointer to the task buffer.
    );

    // Assert that the task handle is not NULL. If it is NULL, there was an error
    // in creating the task.
    configASSERT(th);
}

/* [] END OF FILE */
