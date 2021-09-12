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

#ifndef _FS_UTILS_H
#define _FS_UTILS_H   

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "ff_headers.h"

bool format(FF_Disk_t **ppxDisk, const char *const devName);
bool mount(FF_Disk_t **ppxDisk, const char *const devName, const char *const path);
void unmount(FF_Disk_t *pxDisk, const char *pcPath);
void eject(const char *const name, const char *pcPath);
void getFree(FF_Disk_t *pxDisk, uint64_t *pFreeMB, unsigned *pFreePct);
FF_Error_t ff_set_fsize( FF_FILE *pxFile ); // Make Filesize equal to the FilePointer
int mkdirhier(char *path);

#endif
/* [] END OF FILE */
