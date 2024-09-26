
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
  https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main#customizing-for-the-hardware-configuration
*/

/* Hardware configuration for Expansion Module Type A
See https://oshwlab.com/carlk3/rpi-pico-sd-card-expansion-module-1
*/

#include "hw_config.h"

// Hardware Configuration of SPI "object"
static spi_t spi  = {  // One for each SPI controller used
    .hw_inst = spi0,  // SPI component
    .sck_gpio = 2,    // GPIO number (not Pico pin number)
    .mosi_gpio = 3,
    .miso_gpio = 4,
    .set_drive_strength = true,
    .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA,
    .spi_mode = 3,
    //.baud_rate = 125 * 1000 * 1000 / 10  // 12500000 Hz
    //.baud_rate = 125 * 1000 * 1000 / 8  // 15625000 Hz
    //.baud_rate = 125 * 1000 * 1000 / 6  // 20833333 Hz
    .baud_rate = 125 * 1000 * 1000 / 4  // 31250000 Hz
};

/* SPI Interface */
static sd_spi_if_t spi_if = {
    .spi = &spi,  // Pointer to the SPI driving this card
    .ss_gpio = 7  // The SPI slave select GPIO for this SD card
};

/* Configuration of the SD Card socket object */
static sd_card_t sd_card = {
    // "device_name" is arbitrary:
    .device_name = "sd0",
    // "mount_point" must be a directory off the file system's root directory and must be an
    // absolute path:
    .mount_point = "/sd0",
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if,  // Pointer to the SPI interface driving this card

    // SD Card detect:
    .use_card_detect = true,
    .card_detect_gpio = 9,  
    .card_detected_true = 0, // What the GPIO read returns when a card is present.
};

/* ********************************************************************** */

size_t sd_get_num() { return 1; }

/**
 * @brief Returns a pointer to the SD card object with the given number.
 * @param num The number of the SD card object to return.
 * @return A pointer to the SD card object if found, NULL if not found.
 */
sd_card_t *sd_get_by_num(size_t num) {
    if (0 == num) {
        // Device 0 is the only one supported in this example.
        return &sd_card;
    } else {
        // No other devices are supported in this example.
        return NULL;
    }
}

/* [] END OF FILE */
