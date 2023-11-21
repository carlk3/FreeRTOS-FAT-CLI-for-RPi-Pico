#pragma once
#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    PRIORITY_stdioTask = configMAX_PRIORITIES - 2
};

enum {
	NOTIFICATION_IX_reserved,
    NOTIFICATION_IX_STDIO,
	NOTIFICATION_IX_SD_SPI,
    NOTIFICATION_IX_UNMOUNT  
};

#ifdef __cplusplus
}
#endif
