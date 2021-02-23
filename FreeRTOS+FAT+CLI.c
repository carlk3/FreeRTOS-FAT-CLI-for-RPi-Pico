#include <stdio.h>

//
#include "FreeRTOS.h"
#include "task.h"

//
#include "hardware/clocks.h"
#include "hardware/divider.h"
#include "hardware/dma.h"
//#include "hardware/gpio.h"
//#include "hardware/spi.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

//
#include "crash.h"
#include "stdio_cli.h"

// // UART defines
// // By default the stdout UART is `uart0`, so we will use the second one
// #define UART_ID uart1
// #define BAUD_RATE 9600

// // Use pins 4 and 5 for UART1
// // Pins can be changed, see the GPIO function select table in the datasheet
// for information on GPIO assignments #define UART_TX_PIN 4 #define UART_RX_PIN
// 5

// GPIO defines
// Example uses GPIO 2
#define GPIO 2

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for
// information on GPIO assignments
//#define SPI_PORT spi0
//#define PIN_MISO 16
//#define PIN_CS 17
//#define PIN_SCK 18
//#define PIN_MOSI 19

int old_main() {
    stdio_init_all();
    while (true) {
        printf("Hello, world!\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    return 0;
}

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

static void TaskHello(void* v) {
    (void)v;
    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}

int main() {
    // stdio_init_all();

    // // Set up our UART
    // uart_init(UART_ID, BAUD_RATE);
    // // Set the TX and RX pins by using the function select on the GPIO
    // // Set datasheet for more information on function select
    // gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    // gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // GPIO initialisation.
    // We will make this GPIO an input, and pull it up by default
    gpio_init(GPIO);
    gpio_set_dir(GPIO, GPIO_IN);
    gpio_pull_up(GPIO);

#if 0
    // Example of using the HW divider. The pico_divider library provides a more user friendly set of APIs 
    // over the divider (and support for 64 bit divides), and of course by default regular C language integer
    // divisions are redirected thru that library, meaning you can just use C level `/` and `%` operators and
    // gain the benefits of the fast hardware divider.
    int32_t dividend = 123456;
    int32_t divisor = -321;
    // This is the recommended signed fast divider for general use.
    divmod_result_t result = hw_divider_divmod_s32(dividend, divisor);
    printf("%d/%d = %d remainder %d\n", dividend, divisor, to_quotient_s32(result), to_remainder_s32(result));
    // This is the recommended unsigned fast divider for general use.
    int32_t udividend = 123456;
    int32_t udivisor = 321;
    divmod_result_t uresult = hw_divider_divmod_u32(udividend, udivisor);
    printf("%d/%d = %d remainder %d\n", udividend, udivisor, to_quotient_u32(uresult), to_remainder_u32(uresult));
#endif
    // // This is the basic hardware divider function
    // int32_t dividend = 123456;
    // int32_t divisor = -321;
    // divmod_result_t result = hw_divider_divmod_s32(dividend, divisor);

    //// SPI initialisation. This example will use SPI at 1MHz.
    // spi_init(SPI_PORT, 1000 * 1000);
    // gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    // gpio_set_function(PIN_CS, GPIO_FUNC_SIO);
    // gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    // gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    //// Chip select is active-low, so we'll initialise it to a driven-high
    ///state
    // gpio_set_dir(PIN_CS, GPIO_OUT);
    // gpio_put(PIN_CS, 1);

    // puts("Hello, world!");
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

    // xReturned = xTaskCreate(
    //    TaskHello,        /* Function that implements the task. */
    //    "Hello task",    /* Text name for the task. */
    //    512,              /* Stack size in words, not bytes. */
    //    0,         /* Parameter passed into the task. */
    //    tskIDLE_PRIORITY, /* Priority at which the task is created. */
    //    NULL);
    // configASSERT(pdPASS == xReturned);

    crash_handler_init();

    CLI_Start();

    vTaskStartScheduler();
    configASSERT(!"Can't happen!");

    return 0;
}
