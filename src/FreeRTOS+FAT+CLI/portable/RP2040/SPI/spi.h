/* spi.h
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

#ifndef _SPI_H_
#define _SPI_H_

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
//
#include <stdbool.h>
//
// Pico includes
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "pico/types.h"

#define SPI_FILL_CHAR (0xFF)

// "Class" representing SPIs
typedef struct {
    // SPI HW
    spi_inst_t *hw_inst;
    uint miso_gpio;  // SPI MISO GPIO number (not pin number)
    uint mosi_gpio;
    uint sck_gpio;
    uint baud_rate;
    // State variables:
    uint tx_dma;
    uint rx_dma;
    dma_channel_config tx_dma_cfg;
    dma_channel_config rx_dma_cfg;
    irq_handler_t dma_isr;
    bool initialized;         // Assigned dynamically
    TaskHandle_t owner;       // Assigned dynamically
    SemaphoreHandle_t mutex;  // Assigned dynamically
} spi_t;

#ifdef __cplusplus
extern "C" {
#endif

    void spi_irq_handler(spi_t *pSPI);
    bool spi_transfer(spi_t *pSPI, const uint8_t *tx, uint8_t *rx, size_t length);
    bool my_spi_init(spi_t *pSPI);

#ifdef __cplusplus
}
#endif

#ifndef NO_PICO_LED
#define USE_LED 1
#endif
#if USE_LED
#   define LED_PIN 25
#   include "hardware/gpio.h"
#   define LED_INIT()                       \
    {                                    \
        gpio_init(LED_PIN);              \
        gpio_set_dir(LED_PIN, GPIO_OUT); \
    }
#   define LED_ON() gpio_put(LED_PIN, 1)
#   define LED_OFF() gpio_put(LED_PIN, 0)
#else
#   define LED_ON()
#   define LED_OFF()
#   define LED_INIT()
#endif

#endif
/* [] END OF FILE */
