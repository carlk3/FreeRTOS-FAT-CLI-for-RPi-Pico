/* Instead of a statically linked hw_config.c,
   create configuration dynamically.

   This file should be tailored to match the hardware design.

    */ \
#include "FreeRTOS.h" /* Must come first. */
//
#include <stdio.h>

#include <string>
#include <vector>

//
#include "pico/stdlib.h"
//
#include "ff_headers.h"
#include "ff_sddisk.h"
#include "ff_stdio.h"
#include "ff_utils.h"
//
#include "hw_config.h"
static std::vector<spi_t *> spis;             // SPI H/W components
static std::vector<sd_spi_if_t *> spi_ifs;    // SPI Interfaces
static std::vector<sd_sdio_if_t *> sdio_ifs;  // SDIO Interfaces
static std::vector<sd_card_t *> sd_cards;     // SD Card Sockets

size_t sd_get_num() { return sd_cards.size(); }

sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return sd_cards[num];
    } else {
        return NULL;
    }
}

#define stop()          \
    {                   \
        fflush(stdout); \
        __breakpoint(); \
    }

// See https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/Standard_File_System_API.html

static void SimpleTask(void *arg) {
    sd_card_t *sd_card_p = (sd_card_t *)arg;

    FF_PRINTF("%s: Hello, %s!\n",
           pcTaskGetName(NULL), sd_card_p->device_name);

    FF_Disk_t *pxDisk = FF_SDDiskInit(sd_card_p->device_name);
    configASSERT(pxDisk);
    FF_Error_t xError = FF_SDDiskMount(pxDisk);
    if (FF_isERR(xError) != pdFALSE) {
        FF_PRINTF("FF_SDDiskMount: %s\n",
                  (const char *)FF_GetErrMessage(xError));
        stop();
    }
    FF_FS_Add(sd_card_p->mount_point, pxDisk);

    std::string filename = std::string(sd_card_p->mount_point) + "/filename.txt";
    FF_FILE *pxFile = ff_fopen(filename.c_str(), "a");
    if (!pxFile) {
        FF_PRINTF("ff_fopen(%s) failed: %s (%d)\n",
                  filename.c_str(), strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
        stop();
    }
    if (ff_fprintf(pxFile, "Hello, world!\n") < 0) {
        FF_PRINTF("ff_fprintf failed: %s (%d)\n", strerror(stdioGET_ERRNO()),
                  stdioGET_ERRNO());
        stop();
    }
    if (-1 == ff_fclose(pxFile)) {
        FF_PRINTF("ff_fclose failed: %s (%d)\n", strerror(stdioGET_ERRNO()),
                  stdioGET_ERRNO());
        stop();
    }
    FF_FS_Remove(sd_card_p->mount_point);
    FF_Unmount(pxDisk);
    FF_SDDiskDelete(pxDisk);
    FF_PRINTF("%s: Goodbye, world!\n", pcTaskGetName(NULL));

    vTaskDelete(NULL);
}

int main() {
    stdio_init_all();

    // spis[0]
    spi_t *spi_p = new spi_t();
    configASSERT(spi_p);
    spi_p->hw_inst = spi1;  // RP2040 SPI component
    spi_p->miso_gpio = 8;
    spi_p->sck_gpio = 10;   // GPIO number (not Pico pin number)
    spi_p->mosi_gpio = 11;
    spis.push_back(spi_p);

    /* SPI Interfaces */

    // spi_ifs[0]
    sd_spi_if_t *spi_if_p = new sd_spi_if_t();
    configASSERT(spi_if_p);
    spi_if_p->spi = spis[0];  // Pointer to the SPI driving this card
    spi_if_p->ss_gpio = 12;   // The SPI slave select GPIO for this SD card
    spi_ifs.push_back(spi_if_p);

    /* SDIO Interfaces */

    // sdio_ifs[0]
    sd_sdio_if_t *sd_sdio_if_p = new sd_sdio_if_t();
    configASSERT(sd_sdio_if_p);
    sd_sdio_if_p->CMD_gpio = 17;
    sd_sdio_if_p->D0_gpio = 18;
    sdio_ifs.push_back(sd_sdio_if_p);

    /* Hardware Configuration of the SD Card "objects" */

    // sd_cards[0]
    sd_card_t *sd_card_p = new sd_card_t();
    configASSERT(sd_card_p);
    sd_card_p->device_name = "sd0";  // Name used to mount device
    sd_card_p->mount_point = "/sd0";
    sd_card_p->type = SD_IF_SDIO;
    sd_card_p->sdio_if_p = sdio_ifs[0];  // Pointer to the SPI interface driving this card
    sd_card_p->use_card_detect = true;
    sd_card_p->card_detect_gpio = 9;
    sd_card_p->card_detected_true = 0;  // What the GPIO read returns when a card is present
    sd_card_p->card_detect_use_pull = true;
    sd_card_p->card_detect_pull_hi = true;
    sd_cards.push_back(sd_card_p);

    // sd_cards[1]: Socket sd1
    sd_card_p = new sd_card_t();
    configASSERT(sd_card_p);
    sd_card_p->device_name = "sd1";  // Name used to mount device
    sd_card_p->mount_point = "/sd1";
    sd_card_p->type = SD_IF_SPI;
    sd_card_p->spi_if_p = spi_ifs[0];  // Pointer to the SPI interface driving this card
    sd_card_p->use_card_detect = true;
    sd_card_p->card_detect_gpio = 14;
    sd_card_p->card_detected_true = 0;  // What the GPIO read returns when a card is present
    sd_card_p->card_detect_use_pull = true;
    sd_card_p->card_detect_pull_hi = true;
    sd_cards.push_back(sd_card_p);

    xTaskCreate(SimpleTask, "SimpleTask 0", 1024, sd_cards[0], configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(SimpleTask, "SimpleTask 1", 1024, sd_cards[1], configMAX_PRIORITIES - 1, NULL);

    vTaskStartScheduler();

    /* should never reach here */
    panic_unsupported();
}
