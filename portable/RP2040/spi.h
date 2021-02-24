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
//
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/types.h"

#define SPI_FILL_CHAR (0xFF)

// "Class" representing SPIs
typedef struct {
    // SPI HW
    spi_inst_t *pInst;
    const uint miso_gpio;  // SPI MISO pin number for GPIO
    const uint mosi_gpio;
    const uint sck_gpio;
    // State variables:
    bool initialized;         // Assigned dynamically
    TaskHandle_t owner;       // Assigned dynamically
    SemaphoreHandle_t mutex;  // Assigned dynamically
} spi_t;

bool my_spi_init(spi_t *this);

#endif
/* [] END OF FILE */
