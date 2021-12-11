/* stdio_cli.h
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
