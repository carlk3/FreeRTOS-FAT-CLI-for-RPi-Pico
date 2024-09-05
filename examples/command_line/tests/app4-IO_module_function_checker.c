/* app4-IO_module_function_checker.c
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
/*----------------------------------------------------------------------/
/ Low level disk I/O module function checker                            /
/-----------------------------------------------------------------------/
/ WARNING: The data on the target drive will be lost!
/
/ Original at http://elm-chan.org/fsw/ff/res/app4.c on 2020-23-02.
*/

#include <stdio.h>
#include <string.h>
// #include "ff.h"         /* Declarations of sector size */
// #include "diskio.h"     /* Declarations of disk functions */
//
#include "pico/mutex.h"
//
#include "hw_config.h"
#include "sd_card.h"

#define PRINTF IMSG_PRINTF

extern bool die_now;

typedef uint32_t DWORD;
typedef uint64_t QWORD;
typedef unsigned int UINT;
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef QWORD LBA_t;

/* Status of Disk Functions */
typedef BYTE DSTATUS;

/* Results of Disk Functions */
typedef enum {
    RES_OK = 0, /* 0: Successful */
    RES_ERROR,  /* 1: R/W Error */
    RES_WRPRT,  /* 2: Write Protected */
    RES_NOTRDY, /* 3: Not Ready */
    RES_PARERR  /* 4: Invalid Parameter */
} DRESULT;

#define FF_MIN_SS 512
#define FF_MAX_SS 512

static int sdrc2dresult(int sd_rc) {
    switch (sd_rc) {
        case SD_BLOCK_DEVICE_ERROR_NONE:
            return RES_OK;
        case SD_BLOCK_DEVICE_ERROR_UNUSABLE:
        case SD_BLOCK_DEVICE_ERROR_NO_RESPONSE:
        case SD_BLOCK_DEVICE_ERROR_NO_INIT:
        case SD_BLOCK_DEVICE_ERROR_NO_DEVICE:
            return RES_NOTRDY;
        case SD_BLOCK_DEVICE_ERROR_PARAMETER:
        case SD_BLOCK_DEVICE_ERROR_UNSUPPORTED:
            return RES_PARERR;
        case SD_BLOCK_DEVICE_ERROR_WRITE_PROTECTED:
            return RES_WRPRT;
        case SD_BLOCK_DEVICE_ERROR_CRC:
        case SD_BLOCK_DEVICE_ERROR_WOULD_BLOCK:
        case SD_BLOCK_DEVICE_ERROR_ERASE:
        case SD_BLOCK_DEVICE_ERROR_WRITE:
        default:
            return RES_ERROR;
    }
}

/* Pseudo random number generator */
static DWORD pn(DWORD pns /* 0:Initialize, !0:Read */, DWORD *plfsr) {
    UINT n;

    if (pns) {
        *plfsr = pns;
        for (n = 0; n < 32; n++) pn(0, plfsr);
    }
    if (*plfsr & 1) {
        *plfsr >>= 1;
        *plfsr ^= 0x80200003;
    } else {
        *plfsr >>= 1;
    }
    return *plfsr;
}

static sd_card_t *pdrv;
static WORD sz_sect;
static DWORD sz_eblk, sz_drv;

// uint64_t (*get_num_sectors)(sd_card_t *sd_card_p)
static uint64_t sd_sectors(sd_card_t *pdrv_) {
    return pdrv->get_num_sectors(pdrv_);
}
static DRESULT disk_write(sd_card_t *p_sd, const BYTE *buff, LBA_t sector, UINT count) {
    int rc = p_sd->write_blocks(p_sd, buff, sector, count);
    return sdrc2dresult(rc);
}
static DRESULT disk_read(sd_card_t *p_sd, BYTE *buff, LBA_t sector, UINT count) {
    int rc = p_sd->read_blocks(p_sd, buff, sector, count);
    return sdrc2dresult(rc);
}

int test_diskio_initialize(const char *diskName) {
    PRINTF(" disk_initalize(%p)", pdrv);
    //        ds = disk_initialize(pdrv);

    pdrv = sd_get_by_name(diskName);
    if (!pdrv) {
        PRINTF("sd_get_by_name: unknown name %s\n", diskName);
        return -1;
    }
    // Initialize the media driver
    if (!sd_init_driver()) {
        // Couldn't init
        PRINTF(" - failed.\n");
        fflush(stdout);
        return 2;
    } else {
        PRINTF(" - ok.\n");
    }
    PRINTF("**** Get drive size ****\n");
    PRINTF(" disk_ioctl(%p, GET_SECTOR_COUNT, 0x%08X)", pdrv, (UINT)&sz_drv);
    //        sz_drv = 0;
    //        dr = disk_ioctl(pdrv, GET_SECTOR_COUNT, &sz_drv);
    sz_drv = sd_sectors(pdrv);
    if (sz_drv < 128) {
        PRINTF("Failed: Insufficient drive size to test.\n");
        return 4;
    }
    PRINTF(" Number of sectors on the drive %p is %lu.\n", pdrv,
           (unsigned long)sz_drv);

#if FF_MAX_SS != FF_MIN_SS
    PRINTF("**** Get sector size ****\n");
    PRINTF(" disk_ioctl(%u, GET_SECTOR_SIZE, 0x%X)", pdrv, (UINT)&sz_sect);
    sz_sect = 0;
    dr = disk_ioctl(pdrv, GET_SECTOR_SIZE, &sz_sect);
    if (dr == RES_OK) {
        PRINTF(" - ok.\n");
    } else {
        PRINTF(" - failed.\n");
        fflush(stdout);
        mutex_exit(&test_diskio_initialize_mutex);
        return 5;
    }
    PRINTF(" Size of sector is %u bytes.\n", sz_sect);
#else
    sz_sect = FF_MAX_SS;
#endif

    PRINTF("**** Get block size ****\n");
    PRINTF(" disk_ioctl(%p, GET_BLOCK_SIZE, 0x%X)", pdrv, (UINT)&sz_eblk);
    //        sz_eblk = 0;
    //        dr = disk_ioctl(pdrv, GET_BLOCK_SIZE, &sz_eblk);
    //        sz_eblk = get_block_size();
    sz_eblk = 512;
    if (sz_eblk >= 2) {
        PRINTF(" Size of the erase block is %lu sectors.\n",
               (unsigned long)sz_eblk);
    } else {
        PRINTF(" Size of the erase block is unknown.\n");
    }
    return 0;
}

int test_diskio(UINT ncyc,    /* Number of test cycles */
                DWORD *buff,  /* Pointer to the working buffer */
                UINT sz_buff, /* Size of the working buffer in unit of byte */
                DWORD lba_offset) {
    //    BYTE pdrv = 0;  /* Physical drive number to be checked (all data on
    //    the drive will be lost) */
    UINT n, cc, ns;
    DWORD lba, lba2, pns = 1;
    BYTE *pbuff = (BYTE *)buff;
    //    DSTATUS ds;
    DRESULT dr;
    DWORD lfsr = 0;

    PRINTF("test_diskio(%p, %u, 0x%08X, 0x%08X)\n", pdrv, ncyc, (UINT)buff,
           sz_buff);

    if (sz_buff < FF_MAX_SS + 8) {
        PRINTF("Insufficient work area to run the program.\n");
        return 1;
    }

    for (cc = 1; cc <= ncyc && !die_now; cc++) {
        PRINTF("**** Test cycle %u of %u start ****\n", cc, ncyc);

        /* Single sector write test */
        PRINTF("**** Single sector write test ****\n");
        lba = lba_offset + 0;
        for (n = 0, pn(pns, &lfsr); n < sz_sect; n++) pbuff[n] = (BYTE)pn(0, &lfsr);
        PRINTF(" disk_write(%p, 0x%X, %lu, 1)", pdrv, (UINT)pbuff,
               (unsigned long)lba);
        dr = disk_write(pdrv, pbuff, lba, 1);
        if (dr == RES_OK) {
            PRINTF(" - ok.\n");
        } else {
            PRINTF(" - failed.\n");
            fflush(stdout);
            return 6;
        }
        //        PRINTF(" disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
        //        dr = disk_ioctl(pdrv, CTRL_SYNC, 0);
        //        if (dr == RES_OK) {
        //            PRINTF(" - ok.\n");
        //        } else {
        //            PRINTF(" - failed.\n"); fflush(stdout);
        //            return 7;
        //        }
        memset(pbuff, 0, sz_sect);
        PRINTF(" disk_read(%p, 0x%X, %lu, 1)", pdrv, (UINT)pbuff,
               (unsigned long)lba);
        dr = disk_read(pdrv, pbuff, lba, 1);
        if (dr == RES_OK) {
            PRINTF(" - ok.\n");
        } else {
            PRINTF(" - failed.\n");
            fflush(stdout);
            return 8;
        }
        for (n = 0, pn(pns, &lfsr); n < sz_sect && pbuff[n] == (BYTE)pn(0, &lfsr); n++)
            ;
        if (n == sz_sect) {
            PRINTF(" Read data matched.\n");
        } else {
            PRINTF(" Read data differs from the data written at %u.\n",
                   n - 1);
            fflush(stdout);
            return 10;
        }
        pns++;

        PRINTF("**** Multiple sector write test ****\n");
        lba = lba_offset + 5;
        ns = sz_buff / sz_sect;
        if (ns > 4) ns = 4;
        if (ns > 1) {
            for (n = 0, pn(pns, &lfsr); n < (UINT)(sz_sect * ns); n++)
                pbuff[n] = (BYTE)pn(0, &lfsr);
            PRINTF(" disk_write(%p, 0x%X, %lu, %u)", pdrv, (UINT)pbuff,
                   (unsigned long)lba, ns);
            dr = disk_write(pdrv, pbuff, lba, ns);
            if (dr == RES_OK) {
                PRINTF(" - ok.\n");
            } else {
                PRINTF(" - failed.\n");
                fflush(stdout);
                return 11;
            }
            //            PRINTF(" disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
            //            dr = disk_ioctl(pdrv, CTRL_SYNC, 0);
            //            if (dr == RES_OK) {
            //                PRINTF(" - ok.\n");
            //            } else {
            //                PRINTF(" - failed.\n"); fflush(stdout);
            //                return 12;
            //            }
            memset(pbuff, 0, sz_sect * ns);
            PRINTF(" disk_read(%p, 0x%X, %lu, %u)", pdrv, (UINT)pbuff,
                   (unsigned long)lba, ns);
            dr = disk_read(pdrv, pbuff, lba, ns);
            if (dr == RES_OK) {
                PRINTF(" - ok.\n");
            } else {
                PRINTF(" - failed.\n");
                fflush(stdout);
                return 13;
            }
            for (n = 0, pn(pns, &lfsr);
                 n < (UINT)(sz_sect * ns) && pbuff[n] == (BYTE)pn(0, &lfsr); n++)
                ;
            if (n == (UINT)(sz_sect * ns)) {
                PRINTF(" Read data matched.\n");
            } else {
                PRINTF(" Read data differs from the data written.\n");
                fflush(stdout);
                return 14;
            }
        } else {
            PRINTF(" Test skipped.\n");
        }
        pns++;

        PRINTF(
            "**** Single sector write test (unaligned buffer address) ****\n");
        lba = lba_offset + 5;
        for (n = 0, pn(pns, &lfsr); n < sz_sect; n++) pbuff[n + 3] = (BYTE)pn(0, &lfsr);
        PRINTF(" disk_write(%p, 0x%X, %lu, 1)", pdrv, (UINT)(pbuff + 3),
               (unsigned long)lba);
        dr = disk_write(pdrv, pbuff + 3, lba, 1);
        if (dr == RES_OK) {
            PRINTF(" - ok.\n");
        } else {
            PRINTF(" - failed.\n");
            fflush(stdout);
            return 15;
        }
        //        PRINTF(" disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
        //        dr = disk_ioctl(pdrv, CTRL_SYNC, 0);
        //        if (dr == RES_OK) {
        //            PRINTF(" - ok.\n");
        //        } else {
        //            PRINTF(" - failed.\n"); fflush(stdout);
        //            return 16;
        //        }
        memset(pbuff + 5, 0, sz_sect);
        PRINTF(" disk_read(%p, 0x%X, %lu, 1)", pdrv, (UINT)(pbuff + 5),
               (unsigned long)lba);
        dr = disk_read(pdrv, pbuff + 5, lba, 1);
        if (dr == RES_OK) {
            PRINTF(" - ok.\n");
        } else {
            PRINTF(" - failed.\n");
            fflush(stdout);
            return 17;
        }
        for (n = 0, pn(pns, &lfsr); n < sz_sect && pbuff[n + 5] == (BYTE)pn(0, &lfsr); n++)
            ;
        if (n == sz_sect) {
            PRINTF(" Read data matched.\n");
        } else {
            PRINTF(" Read data differs from the data written.\n");
            fflush(stdout);
            return 18;
        }
        pns++;

        PRINTF("**** 4GB barrier test ****\n");
        if (sz_drv >= 128 + 0x80000000 / (sz_sect / 2)) {
            lba = lba_offset + 6;
            lba2 = lba + 0x80000000 / (sz_sect / 2);
            for (n = 0, pn(pns, &lfsr); n < (UINT)(sz_sect * 2); n++)
                pbuff[n] = (BYTE)pn(0, &lfsr);
            PRINTF(" disk_write(%p, 0x%X, %lu, 1)", pdrv, (UINT)pbuff,
                   (unsigned long)lba);
            dr = disk_write(pdrv, pbuff, lba, 1);
            if (dr == RES_OK) {
                PRINTF(" - ok.\n");
            } else {
                PRINTF(" - failed.\n");
                fflush(stdout);
                return 19;
            }
            PRINTF(" disk_write(%p, 0x%X, %lu, 1)", pdrv,
                   (UINT)(pbuff + sz_sect), (unsigned long)lba2);
            dr = disk_write(pdrv, pbuff + sz_sect, lba2, 1);
            if (dr == RES_OK) {
                PRINTF(" - ok.\n");
            } else {
                PRINTF(" - failed.\n");
                fflush(stdout);
                return 20;
            }
            //            PRINTF(" disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
            //            dr = disk_ioctl(pdrv, CTRL_SYNC, 0);
            //            if (dr == RES_OK) {
            //            PRINTF(" - ok.\n");
            //            } else {
            //                PRINTF(" - failed.\n"); fflush(stdout);
            //                return 21;
            //            }
            memset(pbuff, 0, sz_sect * 2);
            PRINTF(" disk_read(%p, 0x%X, %lu, 1)", pdrv, (UINT)pbuff,
                   (unsigned long)lba);
            dr = disk_read(pdrv, pbuff, lba, 1);
            if (dr == RES_OK) {
                PRINTF(" - ok.\n");
            } else {
                PRINTF(" - failed.\n");
                fflush(stdout);
                return 22;
            }
            PRINTF(" disk_read(%p, 0x%X, %lu, 1)", pdrv,
                   (UINT)(pbuff + sz_sect), (unsigned long)lba2);
            dr = disk_read(pdrv, pbuff + sz_sect, lba2, 1);
            if (dr == RES_OK) {
                PRINTF(" - ok.\n");
            } else {
                PRINTF(" - failed.\n");
                fflush(stdout);
                return 23;
            }
            for (n = 0, pn(pns, &lfsr);
                 pbuff[n] == (BYTE)pn(0, &lfsr) && n < (UINT)(sz_sect * 2); n++)
                ;
            if (n == (UINT)(sz_sect * 2)) {
                PRINTF(" Read data matched.\n");
            } else {
                PRINTF(" Read data differs from the data written.\n");
                return 24;
            }
        } else {
            PRINTF(" Test skipped.\n");
        }
        pns++;

        PRINTF("**** Test cycle %u of %u completed ****\n\n", cc, ncyc);
    }

    return 0;
}

// int main (int argc, char* argv[])
int low_level_io_tests(const char *diskName) {
    const DWORD lba_offset = 0;
    const UINT ncyc = 5;

    int rc;
    // DWORD buff[FF_MAX_SS];  /* Working buffer (4 sector in size) */
    const size_t buff_sz = FF_MAX_SS * sizeof(DWORD);

    DWORD *buff = pvPortMalloc(buff_sz);
    if (!buff) {
        PRINTF("%s: Couldn't allocate %zu bytes\n", __FUNCTION__, buff_sz);
        return -1;
    }

    rc = test_diskio_initialize(diskName);
    if (!rc) {
        /* Check function/compatibility of the physical drive #0 */
        rc = test_diskio(ncyc, buff, buff_sz, lba_offset);
    }
    if (rc) {
        PRINTF(
            "Sorry the function/compatibility test failed. (rc=%d)\nFatFs will "
            "not work with this disk driver.\n",
            rc);
    } else {
        PRINTF("Congratulations! The disk driver works well.\n");
    }
    vPortFree(buff);

    return rc;
}
