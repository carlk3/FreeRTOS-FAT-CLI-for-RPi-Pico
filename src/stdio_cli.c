/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// FreeRTOS
#include "FreeRTOS.h"
//
#include "FreeRTOS_time.h"
#include "queue.h"
#include "task.h"
// Pico
#include "pico/stdlib.h"
//
#include "hardware/irq.h"
#include "hardware/rtc.h"
#include "pico/error.h"
#include "pico/multicore.h"
#include "pico/stdio.h"
#include "pico/util/datetime.h"
//
#include "CLI-commands.h"
#include "File-related-CLI-commands.h"
#include "FreeRTOS_CLI.h"
#include "crash.h"
#include "filesystem_test_suite.h"
#include "stdio_cli.h"

//#define TRACE_PRINTF(fmt, args...)
#define TRACE_PRINTF printf  // task_printf

static QueueHandle_t xQueue;

/* Offload USB polling to the other processor so it doesn't steal all our
 * cycles. */

void core1_entry() {
    for (;;) {
        int cRxedChar = getchar_timeout_us(1000 * 1000);
        /* Get the character from terminal */
        if (PICO_ERROR_TIMEOUT == cRxedChar) continue;
        printf("%c", cRxedChar);  // echo
        stdio_flush();

        multicore_fifo_push_blocking(cRxedChar);
    }
}

void core0_sio_irq() {
    multicore_fifo_clear_irq();
    /* The xHigherPriorityTaskWoken parameter must be initialized to pdFALSE as
     it will get set to pdTRUE inside the interrupt safe API function if a
     context switch is required. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    configASSERT(xQueue);

    while (multicore_fifo_rvalid()) {
        int cRxedChar = multicore_fifo_pop_blocking();
        xQueueSendFromISR(xQueue, &cRxedChar, &xHigherPriorityTaskWoken);
    }

    /* Pass the xHigherPriorityTaskWoken value into portYIELD_FROM_ISR(). If
     xHigherPriorityTaskWoken was set to pdTRUE inside vTaskNotifyGiveFromISR()
     then calling portYIELD_FROM_ISR() will request a context switch. If
     xHigherPriorityTaskWoken is still pdFALSE then calling
     portYIELD_FROM_ISR() will have no effect. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// stdioTask - the function which handles input
static void stdioTask(void *arg) {
    (void)arg;

    size_t cInputIndex = 0;
    static char cOutputString[cmdMAX_OUTPUT_SIZE] = {0};
    static char cInputString[cmdMAX_INPUT_SIZE] = {0};
    BaseType_t xMoreDataToFollow = 0;
    bool in_overflow = false;

    multicore_launch_core1(core1_entry);
    irq_set_exclusive_handler(SIO_IRQ_PROC0, core0_sio_irq);

    /* Any interrupt that uses interrupt-safe FreeRTOS API functions ust also
     * execute at the priority defined by configKERNEL_INTERRUPT_PRIORITY. */
    irq_set_priority(SIO_IRQ_PROC0, 0xFF);  // Lowest urgency.
    irq_set_enabled(SIO_IRQ_PROC0, true);

    for (;;) {
        // int cRxedChar = getchar_timeout_us(1000 * 1000);
        ///* Get the character from terminal */
        // if (PICO_ERROR_TIMEOUT == cRxedChar) {
        //    continue;
        //}
        // printf("%c", cRxedChar);  // echo
        // stdio_flush();

        int cRxedChar;
        xQueueReceive(xQueue, &cRxedChar, portMAX_DELAY);

        static bool first = true;
        if (first) {
            printf("\033[2J\033[H");  // Clear Screen

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
            if (!rtc_running()) printf("RTC is not running.\n");
            datetime_t t = {0, 0, 0, 0, 0, 0, 0};
            rtc_get_datetime(&t);
            char datetime_buf[256] = {0};
            datetime_to_str(datetime_buf, sizeof(datetime_buf), &t);
            printf("%s\n", datetime_buf);
            printf("FreeRTOS+CLI> ");
            stdio_flush();

            first = false;
        }

        /* Newline characters are taken as the end of the command
         string. */
        if (cRxedChar == '\n' || cRxedChar == '\r') {
            in_overflow = false;

            TickType_t xStart = 0;
            /* Just to space the output from the input. */
            printf("%c", '\n');
            stdio_flush();

            if (!strnlen(cInputString,
                         sizeof cInputString)) {  // Empty input
                printf("%s", pcEndOfOutputMessage);
                continue;
            }
            const char timestr[] = "time ";
            if (0 == strncmp(cInputString, timestr, 5)) {
                xStart = xTaskGetTickCount();
                char tmp[cmdMAX_INPUT_SIZE] = {0};
                strlcpy(tmp, cInputString + 5, sizeof tmp);
                strlcpy(cInputString, tmp, cmdMAX_INPUT_SIZE);
            }
            /* Process the input string received prior to the
             newline. */
            do {
                /* Pass the string to FreeRTOS+CLI. */
                cOutputString[0] = 0x00;
                xMoreDataToFollow = FreeRTOS_CLIProcessCommand(
                    cInputString, cOutputString, cmdMAX_OUTPUT_SIZE);

                /* Send the output generated by the command's
                 implementation. */
                printf("%s", cOutputString);

                /* Until the command does not generate any more output.
                 */
            } while (xMoreDataToFollow);

            if (xStart) {
                printf("Time: %lu s\n",
                       (unsigned long)(xTaskGetTickCount() - xStart) /
                           configTICK_RATE_HZ);
            }
            /* All the strings generated by the command processing
             have been sent.  Clear the input string ready to receive
             the next command.  */
            cInputIndex = 0;
            memset(cInputString, 0x00, cmdMAX_INPUT_SIZE);

            /* Transmit a spacer to make the console easier to
             read. */
            printf("%s", pcEndOfOutputMessage);
            fflush(stdout);

        } else {  // Not newline

            if (in_overflow) continue;

            if ((cRxedChar == '\b') || (cRxedChar == cmdASCII_DEL)) {
                /* Backspace was pressed.  Erase the last character
                 in the string - if any. */
                if (cInputIndex > 0) {
                    cInputIndex--;
                    cInputString[(int)cInputIndex] = '\0';
                }
            } else {
                /* A character was entered.  Add it to the string
                 entered so far.  When a \n is entered the complete
                 string will be passed to the command interpreter. */
                if (cInputIndex < cmdMAX_INPUT_SIZE - 1) {
                    cInputString[(int)cInputIndex] = cRxedChar;
                    cInputIndex++;
                } else {
                    printf("\a[Input overflow!]\n");
                    fflush(stdout);
                    memset(cInputString, 0, sizeof(cInputString));
                    cInputIndex = 0;
                    in_overflow = true;
                }
            }
        }
    }
}

/* Start UART operation. */
void CLI_Start() {
    vRegisterCLICommands();
    vRegisterMyCLICommands();
    register_fs_tests();
    vRegisterFileSystemCLICommands();

    extern const CLI_Command_Definition_t xDataLogDemo;
    FreeRTOS_CLIRegisterCommand(&xDataLogDemo);

    // stdio_init_all();
    stdio_init_all();

    FreeRTOS_time_init();

    static StaticQueue_t xStaticQueue;
    static uint32_t ucQueueStorageArea[8];
    xQueue = xQueueCreateStatic(8, sizeof(uint32_t),
                                (uint8_t *)ucQueueStorageArea, &xStaticQueue);

    static StackType_t xStack[1024];
    static StaticTask_t xTaskBuffer;
    TaskHandle_t th = xTaskCreateStatic(
        stdioTask, "stdio Task", sizeof xStack / sizeof xStack[0], 0,
        tskIDLE_PRIORITY + 2, /* Priority at which the task is created. */
        xStack, &xTaskBuffer);
    configASSERT(th);
}

/* [] END OF FILE */
