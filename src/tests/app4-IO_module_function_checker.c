/*----------------------------------------------------------------------/
/ Low level disk I/O module function checker                            /
/-----------------------------------------------------------------------/
/ WARNING: The data on the target drive will be lost!
/
/ Original at http://elm-chan.org/fsw/ff/res/app4.c on 2020-23-02. 
*/

#include <stdio.h>
#include <string.h>
//#include "ff.h"         /* Declarations of sector size */
//#include "diskio.h"     /* Declarations of disk functions */

#include "hw_config.h"

typedef uint32_t DWORD;
typedef unsigned int UINT;
typedef uint8_t BYTE;
typedef uint16_t WORD;

/* Status of Disk Functions */
typedef BYTE	DSTATUS;

/* Results of Disk Functions */
typedef enum {
	RES_OK = 0,		/* 0: Successful */
	RES_ERROR,		/* 1: R/W Error */
	RES_WRPRT,		/* 2: Write Protected */
	RES_NOTRDY,		/* 3: Not Ready */
	RES_PARERR		/* 4: Invalid Parameter */
} DRESULT;

#define FF_MIN_SS		512
#define FF_MAX_SS		512

#define disk_read sd_read_blocks
#define disk_write sd_write_blocks

static DWORD pn (       /* Pseudo random number generator */
    DWORD pns   /* 0:Initialize, !0:Read */
)
{
    static DWORD lfsr;
    UINT n;


    if (pns) {
        lfsr = pns;
        for (n = 0; n < 32; n++) pn(0);
    }
    if (lfsr & 1) {
        lfsr >>= 1;
        lfsr ^= 0x80200003;
    } else {
        lfsr >>= 1;
    }
    return lfsr;
}

int test_diskio (
    const char *diskName,
    UINT ncyc,      /* Number of test cycles */
    DWORD* buff,    /* Pointer to the working buffer */
    UINT sz_buff    /* Size of the working buffer in unit of byte */
)
{
//    BYTE pdrv = 0;  /* Physical drive number to be checked (all data on the drive will be lost) */   
    sd_card_t *pdrv = 0;
    UINT n, cc, ns;
    DWORD sz_drv, lba, lba2, sz_eblk, pns = 1;
    WORD sz_sect;
    BYTE *pbuff = (BYTE*)buff;
//    DSTATUS ds;
    DRESULT dr;

    printf("test_diskio(%p, %u, 0x%08X, 0x%08X)\n", pdrv, ncyc, (UINT)buff, sz_buff);

    if (sz_buff < FF_MAX_SS + 8) {
        printf("Insufficient work area to run the program.\n");
        return 1;
    }
    
    for (cc = 1; cc <= ncyc; cc++) {
        printf("**** Test cycle %u of %u start ****\n", cc, ncyc);

        printf(" disk_initalize(%p)", pdrv);
//        ds = disk_initialize(pdrv);

        pdrv = sd_get_by_name(diskName);
        if (!pdrv) {
            printf("sd_get_by_name: unknown name %s\n", diskName);
			return -1;            
        }
		// Initialize the media driver
		if (0 != sd_init(pdrv)) {
			// Couldn't init
            printf(" - failed.\n");
            return 2;
        } else {
            printf(" - ok.\n");
        }
        printf("**** Get drive size ****\n");
        printf(" disk_ioctl(%p, GET_SECTOR_COUNT, 0x%08X)", pdrv, (UINT)&sz_drv);
//        sz_drv = 0;
//        dr = disk_ioctl(pdrv, GET_SECTOR_COUNT, &sz_drv);
        dr = RES_OK;
        sz_drv = sd_sectors(pdrv);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 3;
        }
        if (sz_drv < 128) {
            printf("Failed: Insufficient drive size to test.\n");
            return 4;
        }
        printf(" Number of sectors on the drive %p is %lu.\n", pdrv, (unsigned long)sz_drv);

#if FF_MAX_SS != FF_MIN_SS
        printf("**** Get sector size ****\n");
        printf(" disk_ioctl(%u, GET_SECTOR_SIZE, 0x%X)", pdrv, (UINT)&sz_sect);
        sz_sect = 0;
        dr = disk_ioctl(pdrv, GET_SECTOR_SIZE, &sz_sect);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 5;
        }
        printf(" Size of sector is %u bytes.\n", sz_sect);
#else
        sz_sect = FF_MAX_SS;
#endif

        printf("**** Get block size ****\n");
        printf(" disk_ioctl(%p, GET_BLOCK_SIZE, 0x%X)", pdrv, (UINT)&sz_eblk);
//        sz_eblk = 0;
//        dr = disk_ioctl(pdrv, GET_BLOCK_SIZE, &sz_eblk);
//        sz_eblk = get_block_size();
        sz_eblk = 512;
        dr = RES_OK;
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
        }
        if (dr == RES_OK || sz_eblk >= 2) {
            printf(" Size of the erase block is %lu sectors.\n", (unsigned long)sz_eblk);
        } else {
            printf(" Size of the erase block is unknown.\n");
        }

        /* Single sector write test */
        printf("**** Single sector write test ****\n");
        lba = 0;
        for (n = 0, pn(pns); n < sz_sect; n++) pbuff[n] = (BYTE)pn(0);
        printf(" disk_write(%p, 0x%X, %lu, 1)", pdrv, (UINT)pbuff, (unsigned long)lba);
        dr = disk_write(pdrv, pbuff, lba, 1);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 6;
        }
//        printf(" disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
//        dr = disk_ioctl(pdrv, CTRL_SYNC, 0);
//        if (dr == RES_OK) {
//            printf(" - ok.\n");
//        } else {
//            printf(" - failed.\n");
//            return 7;
//        }
        memset(pbuff, 0, sz_sect);
        printf(" disk_read(%p, 0x%X, %lu, 1)", pdrv, (UINT)pbuff, (unsigned long)lba);
        dr = disk_read(pdrv, pbuff, lba, 1);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 8;
        }
        for (n = 0, pn(pns); n < sz_sect && pbuff[n] == (BYTE)pn(0); n++) ;
        if (n == sz_sect) {
            printf(" Read data matched.\n");
        } else {
            printf(" Read data differs from the data written.\n");
            return 10;
        }
        pns++;

        printf("**** Multiple sector write test ****\n");
        lba = 5; ns = sz_buff / sz_sect;
        if (ns > 4) ns = 4;
        if (ns > 1) {
            for (n = 0, pn(pns); n < (UINT)(sz_sect * ns); n++) pbuff[n] = (BYTE)pn(0);
            printf(" disk_write(%p, 0x%X, %lu, %u)", pdrv, (UINT)pbuff, (unsigned long)lba, ns);
            dr = disk_write(pdrv, pbuff, lba, ns);
            if (dr == RES_OK) {
                printf(" - ok.\n");
            } else {
                printf(" - failed.\n");
                return 11;
            }
//            printf(" disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
//            dr = disk_ioctl(pdrv, CTRL_SYNC, 0);
//            if (dr == RES_OK) {
//                printf(" - ok.\n");
//            } else {
//                printf(" - failed.\n");
//                return 12;
//            }
            memset(pbuff, 0, sz_sect * ns);
            printf(" disk_read(%p, 0x%X, %lu, %u)", pdrv, (UINT)pbuff, (unsigned long)lba, ns);
            dr = disk_read(pdrv, pbuff, lba, ns);           
            if (dr == RES_OK) {
                printf(" - ok.\n");
            } else {
                printf(" - failed.\n");
                return 13;
            }
            for (n = 0, pn(pns); n < (UINT)(sz_sect * ns) && pbuff[n] == (BYTE)pn(0); n++) ;
            if (n == (UINT)(sz_sect * ns)) {
                printf(" Read data matched.\n");
            } else {
                printf(" Read data differs from the data written.\n");
                return 14;
            }
        } else {
            printf(" Test skipped.\n");
        }
        pns++;

        printf("**** Single sector write test (unaligned buffer address) ****\n");
        lba = 5;
        for (n = 0, pn(pns); n < sz_sect; n++) pbuff[n+3] = (BYTE)pn(0);
        printf(" disk_write(%p, 0x%X, %lu, 1)", pdrv, (UINT)(pbuff+3), (unsigned long)lba);
        dr = disk_write(pdrv, pbuff+3, lba, 1);       
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 15;
        }
//        printf(" disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
//        dr = disk_ioctl(pdrv, CTRL_SYNC, 0);
//        if (dr == RES_OK) {
//            printf(" - ok.\n");
//        } else {
//            printf(" - failed.\n");
//            return 16;
//        }
        memset(pbuff+5, 0, sz_sect);
        printf(" disk_read(%p, 0x%X, %lu, 1)", pdrv, (UINT)(pbuff+5), (unsigned long)lba);
        dr = disk_read(pdrv, pbuff+5, lba, 1);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 17;
        }
        for (n = 0, pn(pns); n < sz_sect && pbuff[n+5] == (BYTE)pn(0); n++) ;
        if (n == sz_sect) {
            printf(" Read data matched.\n");
        } else {
            printf(" Read data differs from the data written.\n");
            return 18;
        }
        pns++;

        printf("**** 4GB barrier test ****\n");
        if (sz_drv >= 128 + 0x80000000 / (sz_sect / 2)) {
            lba = 6; lba2 = lba + 0x80000000 / (sz_sect / 2);
            for (n = 0, pn(pns); n < (UINT)(sz_sect * 2); n++) pbuff[n] = (BYTE)pn(0);
            printf(" disk_write(%p, 0x%X, %lu, 1)", pdrv, (UINT)pbuff, (unsigned long)lba);
            dr = disk_write(pdrv, pbuff, lba, 1);
            if (dr == RES_OK) {
                printf(" - ok.\n");
            } else {
                printf(" - failed.\n");
                return 19;
            }
            printf(" disk_write(%p, 0x%X, %lu, 1)", pdrv, (UINT)(pbuff+sz_sect), (unsigned long)lba2);
            dr = disk_write(pdrv, pbuff+sz_sect, lba2, 1);
            if (dr == RES_OK) {
                printf(" - ok.\n");
            } else {
                printf(" - failed.\n");
                return 20;
            }
//            printf(" disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
//            dr = disk_ioctl(pdrv, CTRL_SYNC, 0);
//            if (dr == RES_OK) {
//            printf(" - ok.\n");
//            } else {
//                printf(" - failed.\n");
//                return 21;
//            }
            memset(pbuff, 0, sz_sect * 2);
            printf(" disk_read(%p, 0x%X, %lu, 1)", pdrv, (UINT)pbuff, (unsigned long)lba);
            dr = disk_read(pdrv, pbuff, lba, 1);           
            if (dr == RES_OK) {
                printf(" - ok.\n");
            } else {
                printf(" - failed.\n");
                return 22;
            }
            printf(" disk_read(%p, 0x%X, %lu, 1)", pdrv, (UINT)(pbuff+sz_sect), (unsigned long)lba2);
            dr = disk_read(pdrv, pbuff+sz_sect, lba2, 1);
            if (dr == RES_OK) {
                printf(" - ok.\n");
            } else {
                printf(" - failed.\n");
                return 23;
            }
            for (n = 0, pn(pns); pbuff[n] == (BYTE)pn(0) && n < (UINT)(sz_sect * 2); n++) ;
            if (n == (UINT)(sz_sect * 2)) {
                printf(" Read data matched.\n");
            } else {
                printf(" Read data differs from the data written.\n");
                return 24;
            }
        } else {
            printf(" Test skipped.\n");
        }
        pns++;
        
        printf("**** Test cycle %u of %u completed ****\n\n", cc, ncyc);
    }
   
    return 0;
}

//int main (int argc, char* argv[])
int low_level_io_tests(const char *diskName)
{
    int rc;
    // DWORD buff[FF_MAX_SS];  /* Working buffer (4 sector in size) */
    const size_t buff_sz = FF_MAX_SS * sizeof(DWORD);
    
	DWORD* buff = pvPortMalloc(buff_sz);
	if (!buff) {
        printf("%s: Couldn't allocate %zu bytes\n", __FUNCTION__, buff_sz);
		return -1;
	}

    /* Check function/compatibility of the physical drive #0 */
    rc = test_diskio(diskName, 3, buff, buff_sz);

    if (rc) {
        printf("Sorry the function/compatibility test failed. (rc=%d)\nFatFs will not work with this disk driver.\n", rc);
    } else {
        printf("Congratulations! The disk driver works well.\n");
    }

    return rc;
}

