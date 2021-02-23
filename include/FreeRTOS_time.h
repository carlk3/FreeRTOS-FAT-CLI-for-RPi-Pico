#pragma once

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

extern time_t epochtime;

void FreeRTOS_time_init();

#ifdef __cplusplus
}
#endif
