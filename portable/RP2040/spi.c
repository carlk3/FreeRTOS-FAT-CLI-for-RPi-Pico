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
//
#include "pico/stdlib.h"
//
#include "FreeRTOS.h"
//
#include "spi.h"

void spi_irq_handler(spi_t *this) {
    // Clear the interrupt request.
    dma_hw->ints0 = 1u << this->rx_dma;
    configASSERT(this->owner);
    configASSERT(!dma_channel_is_busy(this->rx_dma));

    /* The xHigherPriorityTaskWoken parameter must be initialized to pdFALSE as
     it will get set to pdTRUE inside the interrupt safe API function if a
     context switch is required. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Send a notification directly to the task to which interrupt processing is
     being deferred. */
    vTaskNotifyGiveFromISR(this->owner,  // The handle of the task to which the
                                         // notification is being sent.
                           &xHigherPriorityTaskWoken);

    /* Pass the xHigherPriorityTaskWoken value into portYIELD_FROM_ISR(). If
     xHigherPriorityTaskWoken was set to pdTRUE inside vTaskNotifyGiveFromISR()
     then calling portYIELD_FROM_ISR() will request a context switch. If
     xHigherPriorityTaskWoken is still pdFALSE then calling
     portYIELD_FROM_ISR() will have no effect. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// SPI Transfer: Read & Write (simultaneously) on SPI bus
//   If the data that will be received is not important, pass NULL as rx.
//   If the data that will be transmitted is not important,
//     pass NULL as tx and then the SPI_FILL_CHAR is sent out as each data
//     element.

bool spi_transfer(spi_t *this, const uint8_t *tx, uint8_t *rx, size_t length) {
    configASSERT(xTaskGetCurrentTaskHandle() == this->owner);
    configASSERT(512 == length);
    configASSERT(tx || rx);
    // configASSERT(!(tx && rx));


    // Would have to be static if this function could return before DMA
    // completes:
    uint8_t dummy = SPI_FILL_CHAR;
    // tx write increment is already false
    if (tx) {
        channel_config_set_read_increment(&this->tx_dma_cfg, true);
    } else {
        tx = &dummy;
        channel_config_set_read_increment(&this->tx_dma_cfg, false);
    }
    // rx read increment is already false
    if (rx) {
        channel_config_set_write_increment(&this->rx_dma_cfg, true);
    } else {
        rx = &dummy;
        channel_config_set_write_increment(&this->rx_dma_cfg, false);
    }
    /* Ensure this task does not already have a notification pending by calling
     ulTaskNotifyTake() with the xClearCountOnExit parameter set to pdTRUE, and
     a block time of 0 (don't block). */
    BaseType_t rc = ulTaskNotifyTake(pdTRUE, 0);
    configASSERT(!rc);

    dma_channel_configure(this->tx_dma, &this->tx_dma_cfg,
                          &spi_get_hw(this->hw_inst)->dr,  // write address
                          tx,                              // read address
                          XFER_BLOCK_SIZE,  // element count (each element is of
                                            // size transfer_data_size)
                          false);           // start
    dma_channel_configure(this->rx_dma, &this->rx_dma_cfg,
                          rx,                              // write address
                          &spi_get_hw(this->hw_inst)->dr,  // read address
                          XFER_BLOCK_SIZE,  // element count (each element is of
                                            // size transfer_data_size)
                          false);           // start

    // start them exactly simultaneously to avoid races (in extreme cases
    // the FIFO could overflow)
    dma_start_channel_mask((1u << this->tx_dma) | (1u << this->rx_dma));

    /* Timeout 1 sec */
    uint32_t timeOut = 1000;
    /* Wait until master completes transfer or time out has occured. */
    /* Wait 2x, for RX and SPI to complete. */
    // uint32_t ulTaskNotifyTake( BaseType_t xClearCountOnExit, TickType_t
    // xTicksToWait );
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
    configASSERT(!dma_channel_is_busy(this->tx_dma));
    configASSERT(!dma_channel_is_busy(this->rx_dma));

    return true;
}

bool my_spi_init(spi_t *this) {
    // bool __atomic_test_and_set (void *ptr, int memorder)
    // This built-in function performs an atomic test-and-set operation on the
    // byte at *ptr. The byte is set to some implementation defined nonzero
    // “set” value and the return value is true if and only if the previous
    // contents were “set”.
    if (__atomic_test_and_set(&(this->initialized), __ATOMIC_SEQ_CST))
        return true;

    // The SPI may be shared (using multiple SSs); protect it
    this->mutex = xSemaphoreCreateRecursiveMutex();
    xSemaphoreTakeRecursive(this->mutex, portMAX_DELAY);

    /* Configure component */
    // Enable SPI at 100 kHz and connect to GPIOs
    spi_init(this->hw_inst, 100 * 1000);
    spi_set_format(this->hw_inst, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(this->miso_gpio, GPIO_FUNC_SPI);
    gpio_set_function(this->mosi_gpio, GPIO_FUNC_SPI);
    gpio_set_function(this->sck_gpio, GPIO_FUNC_SPI);
    // ss_gpio is initialized in sd_spi_init()

    // SD cards' DO MUST be pulled up.
    gpio_pull_up(this->miso_gpio);

    // Grab some unused dma channels
    this->tx_dma = dma_claim_unused_channel(true);
    this->rx_dma = dma_claim_unused_channel(true);

    this->tx_dma_cfg = dma_channel_get_default_config(this->tx_dma);
    this->rx_dma_cfg = dma_channel_get_default_config(this->rx_dma);
    channel_config_set_transfer_data_size(&this->tx_dma_cfg, DMA_SIZE_8);
    channel_config_set_transfer_data_size(&this->rx_dma_cfg, DMA_SIZE_8);

    // We set the outbound DMA to transfer from a memory buffer to the SPI
    // transmit FIFO paced by the SPI TX FIFO DREQ The default is for the
    // read address to increment every element (in this case 1 byte -
    // DMA_SIZE_8) and for the write address to remain unchanged.
    channel_config_set_dreq(&this->tx_dma_cfg, spi_get_index(this->hw_inst)
                                                   ? DREQ_SPI1_TX
                                                   : DREQ_SPI0_TX);
    channel_config_set_write_increment(&this->tx_dma_cfg, false);

    // We set the inbound DMA to transfer from the SPI receive FIFO to a
    // memory buffer paced by the SPI RX FIFO DREQ We coinfigure the read
    // address to remain unchanged for each element, but the write address
    // to increment (so data is written throughout the buffer)
    channel_config_set_dreq(&this->rx_dma_cfg, spi_get_index(this->hw_inst)
                                                   ? DREQ_SPI1_RX
                                                   : DREQ_SPI0_RX);
    channel_config_set_read_increment(&this->rx_dma_cfg, false);

    /* Theory: we only need an interrupt on rx complete,
    since if rx is complete, tx must also be complete. */

    // Configure the processor to run dma_handler() when DMA IRQ 0 is
    // asserted
    irq_set_exclusive_handler(DMA_IRQ_0, this->dma_isr);

    // Tell the DMA to raise IRQ line 0 when the channel finishes a block
    dma_channel_set_irq0_enabled(this->rx_dma, true);
    irq_set_enabled(DMA_IRQ_0, true);

    LED_INIT();

    xSemaphoreGiveRecursive(this->mutex);
    return true;
}

/* [] END OF FILE */
