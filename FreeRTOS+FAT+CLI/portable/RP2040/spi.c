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
#include "pico/mutex.h"
#include "pico/stdlib.h"
//
#include "FreeRTOS.h"
//
#include "spi.h"

void spi_irq_handler(spi_t *pSPI) {
    // Clear the interrupt request.
    dma_hw->ints0 = 1u << pSPI->rx_dma;
    configASSERT(pSPI->owner);
    configASSERT(!dma_channel_is_busy(pSPI->rx_dma));

    /* The xHigherPriorityTaskWoken parameter must be initialized to pdFALSE as
     it will get set to pdTRUE inside the interrupt safe API function if a
     context switch is required. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Send a notification directly to the task to which interrupt processing is
     being deferred. */
    vTaskNotifyGiveFromISR(pSPI->owner,  // The handle of the task to which the
                                         // notification is being sent.
                           &xHigherPriorityTaskWoken);

    /* Pass the xHigherPriorityTaskWoken value into portYIELD_FROM_ISR().
    If xHigherPriorityTaskWoken was set to pdTRUE inside
     vTaskNotifyGiveFromISR() then calling portYIELD_FROM_ISR() will
     request a context switch. If xHigherPriorityTaskWoken is still
     pdFALSE then calling portYIELD_FROM_ISR() will have no effect. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// SPI Transfer: Read & Write (simultaneously) on SPI bus
//   If the data that will be received is not important, pass NULL as rx.
//   If the data that will be transmitted is not important,
//     pass NULL as tx and then the SPI_FILL_CHAR is sent out as each data
//     element.

bool spi_transfer(spi_t *pSPI, const uint8_t *tx, uint8_t *rx, size_t length) {
    configASSERT(xTaskGetCurrentTaskHandle() == pSPI->owner);
    configASSERT(tx || rx);

    // tx write increment is already false
    if (tx) {
        channel_config_set_read_increment(&pSPI->tx_dma_cfg, true);
    } else {
        static const uint8_t dummy = SPI_FILL_CHAR;
        tx = &dummy;
        channel_config_set_read_increment(&pSPI->tx_dma_cfg, false);
    }
    // rx read increment is already false
    if (rx) {
        channel_config_set_write_increment(&pSPI->rx_dma_cfg, true);
    } else {
        static uint8_t dummy = 0xA5;
        rx = &dummy;
        channel_config_set_write_increment(&pSPI->rx_dma_cfg, false);
    }
    /* Ensure this task does not already have a notification pending by calling
     ulTaskNotifyTake() with the xClearCountOnExit parameter set to pdTRUE, and
     a block time of 0 (don't block). */
    BaseType_t rc = ulTaskNotifyTake(pdTRUE, 0);
    configASSERT(!rc);

    dma_channel_configure(pSPI->tx_dma, &pSPI->tx_dma_cfg,
                          &spi_get_hw(pSPI->hw_inst)->dr,  // write address
                          tx,                              // read address
                          length,  // element count (each element is of
                                   // size transfer_data_size)
                          false);  // start
    dma_channel_configure(pSPI->rx_dma, &pSPI->rx_dma_cfg,
                          rx,                              // write address
                          &spi_get_hw(pSPI->hw_inst)->dr,  // read address
                          length,  // element count (each element is of
                                   // size transfer_data_size)
                          false);  // start

    // start them exactly simultaneously to avoid races (in extreme cases
    // the FIFO could overflow)
    dma_start_channel_mask((1u << pSPI->tx_dma) | (1u << pSPI->rx_dma));

    /* Timeout 1 sec */
    uint32_t timeOut = 1000;
    /* Wait until master completes transfer or time out has occured. */
    rc = ulTaskNotifyTake(
        pdFALSE, pdMS_TO_TICKS(timeOut));  // Wait for notification from ISR
    if (!rc) {
        // This indicates that xTaskNotifyWait() returned without the
        // calling task receiving a task notification. The calling task will
        // have been held in the Blocked state to wait for its notification
        // state to become pending, but the specified block time expired
        // before that happened.
        DBG_PRINTF("Task %s timed out in %s\n",
                   pcTaskGetName(xTaskGetCurrentTaskHandle()), __FUNCTION__);
        return false;
    }
    // Shouldn't be necessary:
    dma_channel_wait_for_finish_blocking(pSPI->tx_dma);
    dma_channel_wait_for_finish_blocking(pSPI->rx_dma);

    configASSERT(!dma_channel_is_busy(pSPI->tx_dma));
    configASSERT(!dma_channel_is_busy(pSPI->rx_dma));

    return true;
}

bool my_spi_init(spi_t *pSPI) {
    auto_init_mutex(my_spi_init_mutex);
    mutex_enter_blocking(&my_spi_init_mutex);
    if (!pSPI->initialized) {
        // The SPI may be shared (using multiple SSs); protect it
        pSPI->mutex = xSemaphoreCreateRecursiveMutex();
        xSemaphoreTakeRecursive(pSPI->mutex, portMAX_DELAY);

        /* Configure component */
        // Enable SPI at 100 kHz and connect to GPIOs
        spi_init(pSPI->hw_inst, 100 * 1000);
        spi_set_format(pSPI->hw_inst, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

        gpio_set_function(pSPI->miso_gpio, GPIO_FUNC_SPI);
        gpio_set_function(pSPI->mosi_gpio, GPIO_FUNC_SPI);
        gpio_set_function(pSPI->sck_gpio, GPIO_FUNC_SPI);
        // ss_gpio is initialized in sd_spi_init()

        // SD cards' DO MUST be pulled up.
        gpio_pull_up(pSPI->miso_gpio);

        // Grab some unused dma channels
        pSPI->tx_dma = dma_claim_unused_channel(true);
        pSPI->rx_dma = dma_claim_unused_channel(true);

        pSPI->tx_dma_cfg = dma_channel_get_default_config(pSPI->tx_dma);
        pSPI->rx_dma_cfg = dma_channel_get_default_config(pSPI->rx_dma);
        channel_config_set_transfer_data_size(&pSPI->tx_dma_cfg, DMA_SIZE_8);
        channel_config_set_transfer_data_size(&pSPI->rx_dma_cfg, DMA_SIZE_8);

        // We set the outbound DMA to transfer from a memory buffer to the SPI
        // transmit FIFO paced by the SPI TX FIFO DREQ The default is for the
        // read address to increment every element (in this case 1 byte -
        // DMA_SIZE_8) and for the write address to remain unchanged.
        channel_config_set_dreq(&pSPI->tx_dma_cfg, spi_get_index(pSPI->hw_inst)
                                                       ? DREQ_SPI1_TX
                                                       : DREQ_SPI0_TX);
        channel_config_set_write_increment(&pSPI->tx_dma_cfg, false);

        // We set the inbound DMA to transfer from the SPI receive FIFO to a
        // memory buffer paced by the SPI RX FIFO DREQ We coinfigure the read
        // address to remain unchanged for each element, but the write address
        // to increment (so data is written throughout the buffer)
        channel_config_set_dreq(&pSPI->rx_dma_cfg, spi_get_index(pSPI->hw_inst)
                                                       ? DREQ_SPI1_RX
                                                       : DREQ_SPI0_RX);
        channel_config_set_read_increment(&pSPI->rx_dma_cfg, false);

        /* Theory: we only need an interrupt on rx complete,
        since if rx is complete, tx must also be complete. */

        // Configure the processor to run dma_handler() when DMA IRQ 0 is
        // asserted
        irq_set_exclusive_handler(DMA_IRQ_0, pSPI->dma_isr);

        /* Any interrupt that uses interrupt-safe FreeRTOS API functions must
         * also execute at the priority defined by
         * configKERNEL_INTERRUPT_PRIORITY. */
        irq_set_priority(DMA_IRQ_0, 0xFF);  // Lowest urgency.

        // Tell the DMA to raise IRQ line 0 when the channel finishes a block
        dma_channel_set_irq0_enabled(pSPI->rx_dma, true);
        irq_set_enabled(DMA_IRQ_0, true);

        LED_INIT();

        xSemaphoreGiveRecursive(pSPI->mutex);

        pSPI->initialized = true;
    }
    mutex_exit(&my_spi_init_mutex);
    return true;
}

/* [] END OF FILE */
