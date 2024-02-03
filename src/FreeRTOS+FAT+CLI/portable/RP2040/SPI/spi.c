/* spi.c
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

#include <stdbool.h>
//
#include "pico/stdlib.h"
//
#include "pico/mutex.h"
//
#include "dma_interrupts.h"
#include "hw_config.h"
#include "my_debug.h"
#include "task_config.h"
#include "util.h"
//
#include "spi.h"

#ifdef NDEBUG
#  pragma GCC diagnostic ignored "-Wunused-variable"
#endif

void spi_irq_handler(spi_t *spi_p) {
    configASSERT(spi_p->owner);
    configASSERT(!dma_channel_is_busy(spi_p->rx_dma));

    /* The xHigherPriorityTaskWoken parameter must be initialized to pdFALSE as
     it will get set to pdTRUE inside the interrupt safe API function if a
     context switch is required. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Send a notification directly to the task to which interrupt processing is
     being deferred. */
    vTaskNotifyGiveIndexedFromISR(
        spi_p->owner,            // The handle of the task to which the
                                 // notification is being sent.
        NOTIFICATION_IX_SD_SPI,  // uxIndexToNotify: The index within the target task's array of
                                 // notification values to which the notification is to be sent.
        &xHigherPriorityTaskWoken);

    /* Pass the xHigherPriorityTaskWoken value into portYIELD_FROM_ISR().
    If xHigherPriorityTaskWoken was set to pdTRUE inside
     vTaskNotifyGiveIndexedFromISR() then calling portYIELD_FROM_ISR() will
     request a context switch. If xHigherPriorityTaskWoken is still
     pdFALSE then calling portYIELD_FROM_ISR() will have no effect. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static bool chk_spi(spi_inst_t *spi) {
    bool rc = true;

    if (spi_get_const_hw(spi)->sr & SPI_SSPSR_BSY_BITS) {
        DBG_PRINTF("\tSPI is busy\n");
        rc = false;
    }
    if (spi_get_const_hw(spi)->sr & SPI_SSPSR_RFF_BITS) {
        DBG_PRINTF("\tSPI Receive FIFO full\n");
        rc = false;
    }
    if (spi_get_const_hw(spi)->sr & SPI_SSPSR_RNE_BITS) {
        DBG_PRINTF("\tSPI Receive FIFO not empty\n");
        rc = false;
    }
    if (!(spi_get_const_hw(spi)->sr & SPI_SSPSR_TNF_BITS)) {
        DBG_PRINTF("\tSPI Transmit FIFO is full\n");
        rc = false;
    }
    if (!(spi_get_const_hw(spi)->sr & SPI_SSPSR_TFE_BITS)) {
        DBG_PRINTF("\tSPI Transmit FIFO is not empty\n");
        rc = false;
    }
    return rc;
}

void spi_transfer_start(spi_t *spi_p, const uint8_t *tx, uint8_t *rx, size_t length) {
    myASSERT(xTaskGetCurrentTaskHandle() == spi_p->owner);
    myASSERT(xTaskGetCurrentTaskHandle() == xSemaphoreGetMutexHolder(spi_p->mutex));
    // myASSERT(512 == length || 1 == length);
    myASSERT(tx || rx);
    // myASSERT(!(tx && rx));

    // tx write increment is already false
    if (tx) {
        channel_config_set_read_increment(&spi_p->tx_dma_cfg, true);
    } else {
        static const uint8_t dummy = SPI_FILL_CHAR;
        tx = &dummy;
        channel_config_set_read_increment(&spi_p->tx_dma_cfg, false);
    }
    // rx read increment is already false
    if (rx) {
        channel_config_set_write_increment(&spi_p->rx_dma_cfg, true);
    } else {
        static uint8_t dummy = 0xA5;
        rx = &dummy;
        channel_config_set_write_increment(&spi_p->rx_dma_cfg, false);
    }

    dma_channel_configure(spi_p->tx_dma, &spi_p->tx_dma_cfg,
                          &spi_get_hw(spi_p->hw_inst)->dr,  // write address
                          tx,                               // read address
                          length,                           // element count (each element is of
                                                            // size transfer_data_size)
                          false);                           // start
    dma_channel_configure(spi_p->rx_dma, &spi_p->rx_dma_cfg,
                          rx,                               // write address
                          &spi_get_hw(spi_p->hw_inst)->dr,  // read address
                          length,                           // element count (each element is of
                                                            // size transfer_data_size)
                          false);                           // start

    switch (spi_p->DMA_IRQ_num) {
        case DMA_IRQ_0:
            myASSERT(!dma_channel_get_irq0_status(spi_p->rx_dma));
            break;
        case DMA_IRQ_1:
            myASSERT(!dma_channel_get_irq1_status(spi_p->rx_dma));
            break;
        default:
            myASSERT(false);
    }

    /* Ensure this task does not already have a notification pending by calling
     ulTaskNotifyTake() with the xClearCountOnExit parameter set to pdTRUE, and
     a block time of 0 (don't block). */
    // uint32_t rc = 
    ulTaskNotifyTakeIndexed(NOTIFICATION_IX_SD_SPI, pdTRUE, 0);
    // myASSERT(!rc);
    // Note: Physical removal and insertion of the card seems to
    // cause spurious interrupts which can show up here.

    // start them exactly simultaneously to avoid races (in extreme cases
    // the FIFO could overflow)
    dma_start_channel_mask((1u << spi_p->tx_dma) | (1u << spi_p->rx_dma));
}

bool spi_transfer_wait_complete(spi_t *spi_p, uint32_t timeout_ms) {
    myASSERT(xTaskGetCurrentTaskHandle() == xSemaphoreGetMutexHolder(spi_p->mutex));
    /* Wait until master completes transfer or time out has occured. */
    // Wait for notification from ISR.
    uint32_t rc = ulTaskNotifyTakeIndexed(
        NOTIFICATION_IX_SD_SPI, pdFALSE, pdMS_TO_TICKS(timeout_ms));
    if (!rc) {
        // This indicates that xTaskNotifyWait() returned without the
        // calling task receiving a task notification. The calling task will
        // have been held in the Blocked state to wait for its notification
        // state to become pending, but the specified block time expired
        // before that happened.
        DBG_PRINTF("Notification wait timed out in %s\n", __FUNCTION__);
    }
    bool spi_ok = chk_spi(spi_p->hw_inst);
    if (!rc || !spi_ok) {
#       ifndef NDEBUG
        // DBG_PRINTF("RX DMA control regs:\n");
        // print_dma_ctrl(&dma_hw->ch[spi_p->rx_dma]); // pico-sdk\src\rp2_common\hardware_dma\dma.c
        // DBG_PRINTF("\n");
        DBG_PRINTF("DMA CTRL_TRIG: 0b%s\n", uint_binary_str(dma_hw->ch[spi_p->rx_dma].ctrl_trig));
        DBG_PRINTF("SPI SSPCR0: 0b%s\n", uint_binary_str(spi_get_hw(spi_p->hw_inst)->cr0));
        DBG_PRINTF("SPI SSPCR1: 0b%s\n", uint_binary_str(spi_get_hw(spi_p->hw_inst)->cr1));
#       endif
        return false;
    }
    // Shouldn't be necessary:
    dma_channel_wait_for_finish_blocking(spi_p->tx_dma);
    dma_channel_wait_for_finish_blocking(spi_p->rx_dma);

    myASSERT(!dma_channel_is_busy(spi_p->tx_dma));
    myASSERT(!dma_channel_is_busy(spi_p->rx_dma));

    return true;
}
// SPI Transfer: Read & Write (simultaneously) on SPI bus
//   If the data that will be received is not important, pass NULL as rx.
//   If the data that will be transmitted is not important,
//     pass NULL as tx and then the SPI_FILL_CHAR is sent out as each data
//     element.
bool spi_transfer(spi_t *spi_p, const uint8_t *tx, uint8_t *rx, size_t length) {
    spi_transfer_start(spi_p, tx, rx, length);
    return spi_transfer_wait_complete(spi_p, 1000);
}

void spi_lock(spi_t *spi_p) {
    myASSERT(spi_p->mutex);
    xSemaphoreTake(spi_p->mutex, portMAX_DELAY);
    myASSERT(0 == spi_p->owner);
    spi_p->owner = xTaskGetCurrentTaskHandle();
}
void spi_unlock(spi_t *spi_p) {
    myASSERT(spi_p->mutex);
    myASSERT(xTaskGetCurrentTaskHandle() == spi_p->owner);
    spi_p->owner = 0;
    xSemaphoreGive(spi_p->mutex);
}

bool my_spi_init(spi_t *spi_p) {
    auto_init_mutex(my_spi_init_mutex);
    mutex_enter_blocking(&my_spi_init_mutex);
    if (!spi_p->initialized) {
        //// The SPI may be shared (using multiple SSs); protect it
        spi_p->mutex = xSemaphoreCreateMutex();
        spi_lock(spi_p);

        // Defaults:
        if (!spi_p->hw_inst)
            spi_p->hw_inst = spi0;
        if (!spi_p->baud_rate)
            spi_p->baud_rate = 10 * 1000 * 1000;
        if (!spi_p->DMA_IRQ_num)
            spi_p->DMA_IRQ_num = DMA_IRQ_0;

        /* Configure component */
        // Enable SPI at 100 kHz and connect to GPIOs
        spi_init(spi_p->hw_inst, 100 * 1000);
        spi_set_format(spi_p->hw_inst, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

        gpio_set_function(spi_p->miso_gpio, GPIO_FUNC_SPI);
        gpio_set_function(spi_p->mosi_gpio, GPIO_FUNC_SPI);
        gpio_set_function(spi_p->sck_gpio, GPIO_FUNC_SPI);
        // ss_gpio is initialized in sd_spi_ctor()

        // Slew rate limiting levels for GPIO outputs.
        // enum gpio_slew_rate { GPIO_SLEW_RATE_SLOW = 0, GPIO_SLEW_RATE_FAST = 1 }
        // void gpio_set_slew_rate (uint gpio,enum gpio_slew_rate slew)
        // Default appears to be GPIO_SLEW_RATE_SLOW.
        gpio_set_slew_rate (spi_p->sck_gpio, GPIO_SLEW_RATE_FAST);

        // Drive strength levels for GPIO outputs.
        // enum gpio_drive_strength { GPIO_DRIVE_STRENGTH_2MA = 0, GPIO_DRIVE_STRENGTH_4MA = 1, GPIO_DRIVE_STRENGTH_8MA = 2,
        // GPIO_DRIVE_STRENGTH_12MA = 3 }
        // enum gpio_drive_strength gpio_get_drive_strength (uint gpio)
        if (spi_p->set_drive_strength) {
            gpio_set_drive_strength(spi_p->mosi_gpio, spi_p->mosi_gpio_drive_strength);
            gpio_set_drive_strength(spi_p->sck_gpio, spi_p->sck_gpio_drive_strength);
        }

        // SD cards' DO MUST be pulled up. However, it might be done externally.
        if (!spi_p->no_miso_gpio_pull_up)
            gpio_pull_up(spi_p->miso_gpio);

        gpio_set_input_hysteresis_enabled(spi_p->miso_gpio, false);

        // Grab some unused dma channels
        spi_p->tx_dma = dma_claim_unused_channel(true);
        spi_p->rx_dma = dma_claim_unused_channel(true);

        spi_p->tx_dma_cfg = dma_channel_get_default_config(spi_p->tx_dma);
        spi_p->rx_dma_cfg = dma_channel_get_default_config(spi_p->rx_dma);
        channel_config_set_transfer_data_size(&spi_p->tx_dma_cfg, DMA_SIZE_8);
        channel_config_set_transfer_data_size(&spi_p->rx_dma_cfg, DMA_SIZE_8);

        // We set the outbound DMA to transfer from a memory buffer to the SPI
        // transmit FIFO paced by the SPI TX FIFO DREQ The default is for the
        // read address to increment every element (in this case 1 byte -
        // DMA_SIZE_8) and for the write address to remain unchanged.
        channel_config_set_dreq(&spi_p->tx_dma_cfg, spi_get_index(spi_p->hw_inst)
                                                        ? DREQ_SPI1_TX
                                                        : DREQ_SPI0_TX);
        channel_config_set_write_increment(&spi_p->tx_dma_cfg, false);

        // We set the inbound DMA to transfer from the SPI receive FIFO to a
        // memory buffer paced by the SPI RX FIFO DREQ We coinfigure the read
        // address to remain unchanged for each element, but the write address
        // to increment (so data is written throughout the buffer)
        channel_config_set_dreq(&spi_p->rx_dma_cfg, spi_get_index(spi_p->hw_inst)
                                                        ? DREQ_SPI1_RX
                                                        : DREQ_SPI0_RX);
        channel_config_set_read_increment(&spi_p->rx_dma_cfg, false);

        /* Theory: we only need an interrupt on rx complete,
        since if rx is complete, tx must also be complete. */

        // Tell the DMA to raise IRQ line 0/1 when the channel finishes a block
        switch (spi_p->DMA_IRQ_num) {
            case DMA_IRQ_0:
                // Clear any pending interrupts:
                dma_hw->ints0 = 1 << spi_p->rx_dma;
                dma_hw->ints0 = 1 << spi_p->tx_dma;
                dma_channel_set_irq0_enabled(spi_p->rx_dma, true);
                dma_channel_set_irq0_enabled(spi_p->tx_dma, false);
                break;
            case DMA_IRQ_1:
                // Clear any pending interrupts:
                dma_hw->ints1 = 1 << spi_p->rx_dma;
                dma_hw->ints1 = 1 << spi_p->tx_dma;
                dma_channel_set_irq1_enabled(spi_p->rx_dma, true);
                dma_channel_set_irq1_enabled(spi_p->tx_dma, false);
                break;
            default:
                myASSERT(false);
        }
        dma_irq_add_handler(spi_p->DMA_IRQ_num, spi_p->use_exclusive_DMA_IRQ_handler);

        LED_INIT();

        spi_p->initialized = true;
        spi_unlock(spi_p);
    }
    mutex_exit(&my_spi_init_mutex);
    return true;
}

/* [] END OF FILE */
