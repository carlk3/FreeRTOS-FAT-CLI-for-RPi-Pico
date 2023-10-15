/* ff_utils.h
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
