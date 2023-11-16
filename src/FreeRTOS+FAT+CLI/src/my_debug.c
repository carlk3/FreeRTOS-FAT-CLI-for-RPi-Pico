/* my_debug.c
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
#include <stdarg.h>
#include <stdint.h>
//
#include "hardware/timer.h"
#include "pico/multicore.h"  // get_core_num()
#include "pico/stdio.h"
#include "pico/stdlib.h"
//
#include "FreeRTOS.h"
#include "semphr.h"
#include "ff_stdio.h"
//
#include <RP2040.h>
//
#include "crash.h"
#include "FreeRTOS_time.h"
#include "my_debug.h"

static time_t start_time;

void mark_start_time() { start_time = FreeRTOS_time(NULL); }
time_t GLOBAL_uptime_seconds() { return FreeRTOS_time(NULL) - start_time; }

/* Function Attribute ((weak))
The weak attribute causes a declaration of an external symbol to be emitted as a weak symbol rather than a global.
This is primarily useful in defining library functions that can be overridden in user code, though it can also be used with non-function declarations.
The overriding symbol must have the same type as the weak symbol.
https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html

You can override these functions in your application to redirect "stdout"-type messages.
*/

/* Single string output callbacks */

void __attribute__((weak)) put_out_error_message(const char *s) {
    (void)s;
}
void __attribute__((weak)) put_out_info_message(const char *s) {
    (void)s;
}
void __attribute__((weak)) put_out_debug_message(const char *s) {
    (void)s;
}

/* "printf"-style output callbacks */

#if defined(USE_PRINTF) && USE_PRINTF

int __attribute__((weak)) error_message_printf(const char *func, int line, const char *fmt, ...) {
    printf("%s:%d: ", func, line);
    va_list args;
    va_start(args, fmt);
    int cw = vprintf(fmt, args);
    va_end(args);
    // stdio_flush();
    fflush(stdout);
    return cw;
}
int __attribute__((weak)) error_message_printf_plain(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int cw = vprintf(fmt, args);
    va_end(args);
    // stdio_flush();
    fflush(stdout);
    return cw;
}
int __attribute__((weak)) info_message_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int cw = vprintf(fmt, args);
    va_end(args);
    return cw;
}
int __attribute__((weak)) debug_message_printf(const char *func, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int cw = vprintf(fmt, args);
    va_end(args);
    // stdio_flush();
    fflush(stdout);
    return cw;
}

#else

/* These will truncate at 256 bytes. You can tell by checking the return code. */

int __attribute__((weak)) error_message_printf(const char *func, int line, const char *fmt, ...) {
    char buf[256] = {0};
    va_list args;
    va_start(args, fmt);
    int cw = vsnprintf(buf, sizeof(buf), fmt, args);
    put_out_error_message(buf);
    va_end(args);
    return cw;
}
int __attribute__((weak)) error_message_printf_plain(const char *fmt, ...) {
    char buf[256] = {0};
    va_list args;
    va_start(args, fmt);
    int cw = vsnprintf(buf, sizeof(buf), fmt, args);
    put_out_info_message(buf);
    va_end(args);
    return cw;
}
int __attribute__((weak)) info_message_printf(const char *fmt, ...) {
    char buf[256] = {0};
    va_list args;
    va_start(args, fmt);
    int cw = vsnprintf(buf, sizeof(buf), fmt, args);
    put_out_info_message(buf);
    va_end(args);
    return cw;
}
int __attribute__((weak)) debug_message_printf(const char *func, int line, const char *fmt, ...) {
    char buf[256] = {0};
    va_list args;
    va_start(args, fmt);
    int cw = vsnprintf(buf, sizeof(buf), fmt, args);
    put_out_debug_message(buf);
    va_end(args);
    return cw;
}

#endif

static SemaphoreHandle_t xSemaphore;
static BaseType_t printf_locked;
static void lock_printf() {
    static StaticSemaphore_t xMutexBuffer;    
    static bool initialized;
    if (!initialized) {
        xSemaphore = xSemaphoreCreateMutexStatic(&xMutexBuffer);
        initialized = true;
    }
    configASSERT(xSemaphore);
    printf_locked = xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(1000));
}
static void unlock_printf() {
    if (pdTRUE == printf_locked) xSemaphoreGive(xSemaphore);
}

void task_printf(const char *pcFormat, ...) {
    char pcBuffer[256] = {0};
    va_list xArgs;
    va_start(xArgs, pcFormat);
    vsnprintf(pcBuffer, sizeof(pcBuffer), pcFormat, xArgs);
    va_end(xArgs);
    // lock_printf();
    printf("core%u: %s: %s", get_core_num(), pcTaskGetName(NULL), pcBuffer);
    fflush(stdout);
    // unlock_printf();
}

int stdio_fail(const char *const fn, const char *const str) {
    int error = stdioGET_ERRNO();
    FF_PRINTF("%s: %s: %s: %s (%d)\n", pcTaskGetName(NULL), fn, str,
              strerror(error), error);
    return error;
}

int ff_stdio_fail(const char *const func, char const *const str,
                  char const *const filename) {
    int error = stdioGET_ERRNO();
    FF_PRINTF("%s: %s: %s(%s): %s (%d)\n", pcTaskGetName(NULL), func, str,
              filename, strerror(error), error);
    return error;
}

/*******************************************************************************
 * Function Name: configure_fault_register
 ********************************************************************************
 * Summary:
 *  This function configures the fault registers(bus fault and usage fault). See
 *  the Arm documentation for more details on the registers.
 *
 *******************************************************************************/
void configure_fault_register(void) {
    /* Set SCB->SHCSR.BUSFAULTENA so that BusFault handler instead of the
     * HardFault handler handles the BusFault.
     */
    //	SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk;

#ifdef DEBUG_FAULT
    /* If ACTLR.DISDEFWBUF is not set to 1, the imprecise BusFault will occur.
     * For the imprecise BusFault, the fault stack information won't be
     * accurate. Setting ACTLR.DISDEFWBUF bit to 1 so that bus faults will be
     * precise. Refer Arm documentation for detailed explanation on precise and
     * imprecise BusFault. WARNING: This will decrease the performance because
     * any store to memory must complete before the processor can execute the
     * next instruction. Don't enable always, if it is not necessary.
     */
    SCnSCB->ACTLR |= SCnSCB_ACTLR_DISDEFWBUF_Msk;

    /* Enable UsageFault when processor executes an divide by 0 */
    SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;

    /* Set SCB->SHCSR.USGFAULTENA so that faults such as DIVBYZERO, UNALIGNED,
     * UNDEFINSTR etc are handled by UsageFault handler instead of the HardFault
     * handler.
     */
    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk;
#endif
}

void my_assert_func(const char *file, int line, const char *func,
                    const char *pred) {
    TRIG();  // DEBUG
    // task_printf(
    error_message_printf_plain(
        "%s: assertion \"%s\" failed: file \"%s\", line %d, function: %s\n",
        pcTaskGetName(NULL), pred, file, line, func);
    fflush(stdout);
    vTaskSuspendAll();
    __disable_irq(); /* Disable global interrupts. */
    __BKPT(3);       // Stop in GUI as if at a breakpoint (if debugging,
                     // otherwise loop forever)
}

void assert_always_func(const char *file, int line, const char *func,
                        const char *pred) {
    TRIG();  // DEBUG
    printf("assertion \"%s\" failed: file \"%s\", line %d, function: %s\n",
           pred, file, line, func);
    vTaskSuspendAll();
    __disable_irq(); /* Disable global interrupts. */
    __BKPT(3);       // Stop in GUI as if at a breakpoint (if debugging,
                     // otherwise loop forever)
}

void assert_case_not_func(const char *file, int line, const char *func, int v) {
    TRIG();  // DEBUG
    char pred[128];
    snprintf(pred, sizeof pred, "case not %d", v);
    assert_always_func(file, line, func, pred);
}

void assert_case_is(const char *file, int line, const char *func, int v,
                    int expected) {
    TRIG();  // DEBUG
    char pred[128];
    snprintf(pred, sizeof pred, "%d is %d", v, expected);
    assert_always_func(file, line, func, pred);
}

void dump8buf(char *buf, size_t buf_sz, uint8_t *pbytes, size_t nbytes) {
    int n = 0;
    for (size_t byte_ix = 0; byte_ix < nbytes; ++byte_ix) {
        for (size_t col = 0; col < 32 && byte_ix < nbytes; ++col, ++byte_ix) {
            n += snprintf(buf + n, buf_sz - n, "%02hhx ", pbytes[byte_ix]);
            configASSERT(0 < n && n < (int)buf_sz);
        }
        n += snprintf(buf + n, buf_sz - n, "\n");
        configASSERT(0 < n && n < (int)buf_sz);
    }
}
void hexdump_8(const char *s, const uint8_t *pbytes, size_t nbytes) {
    lock_printf();
    printf("\n%s: %s(%s, 0x%p, %zu)\n", pcTaskGetName(NULL), __FUNCTION__, s,
           pbytes, nbytes);
    fflush(stdout);
    size_t col = 0;
    for (size_t byte_ix = 0; byte_ix < nbytes; ++byte_ix) {
        printf("%02hhx ", pbytes[byte_ix]);
        if (++col > 31) {
            printf("\n");
            col = 0;
        }
        fflush(stdout);
    }
    unlock_printf();
}
// nwords is size in WORDS!
void hexdump_32(const char *s, const uint32_t *pwords, size_t nwords) {
    lock_printf();
    printf("\n%s: %s(%s, 0x%p, %zu)\n", pcTaskGetName(NULL), __FUNCTION__, s,
           pwords, nwords);
    fflush(stdout);
    size_t col = 0;
    for (size_t word_ix = 0; word_ix < nwords; ++word_ix) {
        printf("%08lx ", pwords[word_ix]);
        if (++col > 7) {
            printf("\n");
            col = 0;
        }
        fflush(stdout);
    }
    unlock_printf();
}
// nwords is size in bytes
bool compare_buffers_8(const char *s0, const uint8_t *pbytes0, const char *s1,
                       const uint8_t *pbytes1, const size_t nbytes) {
    /* Verify the data. */
    if (0 != memcmp(pbytes0, pbytes1, nbytes)) {
        hexdump_8(s0, pbytes0, nbytes);
        hexdump_8(s1, pbytes1, nbytes);
        return false;
    }
    return true;
}
// nwords is size in WORDS!
bool compare_buffers_32(const char *s0, const uint32_t *pwords0, const char *s1,
                        const uint32_t *pwords1, const size_t nwords) {
    /* Verify the data. */
    if (0 != memcmp(pwords0, pwords1, nwords * sizeof(uint32_t))) {
        hexdump_32(s0, pwords0, nwords);
        hexdump_32(s1, pwords1, nwords);
        return false;
    }
    return true;
}
/* [] END OF FILE */
