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

#ifdef ANALYZER
#  define TRIG() gpio_put(15, 1)  // DEBUG
#else 
#  define TRIG()
#endif

extern void vLoggingPrintf(const char *pcFormat, ...)
    __attribute__((format(__printf__, 1, 2)));

//#define DBG_PRINTF task_printf 
#define DBG_PRINTF printf

void task_printf(const char *pcFormat, ...) __attribute__((format(__printf__, 1, 2)));

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

void assert_case_is(const char *file, int line, const char *func, int v,
                    int expected);
#define ASSERT_CASE_IS(__v, __e) \
    ((__v == __e) ? (void)0 : assert_case_is(__FILE__, __LINE__, __func__, __v, __e))

void assert_case_not_func(const char *file, int line, const char *func, int v);
#define ASSERT_CASE_NOT(__v) \
    (assert_case_not_func(__FILE__, __LINE__, __func__, __v))

#ifdef NDEBUG /* required by ANSI standard */
#define DBG_ASSERT_CASE_NOT(__e) ((void)0)
#else
#define DBG_ASSERT_CASE_NOT(__v) \
    (assert_case_not_func(__FILE__, __LINE__, __func__, __v))
#endif

int stdio_fail(const char *const fn, const char *const str);
#define FAIL(str) stdio_fail(__FUNCTION__, str)

int ff_stdio_fail(const char *const func, char const *const str,
                  char const *const filename);
#define FF_FAIL(str, filname) ff_stdio_fail(__FUNCTION__, str, filename)

void dump8buf(char *buf, size_t buf_sz, uint8_t *pbytes, size_t nbytes);
void hexdump_8(const char *s, const uint8_t *pbytes, size_t nbytes);
bool compare_buffers_8(const char *s0, const uint8_t *pbytes0, const char *s1,
                       const uint8_t *pbytes1, const size_t nbytes);

// sz is size in BYTES!
void hexdump_32(const char *s, const uint32_t *pwords, size_t nwords);
// sz is size in BYTES!
bool compare_buffers_32(const char *s0, const uint32_t *pwords0, const char *s1,
                        const uint32_t *pwords1, const size_t nwords);

bool DBG_Connected();

/* [] END OF FILE */
