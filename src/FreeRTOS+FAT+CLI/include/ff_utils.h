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
#pragma once

#include <stdbool.h>
#include <stdint.h>
//
#include "FreeRTOS.h"
#include "ff_headers.h"
#include "sd_card.h"

#ifdef __cplusplus
extern "C" {
#endif

bool format(const char *devName);
bool mount(const char *devName);
void unmount(const char *devName);
void eject(const char *name);
void getFree(FF_Disk_t *pxDisk, uint64_t *pFreeMB, unsigned *pFreePct);
FF_Error_t ff_set_fsize( FF_FILE *pxFile ); // Make Filesize equal to the FilePointer
int mkdirhier(char *path);
void ls(const char *path);
sd_card_t *get_current_sd_card_p();

#ifdef __cplusplus
}
#endif
/* [] END OF FILE */
