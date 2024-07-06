
/* hw_config.c
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

/*

This file should be tailored to match the hardware design.

See
https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico

There should be one element of the spi[] array for each RP2040 hardware SPI used.

There should be one element of the spi_ifs[] array for each SPI interface object.
* Each element of spi_ifs[] must point to an spi_t instance with member "spi".

There should be one element of the sdio_ifs[] array for each SDIO interface object.

There should be one element of the sd_cards[] array for each SD card slot.
* Each element of sd_cards[] must point to its interface with spi_if_p or sdio_if_p.
*/

//
#include "hw_config.h"

/* SDIO Interface */
static sd_sdio_if_t sdio_if = {
    /*
    Pins CLK_gpio, D1_gpio, D2_gpio, and D3_gpio are at offsets from pin D0_gpio.
    The offsets are determined by sd_driver\SDIO\rp2040_sdio.pio.
        CLK_gpio = (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32;
        As of this writing, SDIO_CLK_PIN_D0_OFFSET is 30,
            which is -2 in mod32 arithmetic, so:
        CLK_gpio = D0_gpio -2.
        D1_gpio = D0_gpio + 1;
        D2_gpio = D0_gpio + 2;
        D3_gpio = D0_gpio + 3;
    */
    .CMD_gpio = 17,
    .D0_gpio = 18,
    .baud_rate = 15 * 1000 * 1000  // 15 MHz
};

/* Configuration of the SD Card socket object */
static sd_card_t sd_card = {
    // "device_name" is arbitrary:
    .device_name = "sd0",
    // "mount_point" must be a directory off the file system's root directory and must be an absolute path:
    .mount_point = "/sd0",
    .type = SD_IF_SDIO,
    .spi_if_p = &sdio_if  // Pointer to the SPI interface driving this card
};

/* ********************************************************************** */

size_t sd_get_num() { return 1; }

sd_card_t *sd_get_by_num(size_t num) {
    if (0 == num) {
        return &sd_card;
    } else {
        return NULL;
    }
}

/* [] END OF FILE */
