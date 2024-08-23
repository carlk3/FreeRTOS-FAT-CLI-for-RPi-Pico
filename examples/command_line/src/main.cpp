#include <stdio.h>
#include <stdlib.h>
//
#include "FreeRTOS.h"
#include "FreeRTOS_time.h"
#include "task.h"
//
#include "crash.h"
#include "stdio_cli.h"

#ifndef USE_PRINTF
#error This program is useless without standard input and output.
#endif

int main() {
    crash_handler_init();
    stdio_init_all();
    FreeRTOS_time_init();
    setvbuf(stdout, NULL, _IONBF, 1);  // specify that the stream should be unbuffered

    CLI_Start();

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
    /* should never reach here */
    abort();
    return 0;
}
