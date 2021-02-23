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

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

//#include "pico/stdio.h"

void mark_start_time();
time_t GLOBAL_uptime_seconds();

// Debug printf
// extern void my_printf(const char *pcFormat, ...) __attribute__ ((format
// (__printf__, 1, 2))); #if defined(DEBUG) && !defined(NDEBUG) #   define
//DBG_PRINTF vLoggingPrintf
#define DBG_PRINTF printf
//#else
//#   define DBG_PRINTF(fmt, args...)    /* Don't do anything in release builds
//*/ #endif

#define time_fn(arg)                                              \
    {                                                             \
        TickType_t xStart = xTaskGetTickCount();                  \
        arg;                                                      \
        FF_PRINTF("%s: Elapsed time: %lu s\n", #arg,              \
                  (unsigned long)(xTaskGetTickCount() - xStart) / \
                      configTICK_RATE_HZ);                        \
    }

// See FreeRTOSConfig.h
void my_assert_func(const char *file, int line, const char *func,
                    const char *pred);

void assert_always_func(const char *file, int line, const char *func,
                        const char *pred);
#define ASSERT_ALWAYS(__e) \
    ((__e) ? (void)0 : assert_always_func(__FILE__, __LINE__, __func__, #__e))

void assert_case_not_func(const char *file, int line, const char *func, int v);
#define ASSERT_CASE_NOT(__v) \
    (assert_case_not_func(__FILE__, __LINE__, __func__, __v))

#ifdef NDEBUG /* required by ANSI standard */
#define DBG_ASSERT_CASE_NOT(__e) ((void)0)
#else
#define DBG_ASSERT_CASE_NOT(__v) \
    (assert_case_not_func(__FILE__, __LINE__, __func__, __v))
#endif

void task_printf(const char *pcFormat, ...);

int stdio_fail(const char *const fn, const char *const str);
#define FAIL(str) stdio_fail(__FUNCTION__, str)

int ff_stdio_fail(const char *const func, char const *const str,
                  char const *const filename);
#define FF_FAIL(str, filname) ff_stdio_fail(__FUNCTION__, str, filename)

bool DBG_Connected();

/* [] END OF FILE */
