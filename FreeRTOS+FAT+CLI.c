#include <stdio.h>

//
#include "FreeRTOS.h"
#include "task.h"

//
//#include "hardware/clocks.h"
//#include "hardware/divider.h"
//#include "hardware/dma.h"
//#include "hardware/gpio.h"
//#include "hardware/spi.h"
//#include "hardware/uart.h"
#include "pico/stdlib.h"

//
#include "crash.h"
#include "stdio_cli.h"

static void vTaskCode(void* pvParameters) {
    /* The parameter value is expected to be 1 as 1 is passed in the
    pvParameters value in the call to xTaskCreate() below.
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );
    */
    for (;;) {
        const uint LED_PIN = 25;
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
        while (true) {
            gpio_put(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(250));
            gpio_put(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(250));
        }
    }
}

int main() {

    BaseType_t xReturned;
    TaskHandle_t xHandle = NULL;
    /* Create the task, storing the handle. */
    xReturned = xTaskCreate(
        vTaskCode,            /* Function that implements the task. */
        "Blinky task",        /* Text name for the task. */
        512,                  /* Stack size in words, not bytes. */
        (void*)1,             /* Parameter passed into the task. */
        tskIDLE_PRIORITY + 1, /* Priority at which the task is created. */
        &xHandle);
    configASSERT(pdPASS == xReturned);

    crash_handler_init();

    CLI_Start();

    vTaskStartScheduler();
    configASSERT(!"Can't happen!");

    return 0;
}
