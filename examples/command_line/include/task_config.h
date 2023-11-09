#pragma once
#include "FreeRTOS.h"


enum {
    PRIORITY_stdioTask = configMAX_PRIORITIES - 2
};

enum {
	NOTIFICATION_IX_none,
    NOTIFICATION_IX_STDIO,
	NOTIFICATION_IX_SD_SPI    
};