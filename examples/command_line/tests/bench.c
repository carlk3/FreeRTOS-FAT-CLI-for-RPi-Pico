/* Ported from: https://github.com/greiman/SdFat/blob/master/examples/bench/bench.ino
 *
 * This program is a simple binary write/read benchmark.
 */
#include <my_debug.h>
// #include <string.h>

#include "FreeRTOS_strerror.h"
#include "sd_card.h"
#include "ff_utils.h"
#include "hw_config.h"
//
#include "ff_stdio.h"

#define error(s)                       \
    {                                  \
        EMSG_PRINTF("ERROR: %s\n", s); \
        __breakpoint();                \
    }

static uint32_t millis() {
    return to_ms_since_boot(get_absolute_time());
}
static uint64_t micros() {
    return to_us_since_boot(get_absolute_time());
}
// static sd_card_t* sd_get_by_name(const char* const name) {
//     for (size_t i = 0; i < sd_get_num(); ++i)
//         if (0 == strcmp(sd_get_by_num(i)->pcName, name)) return sd_get_by_num(i);
//     EMSG_PRINTF("%s: unknown name %s\n", __func__, name);
//     return NULL;
// }

// Set SKIP_FIRST_LATENCY true if the first read/write to the SD can
// be avoid by writing a file header or reading the first record.
static const bool SKIP_FIRST_LATENCY = true;

// Size of read/write in bytes
#define BUF_SIZE 65536  // size of an erasable sector

// File size in MiB where MiB = 1048576 bytes.
#define FILE_SIZE_MiB 5

// Write pass count.
static const uint8_t WRITE_COUNT = 2;

// Read pass count.
static const uint8_t READ_COUNT = 2;

static char const *const pathname = "bench.dat";

//==============================================================================
// End of configuration constants.
//------------------------------------------------------------------------------

// File size in bytes.
// static const uint32_t FILE_SIZE = 1000000UL * FILE_SIZE_MB;
#define FILE_SIZE (1024 * 1024 * FILE_SIZE_MiB)

static void bench_test(FF_FILE *file_p, uint8_t buf[BUF_SIZE]) {
    float s;
    uint32_t t;
    uint32_t maxLatency;
    uint32_t minLatency;
    uint32_t totalLatency;
    bool skipLatency;

    IMSG_PRINTF("\nStarting write test, please wait.\n\n");  // << endl
                                                             // << endl;
    // do write test
    uint32_t n = FILE_SIZE / BUF_SIZE;
    IMSG_PRINTF("write speed and latency\n");
    IMSG_PRINTF("speed,max,min,avg\n");
    IMSG_PRINTF("KB/Sec,usec,usec,usec\n");
    for (uint8_t nTest = 0; nTest < WRITE_COUNT; nTest++) {
        ff_rewind(file_p);
        maxLatency = 0;
        minLatency = 9999999;
        totalLatency = 0;
        skipLatency = SKIP_FIRST_LATENCY;
        t = millis();
        for (uint32_t i = 0; i < n; i++) {
            uint32_t m = micros();
            size_t bw = ff_fwrite(buf, 1, BUF_SIZE, file_p); /* Write it to the destination file */
            if (BUF_SIZE != bw) {
                EMSG_PRINTF("ff_fwrite: %s\n", FreeRTOS_strerror(stdioGET_ERRNO()));
                return;
            }
            m = micros() - m;
            totalLatency += m;
            if (skipLatency) {
                // Wait until first write to SD, not just a copy to the cache.
                // skipLatency = file.curPosition() < 512;
                skipLatency = ff_ftell(file_p) < 512;
            } else {
                if (maxLatency < m) {
                    maxLatency = m;
                }
                if (minLatency > m) {
                    minLatency = m;
                }
            }
        }
        t = millis() - t;
        s = ff_filelength(file_p);
        IMSG_PRINTF("%.1f,%lu,%lu", s / t, maxLatency, minLatency);
        IMSG_PRINTF(",%lu\n", totalLatency / n);
    }
    IMSG_PRINTF("\nStarting read test, please wait.\n");
    IMSG_PRINTF("\nread speed and latency\n");
    IMSG_PRINTF("speed,max,min,avg\n");
    IMSG_PRINTF("KB/Sec,usec,usec,usec\n");

    // do read test
    for (uint8_t nTest = 0; nTest < READ_COUNT; nTest++) {
        ff_rewind(file_p);
        maxLatency = 0;
        minLatency = 9999999;
        totalLatency = 0;
        skipLatency = SKIP_FIRST_LATENCY;
        t = millis();
        for (uint32_t i = 0; i < n; i++) {
            buf[BUF_SIZE - 1] = 0;
            uint32_t m = micros();
            size_t nr = ff_fread(buf, 1, BUF_SIZE, file_p);
            if (BUF_SIZE != nr) {
                EMSG_PRINTF("ff_fread: %s\n", FreeRTOS_strerror(stdioGET_ERRNO()));
                return;
            }
            m = micros() - m;
            totalLatency += m;
            if (buf[BUF_SIZE - 1] != '\n') {
                error("data check error");
            }
            if (skipLatency) {
                skipLatency = false;
            } else {
                if (maxLatency < m) {
                    maxLatency = m;
                }
                if (minLatency > m) {
                    minLatency = m;
                }
            }
        }
        s = ff_filelength(file_p);
        t = millis() - t;
        IMSG_PRINTF("%.1f,%lu,%lu", s / t, maxLatency, minLatency);
        IMSG_PRINTF(",%lu\n", totalLatency / n);
    }
    IMSG_PRINTF("\nDone\n");
}
static void bench_open_close(sd_card_t *sd_card_p, uint8_t *buf) {
    FF_PRINTF("Reading FAT and calculating Free Space\n");
    switch (sd_card_p->state.ff_disk.pxIOManager->xPartition.ucType) {
        case FF_T_FAT12:
            IMSG_PRINTF("Type is FAT12");
            break;

        case FF_T_FAT16:
            IMSG_PRINTF("Type is FAT16");
            break;

        case FF_T_FAT32:
            IMSG_PRINTF("Type is FAT32");
            break;

        default:
                IMSG_PRINTF("Type is UNKNOWN)");
                break;
    }

    // IMSG_PRINTF("Card size: ");
    // IMSG_PRINTF("%.2f", (float)sd_card_p->get_num_sectors(sd_card_p) * 512 / 1000000000);
    // IMSG_PRINTF(" GB (GB = 1E9 bytes)\n");

    cidDmp(sd_card_p, info_message_printf);
    csdDmp(sd_card_p, info_message_printf);

    // fill buf with known data
    if (BUF_SIZE > 1) {
        for (size_t i = 0; i < (BUF_SIZE - 2); i++) {
                buf[i] = 'A' + (i % 26);
        }
        buf[BUF_SIZE - 2] = '\r';
    }
    buf[BUF_SIZE - 1] = '\n';

    /* Open the file, creating the file if it does not already exist. */
    FF_Stat_t xStat;
    size_t fsz = 0;
    if (ff_stat(pathname, &xStat) == 0)
        fsz = xStat.st_size;
    static FF_FILE *file_p;
    if (0 < fsz && fsz <= FILE_SIZE) {
        // This is an attempt at optimization:
        // rewriting the file should be faster than
        // writing it from scratch.
        file_p = ff_fopen(pathname, "r+");
    } else {
        file_p = ff_fopen(pathname, "rw");
    }
    if (!file_p) {
        EMSG_PRINTF("ff_fopen: %s\n", FreeRTOS_strerror(stdioGET_ERRNO()));
        return;
    }
    IMSG_PRINTF("\nFILE_SIZE_MB = %d\n", FILE_SIZE_MiB);  // << FILE_SIZE_MB << endl;
    IMSG_PRINTF("BUF_SIZE = %zu\n", BUF_SIZE);            // << BUF_SIZE << F(" bytes\n");

    bench_test(file_p, buf);

    int rc = ff_fclose(file_p);
    if (-1 == rc) {
        EMSG_PRINTF("ff_fclose: %s\n", FreeRTOS_strerror(stdioGET_ERRNO()));
    }
}

//------------------------------------------------------------------------------
void bench() {
    static_assert(0 == FILE_SIZE % BUF_SIZE,
                  "For accurate results, FILE_SIZE must be a multiple of BUF_SIZE.");

    uint8_t *buf = pvPortMalloc(BUF_SIZE);
    if (!buf) {
        EMSG_PRINTF("pvPortMalloc(%zu) failed\n", BUF_SIZE);
    }

    sd_card_t *sd_card_p = get_current_sd_card_p();
    if (!sd_card_p) return;
    bench_open_close(sd_card_p, buf);

    vPortFree(buf);
}
