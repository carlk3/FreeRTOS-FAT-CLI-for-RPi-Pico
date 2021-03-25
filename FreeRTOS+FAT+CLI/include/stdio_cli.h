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

/* Dimensions the buffer into which input characters are placed. */
#define cmdMAX_INPUT_SIZE	60

/* Dimensions the buffer into which input characters are placed. */
#define cmdMAX_INPUT_SIZE	60

/* Dimensions the buffer into which string outputs can be placed. */
#define cmdMAX_OUTPUT_SIZE	1024

/* DEL acts as a backspace. */
static char const  cmdASCII_DEL = 0x7F;

/* Const messages output by the command console. */
static const char * const pcEndOfOutputMessage = "\n> ";

#ifdef __cplusplus
extern "C" {
#endif

void CLI_Start();

#ifdef __cplusplus
}
#endif

/* [] END OF FILE */
