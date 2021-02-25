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

#include <stdarg.h>
#include <stdint.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "hardware/timer.h"

// For pcTaskGetName:
#include "FreeRTOS.h"
#include "ff_stdio.h"
#include "task.h"

//
#include "cmsis_gcc.h"
#include "core_cm0plus.h"

//
#include "crash.h"

static time_t start_time;

void mark_start_time() {
	start_time = FreeRTOS_time(NULL);
}
time_t GLOBAL_uptime_seconds() {
    return FreeRTOS_time(NULL) - start_time;
}

bool DBG_Connected() {
	return false;
}

void task_printf( const char *pcFormat, ... )
{
	char pcBuffer[256] = {0};
	va_list xArgs;
	va_start( xArgs, pcFormat );
	vsnprintf( pcBuffer, sizeof(pcBuffer), pcFormat, xArgs );
	va_end( xArgs );
	printf("%s: %s", pcTaskGetName(NULL), pcBuffer);
	fflush(stdout);
}

int stdio_fail(const char * const fn, const char * const str) {
	int error = stdioGET_ERRNO();
	FF_PRINTF("%s: %s: %s: %s (%d)\n", pcTaskGetName(NULL), fn, str, strerror(error), error);
	return error;
}

int ff_stdio_fail(const char * const func, char const * const str, char const * const filename) {
	int error = stdioGET_ERRNO();
	FF_PRINTF("%s: %s: %s(%s): %s (%d)\n", pcTaskGetName(NULL), func, str, filename, strerror(error), error);
	return error;
}

#if 0
    void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName ) {
	/* The stack space has been exceeded for a task, considering allocating more. */
	printf("\nOut of stack space!\n");
	printf(pcTaskGetName(NULL));
	printf("\n");
	log_printf("\nOut of stack space! Task: %p %s\n", xTask, pcTaskName);
	__disable_irq(); /* Disable global interrupts. */
	vTaskSuspendAll();
//	while(1) { __asm("bkpt #0"); }; // Stop in GUI as if at a breakpoint (if debugging, otherwise loop forever)    
	while(1);
}
#endif

#ifndef NDEBUG 
// Note: All pvPortMallocs should be checked individually,
// but we don't expect any to fail, 
// so this can help flag problems in Debug builds.
void vApplicationMallocFailedHook( void ) {
	printf("\nMalloc failed!\n");    
	printf("\nMalloc failed! Task: %s\n", pcTaskGetName(NULL));
	__disable_irq(); /* Disable global interrupts. */
	vTaskSuspendAll();
    __BKPT(5);
}
#endif

#if 0 // the implementation in cy_syslib.c might be useful
/*******************************************************************************
* Function Name: Cy_SysLib_ProcessingFault
********************************************************************************
* Summary:
*  This function prints out the stacked register at the moment the hard fault 
*  occurred.
*  cy_syslib.c include same function as __WEAK option, this function will  
*  replace the weak function. Cy_SysLib_ProcessingFault() is called at the end
*  of Cy_SysLib_FaultHandler() function which is the default exception handler 
*  set for hard faults.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void Cy_SysLib_ProcessingFault(void)
{
	
	printf("\r\nCM4 FAULT!!\r\n");
	printf("SCB->CFSR = 0x%08lx\r\n", (unsigned long)cy_faultFrame.cfsr.cfsrReg);
	
	/* If MemManage fault valid bit is set to 1, print MemManage fault address */
	if((cy_faultFrame.cfsr.cfsrReg & SCB_CFSR_MMARVALID_Msk) == SCB_CFSR_MMARVALID_Msk )
	{
		printf("MemManage Fault! Fault address = 0x%08lx\r\n", (unsigned long)SCB->MMFAR);
	}
	/* If Bus Fault valid bit is set to 1, print BusFault Address */
	if((cy_faultFrame.cfsr.cfsrReg & SCB_CFSR_BFARVALID_Msk) == SCB_CFSR_BFARVALID_Msk )
	{
		printf("Bus Fault! \r\nFault address = 0x%08lx\r\n", (unsigned long)SCB->BFAR);
	}
	/* Print Fault Frame */
	printf("r0 = 0x%08lx\r\n", (unsigned long)cy_faultFrame.r0);
	printf("r1 = 0x%08lx\r\n", (unsigned long)cy_faultFrame.r1);
	printf("r2 = 0x%08lx\r\n", (unsigned long)cy_faultFrame.r2);
	printf("r3 = 0x%08lx\r\n", (unsigned long)cy_faultFrame.r3);
	printf("r12 = 0x%08lx\r\n", (unsigned long)cy_faultFrame.r12);
	printf("lr = 0x%08lx\r\n", (unsigned long)cy_faultFrame.lr);
	printf("pc = 0x%08lx\r\n", (unsigned long)cy_faultFrame.pc);
	printf("psr = 0x%08lx\r\n", (unsigned long)cy_faultFrame.psr);

	__disable_irq(); /* Disable global interrupts. */
	vTaskSuspendAll();
//	while(1) { __asm("bkpt #0"); }; // Stop in GUI as if at a breakpoint (if debugging, otherwise loop forever)    
	while(1);
}
#endif

/*******************************************************************************
* Function Name: configure_fault_register
********************************************************************************
* Summary:
*  This function configures the fault registers(bus fault and usage fault). See
*  the Arm documentation for more details on the registers.
*
*******************************************************************************/
void configure_fault_register(void)
{
	/* Set SCB->SHCSR.BUSFAULTENA so that BusFault handler instead of the
	 * HardFault handler handles the BusFault.
	 */
//	SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk;

#ifdef DEBUG_FAULT    
	/* If ACTLR.DISDEFWBUF is not set to 1, the imprecise BusFault will occur.
	 * For the imprecise BusFault, the fault stack information won't be accurate.
	 * Setting ACTLR.DISDEFWBUF bit to 1 so that bus faults will be precise.
	 * Refer Arm documentation for detailed explanation on precise and imprecise
	 * BusFault.
	 * WARNING: This will decrease the performance because any store to memory
	 * must complete before the processor can execute the next instruction.
	 * Don't enable always, if it is not necessary.
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

void my_assert_func (const char *file, int line, const char *func, const char *pred) {
//    TRIG(0);
    
    //FIXME: Send error message to GUI?
    
	printf("assertion \"%s\" failed: file \"%s\", line %d, function: %s\n", pred, file, line, func);
	vTaskSuspendAll();
	__disable_irq(); /* Disable global interrupts. */
    if (DBG_Connected()){ 
		__BKPT(3);  // Stop in GUI as if at a breakpoint (if debugging, otherwise loop forever)  
	} else {
        capture_assert(file, line, func, pred);
    }    
}

void assert_always_func(const char *file, int line, const char *func, const char *pred) {
	printf("assertion \"%s\" failed: file \"%s\", line %d, function: %s\n", pred, file, line, func);
	vTaskSuspendAll();
	__disable_irq(); /* Disable global interrupts. */
    if (DBG_Connected()){ 
		__BKPT(3);  // Stop in GUI as if at a breakpoint (if debugging, otherwise loop forever)  
	} else {
        capture_assert(file, line, func, pred);
    }    
}

void assert_case_not_func(const char *file, int line, const char *func, int v) {
        char pred[128];
        snprintf(pred, sizeof pred, "case not %d", v);
        assert_always_func(file, line, func, pred);
}

/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
									StackType_t **ppxIdleTaskStackBuffer,
									uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static – otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ] __attribute__ ((aligned));

	/* Pass out a pointer to the StaticTask_t structure in which the Idle task’s
	state will be stored. */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task’s stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*———————————————————–*/

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
									 StackType_t **ppxTimerTaskStackBuffer,
									 uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static – otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ] __attribute__ ((aligned));

	/* Pass out a pointer to the StaticTask_t structure in which the Timer
	task’s state will be stored. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

	/* Pass out the array that will be used as the Timer task’s stack. */
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;

	/* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

/* [] END OF FILE */
