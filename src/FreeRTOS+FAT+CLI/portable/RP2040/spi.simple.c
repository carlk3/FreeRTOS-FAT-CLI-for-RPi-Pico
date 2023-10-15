/* spi.simple.c
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
#include <string.h>

#include "my_debug.h"
//
#include "spi.h"

bool my_spi_init(spi_t *pSPI) {
    // bool __atomic_test_and_set (void *ptr, int memorder)
    // This built-in function performs an atomic test-and-set operation on the
    // byte at *ptr. The byte is set to some implementation defined nonzero
    // “set” value and the return value is true if and only if the previous
    // contents were “set”.
    if (__atomic_test_and_set(&(pSPI->initialized), __ATOMIC_SEQ_CST))
        return true;

    /* Configure component */
    // Enable SPI at 100 kHz and connect to GPIOs
    spi_init(pSPI->hw_inst, 100 * 1000);
    spi_set_format(pSPI->hw_inst, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(pSPI->miso_gpio, GPIO_FUNC_SPI);
    gpio_set_function(pSPI->mosi_gpio, GPIO_FUNC_SPI);
    gpio_set_function(pSPI->sck_gpio, GPIO_FUNC_SPI);

    // SD cards' DO MUST be pulled up.
    gpio_pull_up(pSPI->miso_gpio);

    // The SPI may be shared (using multiple SSs); protect it
    pSPI->mutex = xSemaphoreCreateRecursiveMutex();

    LED_INIT();

    return true;
}

/* [] END OF FILE */
