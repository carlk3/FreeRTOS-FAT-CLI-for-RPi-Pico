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
/*

This file should be tailored to match the hardware design that is specified in
TopDesign.cysch and Design Wide Resources/Pins (the .cydwr file).

There should be one element of the spi[] array for each hardware SPI used.

There should be one element of the sd_cards[] array for each SD card slot.
The name is arbitrary. The rest of the constants will depend on the type of
socket, which SPI it is driven by, and how it is wired.

*/

#include <string.h>

// Make it easier to spot errors:
#include "hw_config.h"

// Hardware Configuration of SPI "objects"
// Note: multiple SD cards can be driven by one SPI if they use different slave
// selects.
static spi_t spi[] = {  // One for each SPI.
    {
        .pInst = spi0,  // SPI component
        .miso_gpio = 19,
        .mosi_gpio = 16,
        .sck_gpio = 18,
        // Following attributes are dynamically assigned
        .initialized = false,  // initialized flag
        .owner = 0,            // Owning task, assigned dynamically
        .mutex = 0             // Guard semaphore, assigned dynamically
    }};

// Hardware Configuration of the SD Card "objects"
static sd_card_t sd_cards[] = {  // One for each SD card
    {.pcName = "SDCard",         // Name used to mount device
     .spi = &spi[0],             // Pointer to the SPI driving this card
     .ss_gpio = 17,              // The SPI slave select line for this SD card
     .card_detect_gpio = 22,     // Card detect
     .card_detected_true = 0,    // What the GPIO read returns when a card is
                                 // present. Use -1 if there is no card detect.
     // Following attributes are dynamically assigned
     .card_detect_task = 0,
     .m_Status = STA_NOINIT,
     .sectors = 0,
     .card_type = 0,
     .mutex = 0,
     .ff_disk_count = 0,
     .ff_disks = NULL}};

/* ********************************************************************** */
sd_card_t *sd_get_by_name(const char *const name) {
    size_t i;
    for (i = 0; i < sizeof(sd_cards) / sizeof(sd_cards[0]); ++i) {
        if (0 == strcmp(sd_cards[i].pcName, name)) break;
    }
    if (sizeof(sd_cards) / sizeof(sd_cards[0]) == i) {
        DBG_PRINTF("FF_SDDiskInit: unknown name %s\n", name);
        return NULL;
    }
    return &sd_cards[i];
}

sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sizeof(sd_cards) / sizeof(sd_cards[0])) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}

/* [] END OF FILE */
