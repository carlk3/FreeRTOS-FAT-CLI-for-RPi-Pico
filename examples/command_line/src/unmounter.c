#include <limits.h>
#include "hardware/gpio.h"
//
#include "ff_utils.h"
#include "hw_config.h"
#include "my_debug.h"
#include "sd_card.h"
#include "task_config.h"
//
#include "unmounter.h"

static TaskHandle_t th;

static void card_detect_callback(uint gpio, uint32_t events) {
    myASSERT(th);
    /* The xHigherPriorityTaskWoken parameter must be initialized to pdFALSE as
     it will get set to pdTRUE inside the interrupt safe API function if a
     context switch is required. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Send a notification directly to the task to which interrupt processing is
     being deferred. */
    xTaskNotifyIndexedFromISR(th,                        // xTaskToNotify,
                              NOTIFICATION_IX_UNMOUNT,   // uxIndexToNotify,
                              gpio,                      // ulValue,
                              eSetValueWithOverwrite,    // eAction,
                              &xHigherPriorityTaskWoken  // pxHigherPriorityTaskWoken
    );
    /* Pass the xHigherPriorityTaskWoken value into portYIELD_FROM_ISR().
    If xHigherPriorityTaskWoken was set to pdTRUE inside
     vTaskNotifyGiveFromISR() then calling portYIELD_FROM_ISR() will
     request a context switch. If xHigherPriorityTaskWoken is still
     pdFALSE then calling portYIELD_FROM_ISR() will have no effect. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// If the card is physically removed, unmount the filesystem:
static void unmounterTask(void *arg) {
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
    for (;;) {
        uint32_t gpio;
        BaseType_t ok = xTaskNotifyWaitIndexed(NOTIFICATION_IX_UNMOUNT,  // uxIndexToWaitOn,
                                               0x00,                     // ulBitsToClearOnEntry,
                                               ULONG_MAX,                // ulBitsToClearOnExit,
                                               &gpio,                    // pulNotificationValue,
                                               portMAX_DELAY             // xTicksToWait
        );
        if (pdTRUE != ok)
            continue;

        for (size_t i = 0; i < sd_get_num(); ++i) {
            sd_card_t *sd_card_p = sd_get_by_num(i);
            if (sd_card_p->card_detect_gpio == gpio) {
                if (sd_card_p->ff_disk.xStatus.bIsMounted) {
                    DBG_PRINTF("(Card Detect Interrupt: unmounting %s)\n", sd_card_p->device_name);
                    unmount(sd_card_p->device_name);
                }
                sd_card_p->m_Status |= STA_NOINIT;  // in case medium is removed
                sd_card_detect(sd_card_p);
            }
        }
    }
}

void unmounter_init() {
    static StackType_t xStack[256];
    static StaticTask_t xTaskBuffer;
    th = xTaskCreateStatic(
        unmounterTask, "Unmounter Task", sizeof xStack / sizeof xStack[0], 0,
        1, /* Priority at which the task is created. */
        xStack, &xTaskBuffer);
    configASSERT(th);
}
