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
#ifndef _SPI_H_
#define _SPI_H_

#include <stdbool.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
//
#include <semphr.h>
#include <task.h>

// Pico includes
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "pico/types.h"

#define SPI_FILL_CHAR (0xFF)
#define XFER_BLOCK_SIZE 512 // Block size supported for SD card is 512 bytes 

// "Class" representing SPIs
typedef struct {
    // SPI HW
    spi_inst_t *hw_inst;
    const uint miso_gpio;  // SPI MISO pin number for GPIO
    const uint mosi_gpio;
    const uint sck_gpio;
    const uint baud_rate;
    // State variables:
    uint tx_dma;
    uint rx_dma;
    dma_channel_config tx_dma_cfg;
    dma_channel_config rx_dma_cfg;
    irq_handler_t dma_isr;
    bool initialized;  // Assigned dynamically
    TaskHandle_t owner;       // Assigned dynamically
    SemaphoreHandle_t mutex;  // Assigned dynamically
} spi_t;

// SPI DMA interrupts
void spi_irq_handler(spi_t *this);

bool spi_transfer(spi_t *this, const uint8_t *tx, uint8_t *rx, size_t length);
bool my_spi_init(spi_t *this);

#endif
/* [] END OF FILE */
