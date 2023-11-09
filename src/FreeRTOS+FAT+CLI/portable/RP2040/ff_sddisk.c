/* ff_sddisk.c
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
 * FreeRTOS+FAT DOS Compatible Embedded FAT File System
 *     https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/index.html
 * ported to Cypress CY8C6347BZI-BLD53.
 *
 * Editor: Carl Kugler (carlk3@gmail.com)
 */
/*
 * FreeRTOS+FAT build 191128 - Note:  FreeRTOS+FAT is still in the lab!
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Authors include James Walmsley, Hein Tibosch and Richard Barry
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * https://www.FreeRTOS.org
 *
 */

#include "ff_sddisk.h"

#include "ff_ioman.h"
#include "hw_config.h"

#define HUNDRED_64_BIT 100ULL
#define SECTOR_SIZE 512UL
#define PARTITION_NUMBER 0 /* Only a single partition is used. */
#define BYTES_PER_KB (1024ull)
#define SECTORS_PER_KB (BYTES_PER_KB / 512ull)

/* A function to write sectors to the device. */
static int32_t
prvWrite(uint8_t *pucSource,      /* Source of data to be written. */
         uint32_t ulSectorNumber, /* The first sector being written to. */
         uint32_t ulSectorCount,  /* The number of sectors to write. */
         FF_Disk_t *pxDisk)       /* Describes the disk being written to. */
{
    int status = ((sd_card_t *)(pxDisk->pvTag))
                     ->write_blocks(pxDisk->pvTag, pucSource, ulSectorNumber,
                                    ulSectorCount);
    if (SD_BLOCK_DEVICE_ERROR_NONE == status) {
        return FF_ERR_NONE;
    } else {
        return FF_ERR_IOMAN_DRIVER_FATAL_ERROR | FF_ERRFLAG;
    }
}

/* A function to read sectors from the device. */
static int32_t
prvRead(uint8_t *pucDestination, /* Destination for data being read. */
        uint32_t ulSectorNumber, /* Sector from which to start reading data. */
        uint32_t ulSectorCount,  /* Number of sectors to read. */
        FF_Disk_t *pxDisk)       /* Describes the disk being read from. */
{
    int status = ((sd_card_t *)(pxDisk->pvTag))
                     ->read_blocks(pxDisk->pvTag, pucDestination, ulSectorNumber,
                                   ulSectorCount);
    if (SD_BLOCK_DEVICE_ERROR_NONE == status) {
        return FF_ERR_NONE;
    } else {
        return FF_ERR_IOMAN_DRIVER_FATAL_ERROR | FF_ERRFLAG;
    }
}

BaseType_t FF_SDDiskDetect(FF_Disk_t *pxDisk) {
    if (!pxDisk)
        return false;
    return sd_card_detect(pxDisk->pvTag);
}

static bool disk_init(sd_card_t *sd_card_p) {
    FF_Error_t xError = 0;
    FF_CreationParameters_t xParameters;
    const uint32_t xIOManagerCacheSize = 4 * SECTOR_SIZE;

    if (sd_card_p->ff_disk.xStatus.bIsInitialised == pdTRUE) {
        // Already initialized
        return true;
    }

    /* Check the validity of the xIOManagerCacheSize parameter. */
    configASSERT((xIOManagerCacheSize % SECTOR_SIZE) == 0);
    configASSERT((xIOManagerCacheSize >= (2 * SECTOR_SIZE)));

    // Initialize the media driver
    bool rc = sd_init_driver();
    if (!rc)
        return rc;

    //	STA_NOINIT = 0x01, /* Drive not initialized */
    //	STA_NODISK = 0x02, /* No medium in the drive */
    //	STA_PROTECT = 0x04 /* Write protected */
    int ds = sd_card_p->init(sd_card_p);
    if (STA_NODISK & ds || STA_NOINIT & ds)
        return false;

    /* The pvTag member of the FF_Disk_t structure allows the structure to be
            extended to also include media specific parameters. */
    sd_card_p->ff_disk.pvTag = sd_card_p;

    /* The number of sectors is recorded for bounds checking in the read and
     write functions. */
    sd_card_p->ff_disk.ulNumberOfSectors = sd_card_p->sectors;

    /* Create the IO manager that will be used to control the disk –
     the FF_CreationParameters_t structure completed with the required
     parameters, then passed into the FF_CreateIOManager() function. */
    memset(&xParameters, 0, sizeof xParameters);
    xParameters.pucCacheMemory = NULL;
    xParameters.ulMemorySize = xIOManagerCacheSize;
    xParameters.ulSectorSize = SECTOR_SIZE;
    xParameters.fnWriteBlocks = prvWrite;
    xParameters.fnReadBlocks = prvRead;
    xParameters.pxDisk = &sd_card_p->ff_disk;
    xParameters.pvSemaphore = (void *)xSemaphoreCreateRecursiveMutex();
    xParameters.xBlockDeviceIsReentrant = pdTRUE;
    sd_card_p->ff_disk.pxIOManager = FF_CreateIOManger(&xParameters, &xError);

    if ((sd_card_p->ff_disk.pxIOManager != NULL) &&
        (FF_isERR(xError) == pdFALSE)) {
        /* Record that the disk has been initialised. */
        sd_card_p->ff_disk.xStatus.bIsInitialised = pdTRUE;
    } else {
        /* The disk structure was allocated, but the disk’s IO manager could
         not be allocated, so free the disk again. */
        FF_SDDiskDelete(&sd_card_p->ff_disk);
        FF_PRINTF("FF_SDDiskInit: FF_CreateIOManger: %s\n",
                  (const char *)FF_GetErrMessage(xError));
        configASSERT(!"disk's IO manager could not be allocated!");
        sd_card_p->ff_disk.xStatus.bIsInitialised = pdFALSE;
    }
    return true;
}

// Doesn't do an automatic mount, since card might need to be formatted first.
// State after return is disk is initialized, but not mounted.
FF_Disk_t *FF_SDDiskInit(const char *pcName) {
    sd_card_t *sd_card_p = sd_get_by_name(pcName);
    if (!sd_card_p) {
        FF_PRINTF("FF_SDDiskInit: unknown name %s\n", pcName);
        return NULL;
    }
    if (disk_init(sd_card_p))
        return &sd_card_p->ff_disk;
    else
        return NULL;
}

BaseType_t FF_SDDiskReinit(FF_Disk_t *pxDisk) {
    return disk_init(pxDisk->pvTag) ? pdPASS : pdFAIL;
}

/* Unmount the volume */
// FF_SDDiskUnmount() calls FF_Unmount().
BaseType_t FF_SDDiskUnmount(FF_Disk_t *pDisk) {
    if (!pDisk->xStatus.bIsMounted)
        return FF_ERR_NONE;
    FF_Error_t e = FF_Unmount(pDisk);
    if (FF_ERR_NONE != e) {
        FF_PRINTF("FF_Unmount error: %s\n", FF_GetErrMessage(e));
    } else {
        pDisk->xStatus.bIsMounted = pdFALSE;
    }
    return e;
}

/* Mount the volume */
// FF_SDDiskMount() calls FF_Mount().
BaseType_t FF_SDDiskMount(FF_Disk_t *pDisk) {
    if (pDisk->xStatus.bIsMounted)
        return FF_ERR_NONE;
    // FF_Error_t FF_Mount( FF_Disk_t *pxDisk, BaseType_t xPartitionNumber );
    FF_Error_t e = FF_Mount(pDisk, PARTITION_NUMBER);
    if (FF_ERR_NONE != e) {
        FF_PRINTF("FF_Mount error: %s\n", FF_GetErrMessage(e));
    } else {
        pDisk->xStatus.bIsMounted = pdTRUE;
    }
    return e;
}

BaseType_t FF_SDDiskDelete(FF_Disk_t *pxDisk) {
    if (pxDisk) {
        if (pxDisk->pvTag) {
            ((sd_card_t *)(pxDisk->pvTag))->init(pxDisk->pvTag);
        }
        if (pxDisk->xStatus.bIsInitialised) {
            if (pxDisk->pxIOManager) {
                FF_DeleteIOManager(pxDisk->pxIOManager);
            }
            pxDisk->ulSignature = 0;
            pxDisk->xStatus.bIsInitialised = pdFALSE;
        }
        sd_card_t *sd_card_p = pxDisk->pvTag;
        configASSERT(sd_card_p);
    }
    return pdPASS;
}

/* Show some partition information */
BaseType_t FF_SDDiskShowPartition(FF_Disk_t *pxDisk) {
    FF_Error_t xError;
    uint64_t ullFreeSectors;
    uint32_t ulTotalSizeKB, ulFreeSizeKB;
    int iPercentageFree;
    FF_IOManager_t *pxIOManager;
    const char *pcTypeName = "unknown type";
    BaseType_t xReturn = pdPASS;

    if (pxDisk == NULL) {
        xReturn = pdFAIL;
    } else {
        pxIOManager = pxDisk->pxIOManager;

        FF_PRINTF("Reading FAT and calculating Free Space\n");

        switch (pxIOManager->xPartition.ucType) {
            case FF_T_FAT12:
                pcTypeName = "FAT12";
                break;

            case FF_T_FAT16:
                pcTypeName = "FAT16";
                break;

            case FF_T_FAT32:
                pcTypeName = "FAT32";
                break;

            default:
                pcTypeName = "UNKOWN";
                break;
        }

        FF_GetFreeSize(pxIOManager, &xError);

        ullFreeSectors = pxIOManager->xPartition.ulFreeClusterCount *
                         pxIOManager->xPartition.ulSectorsPerCluster;
        if (pxIOManager->xPartition.ulDataSectors == (uint32_t)0) {
            iPercentageFree = 0;
        } else {
            iPercentageFree =
                (int)((HUNDRED_64_BIT * ullFreeSectors +
                       pxIOManager->xPartition.ulDataSectors / 2) /
                      ((uint64_t)pxIOManager->xPartition.ulDataSectors));
        }

        ulTotalSizeKB = pxIOManager->xPartition.ulDataSectors / SECTORS_PER_KB;
        ulFreeSizeKB = (uint32_t)(ullFreeSectors / SECTORS_PER_KB);

        /* It is better not to use the 64-bit format such as %Lu because it
         might not be implemented. */
        FF_PRINTF("Partition Nr   %8u\n", pxDisk->xStatus.bPartitionNumber);
        FF_PRINTF("Type           %8u (%s)\n", pxIOManager->xPartition.ucType,
                  pcTypeName);
        FF_PRINTF("VolLabel       '%8s' \n", pxIOManager->xPartition.pcVolumeLabel);
        FF_PRINTF("TotalSectors   %8lu\n",
                  (unsigned long)pxIOManager->xPartition.ulTotalSectors);
        FF_PRINTF("SecsPerCluster %8lu\n",
                  (unsigned long)pxIOManager->xPartition.ulSectorsPerCluster);
        FF_PRINTF("Size           %8lu KB\n", (unsigned long)ulTotalSizeKB);
        FF_PRINTF("FreeSize       %8lu KB ( %d perc free )\n",
                  (unsigned long)ulFreeSizeKB, iPercentageFree);
    }

    return xReturn;
}

/* Flush changes from the driver's buf to disk */
void FF_SDDiskFlush(FF_Disk_t *pDisk) { FF_FlushCache(pDisk->pxIOManager); }

/* Format a given partition on an SD-card. */
// FF_SDDiskFormat() calls FF_Format() + FF_SDDiskMount().
BaseType_t FF_SDDiskFormat(FF_Disk_t *pxDisk, BaseType_t aPart) {
    // FF_Error_t FF_Format( FF_Disk_t *pxDisk, BaseType_t xPartitionNumber,
    // BaseType_t xPreferFAT16, BaseType_t xSmallClusters );
    FF_Error_t e = FF_Format(pxDisk, aPart, pdFALSE, pdFALSE);
    if (FF_ERR_NONE != e) {
        FF_PRINTF("FF_Format error:%s\n", FF_GetErrMessage(e));
        return e;
    }
    return FF_SDDiskMount(pxDisk);
}

/* Return non-zero if an SD-card is detected in a given slot. */
BaseType_t FF_SDDiskInserted(BaseType_t xDriveNr) {
    sd_card_t *sd_card_p = sd_get_by_num(xDriveNr);
    if (!sd_card_p)
        return false;
    return sd_card_detect(sd_card_p);
}

/*-----------------------------------------------------------*/
