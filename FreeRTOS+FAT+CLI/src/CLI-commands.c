/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/* Standard includes. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>  // min and max
#include <time.h>

#include <malloc.h> // mallinfo2
//
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "ff_time.h"
#include "task.h"
#include "FreeRTOS_time.h"

/* FreeRTOS+CLI includes. */
#include "CLI-commands.h"
#include "FreeRTOS_CLI.h"
//
#include "crash.h"
#include "hw_config.h"
#include "sd_card.h"
#include "ff_sddisk.h"
#include "stdio_cli.h"

#ifndef configINCLUDE_TRACE_RELATED_CLI_COMMANDS
#define configINCLUDE_TRACE_RELATED_CLI_COMMANDS 0
#endif

/*
 * Implements the run-time-stats command.
 */
static BaseType_t prvTaskStatsCommand(char *pcWriteBuffer,
                                      size_t xWriteBufferLen,
                                      const char *pcCommandString);

static BaseType_t prvHeapStatsCommand(char *pcWriteBuffer,
                                      size_t xWriteBufferLen,
                                      const char *pcCommandString);

/*
 * Implements the task-stats command.
 */
static BaseType_t prvRunTimeStatsCommand(char *pcWriteBuffer,
                                         size_t xWriteBufferLen,
                                         const char *pcCommandString);

/*
 * Implements the echo-three-parameters command.
 */
static BaseType_t prvThreeParameterEchoCommand(char *pcWriteBuffer,
                                               size_t xWriteBufferLen,
                                               const char *pcCommandString);

/*
 * Implements the echo-parameters command.
 */
static BaseType_t prvParameterEchoCommand(char *pcWriteBuffer,
                                          BaseType_t xWriteBufferLen,
                                          const char *pcCommandString);

static BaseType_t prvCLSCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString);

/*
 * Implements the "trace start" and "trace stop" commands;
 */
#if configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1
static BaseType_t prvStartStopTraceCommand(char *pcWriteBuffer,
                                           size_t xWriteBufferLen,
                                           const char *pcCommandString);
#endif

#if configINCLUDE_DEMO_DEBUG_STATS != 0
/* Structure that defines the "ip-debug-stats" command line command. */
static const CLI_Command_Definition_t xIPDebugStats = {
    "ip-debug-stats", /* The command string to type. */
    "ip-debug-stats:\n Shows some IP stack stats useful for debug - an "
    "example only.\n",
    prvDisplayIPDebugStats, /* The function to run. */
    0                       /* No parameters are expected. */
};
#endif /* configINCLUDE_DEMO_DEBUG_STATS */

/* Structure that defines the "run-time-stats" command line command.   This
generates a table that shows how much run time each task has */
static const CLI_Command_Definition_t xRunTimeStats = {
    "run-time-stats", /* The command string to type. */
    "run-time-stats:\n Displays a table showing how much processing time "
    "each FreeRTOS task has used\n\n",
    prvRunTimeStatsCommand, /* The function to run. */
    0                       /* No parameters are expected. */
};

/* Structure that defines the "task-stats" command line command.  This generates
a table that gives information on each task in the system. */
static const CLI_Command_Definition_t xTaskStats = {
    "task-stats", /* The command string to type. */
    "task-stats:\n Displays a table showing the state of each FreeRTOS "
    "task\n\n",
    prvTaskStatsCommand, /* The function to run. */
    0                    /* No parameters are expected. */
};

/* FreeRTOS Heap 4 and newlib heap stats */
static const CLI_Command_Definition_t xHeapStats = {
    "heap-stats", /* The command string to type. */
    "heap-stats:\n Displays FreeRTOS Heap 4 and newlib heap stats\n\n",
    prvHeapStatsCommand, /* The function to run. */
    0                    /* No parameters are expected. */
};

#ifdef ParameterEcho
/* Structure that defines the "echo_3_parameters" command line command.  This
takes exactly three parameters that the command simply echos back one at a
time. */
static const CLI_Command_Definition_t xThreeParameterEcho = {
    "echo-3-parameters",
    "echo-3-parameters <param1> <param2> <param3>:\n Expects three "
    "parameters, echos each in turn\n",
    prvThreeParameterEchoCommand, /* The function to run. */
    3 /* Three parameters are expected, which can take any value. */
};

/* Structure that defines the "echo_parameters" command line command.  This
takes a variable number of parameters that the command simply echos back one at
a time. */
static const CLI_Command_Definition_t xParameterEcho = {
    "echo-parameters",
    "echo-parameters <...>:\n Take variable number of parameters, echos each "
    "in turn\n",
    prvParameterEchoCommand, /* The function to run. */
    -1                       /* The user can enter any number of commands. */
};
#endif

static const CLI_Command_Definition_t xCLS = {"cls", "cls:\n Clear screen\n",
                                              prvCLSCommand, 0};

#ifdef ipconfigUSE_TCP
#if (ipconfigUSE_TCP == 1)
/* Structure that defines the "task-stats" command line command.  This generates
a table that gives information on each task in the system. */
static const CLI_Command_Definition_t xNetStats = {
    "net-stats", /* The command string to type. */
    "net-stats:\n Calls FreeRTOS_netstat()\n",
    prvNetStatCommand, /* The function to run. */
    0                  /* No parameters are expected. */
};
#endif /* ipconfigUSE_TCP == 1 */
#endif /* ifdef ipconfigUSE_TCP */

#if ipconfigSUPPORT_OUTGOING_PINGS == 1

/* Structure that defines the "ping" command line command.  This takes an IP
address or host name and (optionally) the number of bytes to ping as
parameters. */
static const CLI_Command_Definition_t xPing = {
    "ping",
    "ping <ipaddress> <optional:bytes to send>:\n for example, ping "
    "192.168.0.3 8, or ping www.example.com\n",
    prvPingCommand, /* The function to run. */
    -1 /* Ping can take either one or two parameter, so the number of parameters
          has to be determined by the ping command implementation. */
};

#endif /* ipconfigSUPPORT_OUTGOING_PINGS */

#if configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1
/* Structure that defines the "trace" command line command.  This takes a single
parameter, which can be either "start" or "stop". */
static const CLI_Command_Definition_t xStartStopTrace = {
    "trace",
    "trace [start | stop]:\n Starts or stops a trace recording for viewing "
    "in FreeRTOS+Trace\n",
    prvStartStopTraceCommand, /* The function to run. */
    1 /* One parameter is expected.  Valid values are "start" and "stop". */
};
#endif /* configINCLUDE_TRACE_RELATED_CLI_COMMANDS */

/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

static BaseType_t prvTaskStatsCommand(char *pcWriteBuffer,
                                      size_t xWriteBufferLen,
                                      const char *pcCommandString) {
    const char *const pcHeader =
        "Task          State  Priority  Stack	"
        "#\n************************************************\n";

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    /* Generate a table of task stats. */
    strcpy(pcWriteBuffer, pcHeader);
    vTaskList(pcWriteBuffer + strlen(pcHeader));

    /* There is no more data to return after this single string, so return
    pdFALSE. */
    return pdFALSE;
}
/*-----------------------------------------------------------*/

static BaseType_t prvRunTimeStatsCommand(char *pcWriteBuffer,
                                         size_t xWriteBufferLen,
                                         const char *pcCommandString) {
    const char *const pcHeader =
        "Task            Abs Time      % "
        "Time\n****************************************\n";

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    /* Generate a table of task stats. */
    strcpy(pcWriteBuffer, pcHeader);
    vTaskGetRunTimeStats(pcWriteBuffer + strlen(pcHeader));

    /* There is no more data to return after this single string, so return
    pdFALSE. */
    return pdFALSE;
}

static BaseType_t prvHeapStatsCommand(char *pcWriteBuffer,
                                      size_t xWriteBufferLen,
                                      const char *pcCommandString) {
    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    int i = snprintf(pcWriteBuffer, cmdMAX_OUTPUT_SIZE,
                     "heap4:\n\tTotal heap space remaining unallocated: %zu\n",
                     xPortGetFreeHeapSize());
    configASSERT(0 <= i && i < cmdMAX_OUTPUT_SIZE);

    i += snprintf(pcWriteBuffer + i, cmdMAX_OUTPUT_SIZE - i,
                  "\tMinimum ever free heap space: %zu\n",
                  xPortGetMinimumEverFreeHeapSize());
    configASSERT(0 <= i && i < cmdMAX_OUTPUT_SIZE);

    struct mallinfo mi;
    mi = mallinfo();

    i += snprintf(pcWriteBuffer + i, cmdMAX_OUTPUT_SIZE - i,
                  "glibc:\n\tTotal space allocated from system: %zu\n",
                  mi.arena);
    configASSERT(0 <= i && i < cmdMAX_OUTPUT_SIZE);

    i += snprintf(pcWriteBuffer + i, cmdMAX_OUTPUT_SIZE - i,
                  "\tTotal allocated space (bytes): %zu\n", mi.uordblks);
    configASSERT(0 <= i && i < cmdMAX_OUTPUT_SIZE);

    i += snprintf(pcWriteBuffer + i, cmdMAX_OUTPUT_SIZE - i,
                  "\tTotal free space (bytes): %zu\n", mi.fordblks);
    configASSERT(0 <= i && i < cmdMAX_OUTPUT_SIZE);

    /* There is no more data to return after this single string, so return
    pdFALSE. */
    return pdFALSE;
}

/*-----------------------------------------------------------*/

static BaseType_t prvThreeParameterEchoCommand(char *pcWriteBuffer,
                                               size_t xWriteBufferLen,
                                               const char *pcCommandString) {
    const char *pcParameter;
    BaseType_t xParameterStringLength, xReturn;
    static BaseType_t lParameterNumber = 0;

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    (void)pcCommandString;
    configASSERT(pcWriteBuffer);

    if (lParameterNumber == 0) {
        /* The first time the function is called after the command has been
        entered just a header string is returned. */
        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "The three parameters were:\n");

        /* Next time the function is called the first parameter will be echoed
        back. */
        lParameterNumber = 1L;

        /* There is more data to be returned as no parameters have been echoed
        back yet. */
        xReturn = pdPASS;
    } else {
        /* Obtain the parameter string. */
        pcParameter = FreeRTOS_CLIGetParameter(
            pcCommandString,        /* The command string itself. */
            lParameterNumber,       /* Return the next parameter. */
            &xParameterStringLength /* Store the parameter string length. */
        );

        /* Sanity check something was returned. */
        configASSERT(pcParameter);

        /* Return the parameter string. */
        memset(pcWriteBuffer, 0x00, xWriteBufferLen);
        snprintf(pcWriteBuffer, xWriteBufferLen, "%d: ", (int)lParameterNumber);
        strncat(pcWriteBuffer, pcParameter, xParameterStringLength);
        strncat(pcWriteBuffer, "\n", strlen("\n"));

        /* If this is the last of the three parameters then there are no more
        strings to return after this one. */
        if (lParameterNumber == 3L) {
            /* If this is the last of the three parameters then there are no
            more strings to return after this one. */
            xReturn = pdFALSE;
            lParameterNumber = 0L;
        } else {
            /* There are more parameters to return after this one. */
            xReturn = pdTRUE;
            lParameterNumber++;
        }
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t prvParameterEchoCommand(char *pcWriteBuffer,
                                          BaseType_t xWriteBufferLen,
                                          const char *pcCommandString) {
    const char *pcParameter;
    BaseType_t xParameterStringLength, xReturn;
    static BaseType_t lParameterNumber = 0;

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    (void)pcCommandString;
    configASSERT(pcWriteBuffer);

    if (lParameterNumber == 0) {
        /* The first time the function is called after the command has been
        entered just a header string is returned. */
        snprintf(pcWriteBuffer, xWriteBufferLen, "The parameters were:\n");

        /* Next time the function is called the first parameter will be echoed
        back. */
        lParameterNumber = 1L;

        /* There is more data to be returned as no parameters have been echoed
        back yet. */
        xReturn = pdPASS;
    } else {
        /* Obtain the parameter string. */
        pcParameter = FreeRTOS_CLIGetParameter(
            pcCommandString,        /* The command string itself. */
            lParameterNumber,       /* Return the next parameter. */
            &xParameterStringLength /* Store the parameter string length. */
        );

        if (pcParameter != NULL) {
            /* Return the parameter string. */
            memset(pcWriteBuffer, 0x00, xWriteBufferLen);
            snprintf(pcWriteBuffer, xWriteBufferLen,
                     "%d: ", (int)lParameterNumber);
            // Truncate, if necessary
            BaseType_t m = strlen(pcWriteBuffer);
            BaseType_t x = xParameterStringLength + 2;  // for CRLF
            int n = m + x > xWriteBufferLen ? xWriteBufferLen - m - 2
                                            : xParameterStringLength;
            if (n > 0) {
                strncat(pcWriteBuffer, pcParameter, n);
                strncat(pcWriteBuffer, "\n", strlen("\n"));
            } else {
                pcWriteBuffer[xWriteBufferLen - 1] = '\n';
                pcWriteBuffer[xWriteBufferLen - 2] = '\r';
            }
            /* There might be more parameters to return after this one. */
            xReturn = pdTRUE;
            lParameterNumber++;
        } else {
            /* No more parameters were found.  Make sure the write buffer does
            not contain a valid string. */
            pcWriteBuffer[0] = 0x00;

            /* No more data to return. */
            xReturn = pdFALSE;

            /* Start over the next time this command is executed. */
            lParameterNumber = 0;
        }
    }

    return xReturn;
}

/*-----------------------------------------------------------*/
static BaseType_t prvCLSCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString) {
    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    strcpy(pcWriteBuffer, "\033[2J\033[H");

    /* There is no more data to return after this single string, so return
    pdFALSE. */
    return pdFALSE;
}
/*-----------------------------------------------------------*/

#ifdef ipconfigUSE_TCP

#if ipconfigUSE_TCP == 1

static BaseType_t prvNetStatCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                    const char *pcCommandString) {
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    (void)pcCommandString;

    FreeRTOS_netstat();
    snprintf(pcWriteBuffer, xWriteBufferLen,
             "FreeRTOS_netstat() called - output uses FreeRTOS_printf\n");
    return pdFALSE;
}

#endif /* ipconfigUSE_TCP == 1 */

#endif /* ifdef ipconfigUSE_TCP */
/*-----------------------------------------------------------*/

#if ipconfigSUPPORT_OUTGOING_PINGS == 1

static BaseType_t prvPingCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                 const char *pcCommandString) {
    char *pcParameter;
    BaseType_t lParameterStringLength, xReturn;
    uint32_t ulIPAddress, ulBytesToPing;
    const uint32_t ulDefaultBytesToPing = 8UL;
    char cBuffer[16];

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    (void)pcCommandString;
    configASSERT(pcWriteBuffer);

    /* Start with an empty string. */
    pcWriteBuffer[0] = 0x00;

    /* Obtain the number of bytes to ping. */
    pcParameter = (char *)FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        2,                      /* Return the second parameter. */
        &lParameterStringLength /* Store the parameter string length. */
    );

    if (pcParameter == NULL) {
        /* The number of bytes was not specified, so default it. */
        ulBytesToPing = ulDefaultBytesToPing;
    } else {
        ulBytesToPing = atol(pcParameter);
    }

    /* Obtain the IP address string. */
    pcParameter = (char *)FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the first parameter. */
        &lParameterStringLength /* Store the parameter string length. */
    );

    /* Sanity check something was returned. */
    configASSERT(pcParameter);

    /* Attempt to obtain the IP address.   If the first character is not a
    digit, assume the host name has been passed in. */
    if ((*pcParameter >= '0') && (*pcParameter <= '9')) {
        ulIPAddress = FreeRTOS_inet_addr(pcParameter);
    } else {
        /* Terminate the host name. */
        pcParameter[lParameterStringLength] = 0x00;

        /* Attempt to resolve host. */
        ulIPAddress = FreeRTOS_gethostbyname(pcParameter);
    }

    /* Convert IP address, which may have come from a DNS lookup, to string. */
    FreeRTOS_inet_ntoa(ulIPAddress, cBuffer);

    if (ulIPAddress != 0) {
        xReturn = FreeRTOS_SendPingRequest(ulIPAddress, (uint16_t)ulBytesToPing,
                                           portMAX_DELAY);
    } else {
        xReturn = pdFALSE;
    }

    if (xReturn == pdFALSE) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "%s",
                 "Could not send ping request\n");
    } else {
        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "Ping sent to %s with identifier %d\n", cBuffer, xReturn);
    }

    return pdFALSE;
}
/*-----------------------------------------------------------*/

#endif /* ipconfigSUPPORT_OUTGOING_PINGS */

#if configINCLUDE_DEMO_DEBUG_STATS != 0

static BaseType_t prvDisplayIPDebugStats(char *pcWriteBuffer,
                                         size_t xWriteBufferLen,
                                         const char *pcCommandString) {
    static BaseType_t xIndex = -1;
    extern ExampleDebugStatEntry_t xIPTraceValues[];
    BaseType_t xReturn;

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    (void)pcCommandString;
    configASSERT(pcWriteBuffer);

    xIndex++;

    if (xIndex < xExampleDebugStatEntries()) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "%s %d\n",
                 xIPTraceValues[xIndex].pucDescription,
                 (int)xIPTraceValues[xIndex].ulData);
        xReturn = pdPASS;
    } else {
        /* Reset the index for the next time it is called. */
        xIndex = -1;

        /* Ensure nothing remains in the write buffer. */
        pcWriteBuffer[0] = 0x00;
        xReturn = pdFALSE;
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

#endif /* configINCLUDE_DEMO_DEBUG_STATS */

#if configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1

static BaseType_t prvStartStopTraceCommand(char *pcWriteBuffer,
                                           size_t xWriteBufferLen,
                                           const char *pcCommandString) {
    const char *pcParameter;
    BaseType_t lParameterStringLength;

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    (void)pcCommandString;
    configASSERT(pcWriteBuffer);

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the first parameter. */
        &lParameterStringLength /* Store the parameter string length. */
    );

    /* Sanity check something was returned. */
    configASSERT(pcParameter);

    /* There are only two valid parameter values. */
    if (strncmp(pcParameter, "start", strlen("start")) == 0) {
        /* Start or restart the trace. */
        vTraceStop();
        vTraceClear();
        vTraceStart();

        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "Trace recording (re)started.\n");
    } else if (strncmp(pcParameter, "stop", strlen("stop")) == 0) {
        /* End the trace, if one is running. */
        vTraceStop();
        snprintf(pcWriteBuffer, xWriteBufferLen, "Stopping trace recording.\n");
    } else {
        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "Valid parameters are 'start' and 'stop'.\n");
    }

    /* There is no more data to return after this single string, so return
    pdFALSE. */
    return pdFALSE;
}

#endif /* configINCLUDE_TRACE_RELATED_CLI_COMMANDS */

/*-----------------------------------------------------------*/
static BaseType_t diskInfo(char *pcWriteBuffer, size_t xWriteBufferLen,
                           const char *pcCommandString) {
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the first parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );

    /* Sanity check something was returned. */
    configASSERT(pcParameter);

    sd_card_t *sd = sd_get_by_name(pcParameter);
    if (!sd) return pdFALSE;
    for (size_t i = 0; i < sd->ff_disk_count; ++i) {
        FF_Disk_t *pxDisk = sd->ff_disks[i];
        if (pxDisk) {
            FF_SDDiskShowPartition(pxDisk);
        }
    }
    return pdFALSE;
}
static const CLI_Command_Definition_t xDiskInfo = {
    "diskinfo", /* The command string to type. */
    "\ndiskinfo <device name>:\n Print information about mounted "
    "partitions\n"
    "\te.g.: \"diskinfo sd0\"\n",
    diskInfo, /* The function to run. */
    1         /* One parameter is expected. */
};
/*-----------------------------------------------------------*/
bool die_now;
static BaseType_t die_fn(char *pcWriteBuffer, size_t xWriteBufferLen,
                         const char *pcCommandString) {
    (void)pcCommandString;
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    die_now = true;

    return pdFALSE;
}
static const CLI_Command_Definition_t xDie = {
    "die",                                    /* The command string to type. */
    "die:\n Kill background tasks\n", die_fn, /* The function to run. */
    0                                         /* No parameters are expected. */
};
/*-----------------------------------------------------------*/
static BaseType_t unDie(char *pcWriteBuffer, size_t xWriteBufferLen,
                        const char *pcCommandString) {
    (void)pcCommandString;
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    die_now = false;

    return pdFALSE;
}
static const CLI_Command_Definition_t xUnDie = {
    "undie", /* The command string to type. */
    "\nundie:\n Allow background tasks to live again\n",
    unDie, /* The function to run. */
    0      /* No parameters are expected. */
};

static BaseType_t reset_fn(char *pcWriteBuffer, size_t xWriteBufferLen,
                           const char *pcCommandString) {
    (void)pcCommandString;
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    SYSTEM_RESET();

    return pdFALSE;
}
static const CLI_Command_Definition_t xReset = {
    "reset", /* The command string to type. */
    "\nreset:\n Soft system reset\n", reset_fn, /* The function to run. */
    0 /* No parameters are expected. */
};
/*-----------------------------------------------------------*/
static BaseType_t setrtc_cmd(char *pcWriteBuffer, size_t xWriteBufferLen,
                         const char *pcCommandString) {
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    BaseType_t xParameterStringLength;

    const char *secStr = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        6,                      /* Return the numbered parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    configASSERT(secStr);
    int sec = atoi(secStr);

    const char *minStr = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        5,                      /* Return the numbered parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    configASSERT(minStr);
    int min = atoi(minStr);

    const char *hourStr = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        4,                      /* Return the numbered parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    configASSERT(hourStr);
    int hour = atoi(hourStr);

    const char *yearStr = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        3,                      /* Return the numbered parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    configASSERT(yearStr);
    int year = atoi(yearStr) + 2000;

    const char *monthStr = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        2,                      /* Return the numbered parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    configASSERT(monthStr);
    int month = atoi(monthStr);

    const char *dateStr = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the numbered parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    configASSERT(dateStr);
    int date = atoi(dateStr);

    // SetDateTime(dateStr, monthStr, yearStr, secStr, minStr, hourStr);

    datetime_t t = {.year = year,
                    .month = month,
                    .day = date,
                    .dotw = 0,  // 0 is Sunday, so 5 is Friday
                    .hour = hour,
                    .min = min,
                    .sec = sec};
    //bool r = rtc_set_datetime(&t);
    setrtc(&t);

    return pdFALSE;
}
static const CLI_Command_Definition_t xSetRTC = {
    "setrtc", /* The command string to type. */
    "\nsetrtc <DD> <MM> <YY> <hh> <mm> <ss>:\n"
    " Set Real Time Clock\n"
    " Parameters: new date (DD MM YY) new time in 24-hour format (hh "
    "mm ss)\n",
    setrtc_cmd, /* The function to run. */
    6       /* parameters are expected. */
};
/*-----------------------------------------------------------*/
static void printDateTime() {
    char buf[128] = {0};

    //	PrintDateTime();

    time_t epoch_secs = FreeRTOS_time(NULL);
    struct tm *ptm = localtime(&epoch_secs);
    size_t n = strftime(buf, sizeof(buf), "%c", ptm);
    configASSERT(n);
    printf("%s\n", buf);
    strftime(buf, sizeof(buf), "%j",
             ptm);  // The day of the year as a decimal number (range
                    // 001 to 366).
    printf("Day of year: %s\n", buf);
}
static BaseType_t date(char *pcWriteBuffer, size_t xWriteBufferLen,
                       const char *pcCommandString) {
    (void)pcCommandString;
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    printDateTime();

    return pdFALSE;
}
static const CLI_Command_Definition_t xDate = {
    "date", /* The command string to type. */
    "\ndate:\n Print current date and time\n\n", date, /* The function to run. */
    0 /* No parameters are expected. */
};
/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/
extern void my_test(int v);
static BaseType_t test(char *pcWriteBuffer, size_t xWriteBufferLen,
                       const char *pcCommandString) {
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    const char *pcParameter;
    BaseType_t xParameterStringLength;

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the first parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    /* Sanity check something was returned. */
    configASSERT(pcParameter);

    my_test(atoi(pcParameter));
    //    test_harness();

    return pdFALSE;
}
static const CLI_Command_Definition_t xTest = {
    "test", /* The command string to type. */
    "\ntest <number>:\n Development test\n", test, /* The function to run. */
    1 /* One parameter is expected. */
};
/*-----------------------------------------------------------*/

void vRegisterCLICommands(void) {
    static BaseType_t xCommandRegistered = pdFALSE;

    /* Prevent commands being registered more than once. */
    if (xCommandRegistered == pdFALSE) {
        /* Register all the command line commands defined immediately above. */
        FreeRTOS_CLIRegisterCommand(&xTaskStats);
        FreeRTOS_CLIRegisterCommand(&xRunTimeStats);
        FreeRTOS_CLIRegisterCommand(&xHeapStats);
        //		FreeRTOS_CLIRegisterCommand( &xThreeParameterEcho );
        //		FreeRTOS_CLIRegisterCommand( &xParameterEcho );
        FreeRTOS_CLIRegisterCommand(&xCLS);

#if ipconfigSUPPORT_OUTGOING_PINGS == 1
        { FreeRTOS_CLIRegisterCommand(&xPing); }
#endif /* ipconfigSUPPORT_OUTGOING_PINGS */

#ifdef ipconfigUSE_TCP
        {
#if ipconfigUSE_TCP == 1
            { FreeRTOS_CLIRegisterCommand(&xNetStats); }
#endif /* ipconfigUSE_TCP == 1 */
        }
#endif /* ifdef ipconfigUSE_TCP */

#if configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1
        { FreeRTOS_CLIRegisterCommand(&xStartStopTrace); }
#endif

        xCommandRegistered = pdTRUE;
    }
}

void vRegisterMyCLICommands(void) {
    /* Register all the command line commands defined immediately above.
     */
    FreeRTOS_CLIRegisterCommand(&xDiskInfo);
    FreeRTOS_CLIRegisterCommand(&xSetRTC);
    FreeRTOS_CLIRegisterCommand(&xDate);
    FreeRTOS_CLIRegisterCommand(&xDie);
    FreeRTOS_CLIRegisterCommand(&xUnDie);
    FreeRTOS_CLIRegisterCommand(&xTest);
    FreeRTOS_CLIRegisterCommand(&xReset);
}
