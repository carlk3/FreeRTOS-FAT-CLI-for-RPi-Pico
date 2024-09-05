#include <limits.h>

#include "hardware/gpio.h"
//
#include "FreeRTOS.h"
#include "timers.h"
//
#include "ff_utils.h"
#include "hw_config.h"
#include "my_debug.h"
#include "sd_card.h"
#include "task_config.h"
//
#include "unmounter.h"

#if defined(NDEBUG) || !USE_DBG_PRINTF
#  pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static void vDeferredHandlingFunction(void *, uint32_t gpio) {
    for (size_t i = 0; i < sd_get_num(); ++i) {
        sd_card_t *sd_card_p = sd_get_by_num(i);
        if (sd_card_p->card_detect_gpio == gpio) {
            if (sd_card_p->state.ff_disk.xStatus.bIsMounted) {
                DBG_PRINTF("(Card Detect Interrupt: unmounting %s)\n", sd_card_p->device_name);
                unmount(sd_card_p->device_name);
            }
            sd_card_p->state.m_Status |= STA_NOINIT;  // in case medium is removed
            sd_card_detect(sd_card_p);
        }
    }
}

static void card_detect_callback(uint gpio, uint32_t) {
    /* The xHigherPriorityTaskWoken parameter must be initialized to pdFALSE as
     it will get set to pdTRUE inside the interrupt safe API function if a
     context switch is required. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Send a pointer to the interrupt's deferred handling function to the daemon task. */
    BaseType_t rc =
        xTimerPendFunctionCallFromISR(vDeferredHandlingFunction, /* Function to execute. */
                                      NULL,                      /* Not used. */
                                      gpio,
                                      &xHigherPriorityTaskWoken);

    configASSERT(pdPASS == rc);

    /* Pass the xHigherPriorityTaskWoken value into portYIELD_FROM_ISR().
    If xHigherPriorityTaskWoken was set to pdTRUE inside
     vTaskNotifyGiveFromISR() then calling portYIELD_FROM_ISR() will
     request a context switch. If xHigherPriorityTaskWoken is still
     pdFALSE then calling portYIELD_FROM_ISR() will have no effect. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void unmounter_init() {
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
}
