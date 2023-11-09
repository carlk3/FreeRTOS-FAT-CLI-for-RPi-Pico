#include <stdio.h>

#include "sd_card.h"
#include "hw_config.h"
#include "stdio_cli.h"
#include "ff_utils.h"
//
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_time.h"
//
#include "pico/stdlib.h"
//
#include "crash.h"

#ifndef USE_PRINTF
#error This program is useless without standard input and output.
#endif

// If the card is physically removed, unmount the filesystem:
static void card_detect_callback(uint gpio, uint32_t events) {
    static bool busy;
    if (busy) return; // Avoid switch bounce
    busy = true;
    for (size_t i = 0; i < sd_get_num(); ++i) {
        sd_card_t *sd_card_p = sd_get_by_num(i);
        if (sd_card_p->card_detect_gpio == gpio) {
            if (sd_card_p->ff_disk.xStatus.bIsMounted) {
                DBG_PRINTF("(Card Detect Interrupt: unmounting %s)\n", sd_card_p->device_name);
                unmount(sd_card_p->device_name);
            }
            sd_card_p->m_Status |= STA_NOINIT; // in case medium is removed
            sd_card_detect(sd_card_p);
        }
    }
    busy = false;
}

int main() {
    crash_handler_init();
    stdio_init_all();
    FreeRTOS_time_init();
    setvbuf(stdout, NULL, _IONBF, 1); // specify that the stream should be unbuffered

    printf("\033[2J\033[H");  // Clear Screen
    stdio_flush();

    // Implicitly called by disk_initialize, 
    // but called here to set up the GPIOs 
    // before enabling the card detect interrupt:
    sd_init_driver();

    for (size_t i = 0; i < sd_get_num(); ++i) {
        sd_card_t *sd_card_p = sd_get_by_num(i);
        if (sd_card_p->use_card_detect) {
            // Set up an interrupt on Card Detect to detect removal of the card
            // when it happens:
            gpio_set_irq_enabled_with_callback(
                sd_card_p->card_detect_gpio, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                true, &card_detect_callback);
        }
    }

    CLI_Start();

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
    /* should never reach here */
    abort();
    return 0;
}
