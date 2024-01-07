/* simple.c
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
// Adapted from "FATFileSystem example"
//  at https://os.mbed.com/docs/mbed-os/v5.15/apis/fatfilesystem.html

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//
#include "FreeRTOS_strerror.h"
#include "ff_stdio.h"
#include "ff_utils.h"
#include "my_debug.h"

// Maximum number of elements in buffer
#define BUFFER_MAX_LEN 10

#define TRACE_PRINTF(fmt, args...)
// #define TRACE_PRINTF printf

void simple() {
    int err = 0;

    IMSG_PRINTF("\nSimple Test\n");

    // Open the numbers file
    IMSG_PRINTF("Opening \"numbers.txt\"... ");
    FF_FILE *f = ff_fopen("numbers.txt", "r+");
    IMSG_PRINTF("%s\n", (!f ? "Fail :(" : "OK"));
    if (!f) {
        // Create the numbers file if it doesn't exist
        IMSG_PRINTF("No file found, creating a new file... ");

        f = ff_fopen("numbers.txt", "w+");
        IMSG_PRINTF("%s\n", (!f ? "Fail :(" : "OK"));
        if (!f) {
            EMSG_PRINTF("error: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()),
                        stdioGET_ERRNO());
            return;
        }
        for (int i = 0; i < 10; i++) {
            IMSG_PRINTF("\rWriting numbers (%d/%d)... ", i, 10);

            err = ff_fprintf(f, "    %d\n", i);
            if (err < 0) {
                EMSG_PRINTF("Fail :(\n");
                EMSG_PRINTF("error: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()),
                            stdioGET_ERRNO());
                return;
            }
        }
        IMSG_PRINTF("\rWriting numbers (%d/%d)... OK\n", 10, 10);
        IMSG_PRINTF("Seeking file... ");
        err = ff_fseek(f, 0, FF_SEEK_SET);
        IMSG_PRINTF("%s\n", (err < 0 ? "Fail :(" : "OK"));
        if (err < 0) {
            EMSG_PRINTF("error: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()),
                        stdioGET_ERRNO());
            return;
        }
    }
    // Go through and increment the numbers
    for (int i = 0; i < 10; i++) {
        IMSG_PRINTF("\nIncrementing numbers (%d/%d)... ", i, 10);

        // Get current stream position
        long pos = ff_ftell(f);
        // Parse out the number and increment
        char buf[BUFFER_MAX_LEN];
        if (!ff_fgets(buf, BUFFER_MAX_LEN, f)) {
            EMSG_PRINTF("error: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()),
                        stdioGET_ERRNO());
            return;
        }
        char *endptr;
        int32_t number = strtol(buf, &endptr, 10);
        if ((endptr == buf) ||            // No character was read
            (*endptr && *endptr != '\n')  // The whole input was not converted
        ) {
            continue;
        }
        number += 1;

        // Seek to beginning of number
        ff_fseek(f, pos, FF_SEEK_SET);

        // Store number
        ff_fprintf(f, "    %d\n", (int)number);

        // Flush between write and read on same file
        //         ff_fflush(f);
    }
    IMSG_PRINTF("\rIncrementing numbers (%d/%d)... OK\n", 10, 10);

    // Close the file which also flushes any cached writes
    IMSG_PRINTF("Closing \"numbers.txt\"... ");
    err = ff_fclose(f);
    IMSG_PRINTF("%s\n", (err < 0 ? "Fail :(" : "OK"));
    if (err < 0) {
        EMSG_PRINTF("error: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()),
                    stdioGET_ERRNO());
        return;
    }

    ls("");

    // Display the numbers file
    IMSG_PRINTF("Opening \"numbers.txt\"... ");
    f = ff_fopen("numbers.txt", "r");
    IMSG_PRINTF("%s\n", (!f ? "Fail :(" : "OK"));
    if (!f) {
        EMSG_PRINTF("f_open error: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
        return;
    }

    IMSG_PRINTF("numbers:\n");
    while (!ff_feof(f)) {
        int c = ff_fgetc(f);
        IMSG_PRINTF("%c", c);
    }

    IMSG_PRINTF("\nClosing \"numbers.txt\"... ");
    err = ff_fclose(f);
    IMSG_PRINTF("%s\n", (err < 0 ? "Fail :(" : "OK"));
    if (err < 0) {
        EMSG_PRINTF("error: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()),
                    stdioGET_ERRNO());
    }
}
