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

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
//
/* FreeRTOS includes. */
#include "FreeRTOS.h"
//#include "FreeRTOSFATConfig.h" // for DBG_PRINTF
//
#include "sd_card.h"
#include "sd_spi.h"
#include "spi.h"
//

//#define TRACE_PRINTF(fmt, args...)
#define TRACE_PRINTF printf  // task_printf

void sd_spi_go_high_frequency(sd_card_t *this) {
    uint actual = spi_set_baudrate(this->spi->pInst, 6 * 1000 * 1000);
    TRACE_PRINTF("%s: Actual frequency: %lu\n", __FUNCTION__, actual);
}
void sd_spi_go_low_frequency(sd_card_t *this) {
    uint actual = spi_set_baudrate(this->spi->pInst, 100 * 1000);
    TRACE_PRINTF("%s: Actual frequency: %lu\n", __FUNCTION__, actual);
}

static void sd_spi_lock(sd_card_t *this) {
    configASSERT(this->spi->mutex);
    xSemaphoreTakeRecursive(this->spi->mutex, portMAX_DELAY);
    this->spi->owner = xTaskGetCurrentTaskHandle();
}
static void sd_spi_unlock(sd_card_t *this) {
    this->spi->owner = 0;
    xSemaphoreGiveRecursive(this->spi->mutex);
}

void sd_spi_select(sd_card_t *this) {
    asm volatile("nop \n nop \n nop");  // FIXME
    gpio_put(this->ss_gpio, 0);
    asm volatile("nop \n nop \n nop");  // FIXME
}

void sd_spi_deselect(sd_card_t *this) {
    asm volatile("nop \n nop \n nop");  // FIXME
    gpio_put(this->ss_gpio, 1);
    asm volatile("nop \n nop \n nop");  // FIXME
    /*
    MMC/SDC enables/disables the DO output in synchronising to the SCLK. This
    means there is a posibility of bus conflict with MMC/SDC and another SPI
    slave that shares an SPI bus. Therefore to make MMC/SDC release the MISO
    line, the master device needs to send a byte after the CS signal is
    deasserted.
    */
    uint8_t fill = SPI_FILL_CHAR;
    spi_write_blocking(this->spi->pInst, &fill, 1);
}

void sd_spi_acquire(sd_card_t *this) {
    sd_spi_lock(this);
    sd_spi_select(this);
}

void sd_spi_release(sd_card_t *this) {
    sd_spi_deselect(this);
    sd_spi_unlock(this);
}

// SPI Transfer: Read & Write (simultaneously) on SPI bus
//   If the data that will be received is not important, pass NULL as rx.
//   If the data that will be transmitted is not important,
//     pass NULL as tx and then the SPI_FILL_CHAR is sent out as each data
//     element.
bool sd_spi_transfer(sd_card_t *this, const uint8_t *tx, uint8_t *rx,
                     size_t length) {
    //  TRACE_PRINTF("%s\n", __FUNCTION__);
    BaseType_t rc;

    configASSERT(512 == length);
    configASSERT(tx || rx);

    int num = 0;
    if (tx && rx) {
        num = spi_write_read_blocking(this->spi->pInst, tx, rx, length);
    } else if (tx) {
        num = spi_write_blocking(this->spi->pInst, tx, length);
    } else if (rx) {
        num = spi_read_blocking(this->spi->pInst, SPI_FILL_CHAR, rx, length);
    }
    configASSERT(num == length);

    return true;
}

uint8_t sd_spi_write(sd_card_t *this, const uint8_t value) {
    // TRACE_PRINTF("%s\n", __FUNCTION__);
    u_int8_t received = SPI_FILL_CHAR;

    int num = spi_write_read_blocking(this->spi->pInst, &value, &received, 1);
    configASSERT(1 == num);

    return received;
}

/* [] END OF FILE */
