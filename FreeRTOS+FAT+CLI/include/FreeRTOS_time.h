#pragma once

#include <time.h>

#include "pico/util/datetime.h"

#ifdef __cplusplus
extern "C" {
#endif

extern time_t epochtime;

void FreeRTOS_time_init();
void setrtc(datetime_t *t);

#ifdef __cplusplus
}
#endif
