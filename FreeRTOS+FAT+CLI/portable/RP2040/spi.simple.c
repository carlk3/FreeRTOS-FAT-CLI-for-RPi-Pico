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

#include <string.h>

#include "my_debug.h"
//
#include "spi.h"

bool my_spi_init(spi_t *this) {
    // bool __atomic_test_and_set (void *ptr, int memorder)
    // This built-in function performs an atomic test-and-set operation on the
    // byte at *ptr. The byte is set to some implementation defined nonzero
    // “set” value and the return value is true if and only if the previous
    // contents were “set”.
    if (__atomic_test_and_set(&(this->initialized), __ATOMIC_SEQ_CST))
        return true;

    /* Configure component */
    // Enable SPI at 100 kHz and connect to GPIOs
    spi_init(this->hw_inst, 100 * 1000);
    spi_set_format(this->hw_inst, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(this->miso_gpio, GPIO_FUNC_SPI);
    gpio_set_function(this->mosi_gpio, GPIO_FUNC_SPI);
    gpio_set_function(this->sck_gpio, GPIO_FUNC_SPI);

    // SD cards' DO MUST be pulled up.
    gpio_pull_up(this->miso_gpio);

    // The SPI may be shared (using multiple SSs); protect it
    this->mutex = xSemaphoreCreateRecursiveMutex();

    LED_INIT();

    return true;
}

/* [] END OF FILE */
