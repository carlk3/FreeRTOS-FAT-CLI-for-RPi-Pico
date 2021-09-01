#include <stdio.h>

//
#include "FreeRTOS.h"
#include "task.h"
//
#include "pico/stdlib.h"
#include "pico/multicore.h"
//
#include "crash.h"
#include "stdio_cli.h"

int main() {

#ifdef ANALYZER
    gpio_init(15);                  //DEBUG
    gpio_set_dir(15, GPIO_OUT);
    gpio_init(14);  // DEBUG
    gpio_set_dir(14, GPIO_OUT);
#endif

    crash_handler_init();

    CLI_Start();

    printf("Core %d: Launching FreeRTOS scheduler\n", get_core_num());

    vTaskStartScheduler();
    configASSERT(!"Can't happen!");

    return 0;
}
