/* sd_card.h
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

// Note: The model used here is one FatFS per SD card. 
// Multiple partitions on a card are not supported.

#pragma once

#include <stdint.h>
//
#include "FreeRTOS.h"
/* FreeRTOS includes. */
#include <semphr.h>
//
#include "hardware/gpio.h"
#include <hardware/pio.h>
#include "pico/mutex.h"
//
#include "ff_headers.h"
//
#include "SPI/spi.h"
#include "sd_card_constants.h"
#include "sd_regs.h"
#include "SDIO/rp2040_sdio.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Disk Status Bits (DSTATUS) */
enum {
    STA_NOINIT = 0x01, /* Drive not initialized */
    STA_NODISK = 0x02, /* No medium in the drive */
    STA_PROTECT = 0x04 /* Write protected */
};

typedef enum {
    SD_IF_SPI,
    SD_IF_SDIO
} sd_if_t;

typedef struct sd_spi_if_t {
    spi_t *spi;
    // Slave select is here instead of in spi_t because multiple SDs can share an SPI.
    uint ss_gpio;                   // Slave select for this SD card
    // Drive strength levels for GPIO outputs:
    // GPIO_DRIVE_STRENGTH_2MA 
    // GPIO_DRIVE_STRENGTH_4MA
    // GPIO_DRIVE_STRENGTH_8MA 
    // GPIO_DRIVE_STRENGTH_12MA
    bool set_drive_strength;
    enum gpio_drive_strength ss_gpio_drive_strength;
} sd_spi_if_t;

typedef struct sd_sdio_if_t {
    // See sd_driver\SDIO\rp2040_sdio.pio for SDIO_CLK_PIN_D0_OFFSET
    uint CLK_gpio;  // Must be (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32
    uint CMD_gpio;
    uint D0_gpio;      // D0
    uint D1_gpio;      // Must be D0 + 1
    uint D2_gpio;      // Must be D0 + 2
    uint D3_gpio;      // Must be D0 + 3
    PIO SDIO_PIO;      // either pio0 or pio1
    uint DMA_IRQ_num;  // DMA_IRQ_0 or DMA_IRQ_1
    bool use_exclusive_DMA_IRQ_handler;
    uint baud_rate;
    // Drive strength levels for GPIO outputs:
    // GPIO_DRIVE_STRENGTH_2MA 
    // GPIO_DRIVE_STRENGTH_4MA
    // GPIO_DRIVE_STRENGTH_8MA 
    // GPIO_DRIVE_STRENGTH_12MA
    bool set_drive_strength;
    enum gpio_drive_strength CLK_gpio_drive_strength;
    enum gpio_drive_strength CMD_gpio_drive_strength;
    enum gpio_drive_strength D0_gpio_drive_strength;
    enum gpio_drive_strength D1_gpio_drive_strength;
    enum gpio_drive_strength D2_gpio_drive_strength;
    enum gpio_drive_strength D3_gpio_drive_strength;

    /* The following fields are not part of the configuration. 
    They are state variables, and are dynamically assigned. */
    sd_sdio_state_t state;
} sd_sdio_if_t;

typedef struct sd_card_state_t {    
    int m_Status;      // Card status
    CSD_t CSD;         // Card-Specific Data register.
    CID_t CID;         // Card IDentification register
    uint64_t sectors;  // Assigned dynamically
    int card_type;     // Assigned dynamically
    SemaphoreHandle_t mutex;  // Guard semaphore, assigned dynamically
    TaskHandle_t owner;       // Assigned dynamically
    FF_Disk_t ff_disk;  // FreeRTOS+FAT "disk" using this device
} sd_card_state_t;

typedef struct sd_card_t sd_card_t;

// "Class" representing SD Cards
struct sd_card_t {
    const char *device_name;
    const char *mount_point; // Must be a directory off the file system's root directory and must be an absolute path that starts with a forward slash (/)
    sd_if_t type; // Interface type
    union {
        sd_spi_if_t *spi_if_p;
        sd_sdio_if_t *sdio_if_p;
    };
    bool use_card_detect;
    uint card_detect_gpio;    // Card detect; ignored if !use_card_detect
    uint card_detected_true;  // Varies with card socket; ignored if !use_card_detect
    bool card_detect_use_pull;
    bool card_detect_pull_hi;

    /* The following fields are state variables and not part of the configuration. 
    They are dynamically assigned. */
    sd_card_state_t state;

    int (*init)(sd_card_t *sd_card_p);
    void (*deinit)(sd_card_t *sd_card_p);

    int (*write_blocks)(sd_card_t *sd_card_p, const uint8_t *buffer,
                        uint64_t ulSectorNumber, uint32_t blockCnt);
    int (*read_blocks)(sd_card_t *sd_card_p, uint8_t *buffer, uint64_t ulSectorNumber,
                       uint32_t ulSectorCount);
    uint64_t (*get_num_sectors)(sd_card_t *sd_card_p);

    // Useful when use_card_detect is false - call periodically to check for presence of SD card
    // Returns true if and only if SD card was sensed on the bus
    bool (*sd_test_com)(sd_card_t *sd_card_p);
};

void sd_lock(sd_card_t *sd_card_p);
void sd_unlock(sd_card_t *sd_card_p);
bool sd_is_locked(sd_card_t *sd_card_p);

bool sd_init_driver();
bool sd_card_detect(sd_card_t *sd_card_p);
void cidDmp(sd_card_t *sd_card_p, printer_t printer);
void csdDmp(sd_card_t *sd_card_p, printer_t printer);
bool sd_allocation_unit(sd_card_t *sd_card_p, size_t *au_size_bytes_p);
sd_card_t *sd_get_by_name(const char *const name);
sd_card_t *sd_get_by_mount_point(const char *const name);

#ifdef __cplusplus
}
#endif

/* [] END OF FILE */
