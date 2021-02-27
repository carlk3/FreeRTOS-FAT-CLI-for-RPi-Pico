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

/* FreeRTOS includes. */
#include "FreeRTOS.h"
//#include "FreeRTOSFATConfig.h" // for DBG_PRINTF
//
#include "my_debug.h"
#include "sd_card.h"
#include "sd_spi.h"
#include "spi.h"
//

//#define TRACE_PRINTF(fmt, args...)
#define TRACE_PRINTF printf  // task_printf

void sd_spi_go_high_frequency(sd_card_t *this) {
    uint actual = spi_set_baudrate(this->spi->hw_inst, this->spi->baud_rate);
    TRACE_PRINTF("%s: Actual frequency: %lu\n", __FUNCTION__, actual);
}
void sd_spi_go_low_frequency(sd_card_t *this) {
    uint actual = spi_set_baudrate(this->spi->hw_inst, 100 * 1000);
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

// Would do nothing if this->ss_gpio were set to GPIO_FUNC_SPI.
static void sd_spi_select(sd_card_t *this) {
    //asm volatile("nop \n nop \n nop");  // FIXME
    gpio_put(this->ss_gpio, 0);
    //asm volatile("nop \n nop \n nop");  // FIXME
    LED_ON();
}

static void sd_spi_deselect(sd_card_t *this) {
    //asm volatile("nop \n nop \n nop");  // FIXME
    gpio_put(this->ss_gpio, 1);
    //asm volatile("nop \n nop \n nop");  // FIXME
    LED_OFF();
    /*
    MMC/SDC enables/disables the DO output in synchronising to the SCLK. This
    means there is a posibility of bus conflict with MMC/SDC and another SPI
    slave that shares an SPI bus. Therefore to make MMC/SDC release the MISO
    line, the master device needs to send a byte after the CS signal is
    deasserted.
    */
    uint8_t fill = SPI_FILL_CHAR;
    spi_write_blocking(this->spi->hw_inst, &fill, 1);
}

void sd_spi_acquire(sd_card_t *this) {
    sd_spi_lock(this);
    sd_spi_select(this);
}

void sd_spi_release(sd_card_t *this) {
    sd_spi_deselect(this);
    sd_spi_unlock(this);
}

uint8_t sd_spi_write(sd_card_t *this, const uint8_t value) {
    // TRACE_PRINTF("%s\n", __FUNCTION__);
    u_int8_t received = SPI_FILL_CHAR;

    configASSERT(xTaskGetCurrentTaskHandle() == this->spi->owner);

    int num = spi_write_read_blocking(this->spi->hw_inst, &value, &received, 1);
    configASSERT(1 == num);

    return received;
}

bool sd_spi_transfer(sd_card_t *this, const uint8_t *tx, uint8_t *rx,
                     size_t length) {
    return spi_transfer(this->spi, tx, rx, length);
}

//void sd_spi_init_manual() {
void sd_spi_init(sd_card_t *this) {
    // Chip select is active-low, so we'll initialise it to a driven-high
    // state.
    gpio_init(this->ss_gpio);
    gpio_put(this->ss_gpio, 1);
    gpio_set_dir(this->ss_gpio, GPIO_OUT);
}
void sd_spi_init_pl022(sd_card_t *this) {
    // Let the PL022 SPI handle it.
    // the CS line is brought high between each byte during transmission.
    gpio_set_function(this->ss_gpio, GPIO_FUNC_SPI);
}

/* [] END OF FILE */
