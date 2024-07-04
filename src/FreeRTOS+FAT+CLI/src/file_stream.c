/*
 * file_stream.c
 *
 *  Created on: Jun 20, 2024
 *      Author: carlk
 */

#define _GNU_SOURCE
#include <stdio.h>
//
#include "FreeRTOS.h"
//
#include "ff_headers.h"
#include "ff_stdio.h"
//
#include "FreeRTOS_strerror.h"
#include "my_debug.h"
//
#include "file_stream.h"

typedef struct {
    FF_FILE *ff_file;
} cookie_t;

//functions.read should return -1 on failure, or else the number of bytes read (0 on EOF).
// It is similar to read, except that cookie will be passed as the first argument.
static ssize_t cookie_read_function(void *vcookie_p, char *buf, size_t n) {
    cookie_t *cookie_p = vcookie_p;
    void *pvBuffer = buf;
    size_t xSize = 1;
    size_t xItems = n;
    FF_FILE *pxStream = cookie_p->ff_file;
    stdioSET_ERRNO(0);
    size_t nr = ff_fread(pvBuffer, xSize, xItems, pxStream);
    int error = stdioGET_ERRNO();
    if (error) {
        DBG_PRINTF("%s: %s: %s: %s (%d)\n", pcTaskGetName(NULL), __func__, "ff_fread",
                FreeRTOS_strerror(error), error);
        return -1;
    } else return nr;
}

//functions.write should return -1 on failure, or else the number of bytes written.
// It is similar to write, except that cookie will be passed as the first argument.
static ssize_t cookie_write_function(void *vcookie_p, const char *buf, size_t n) {
    cookie_t *cookie_p = vcookie_p;
    const void *pvBuffer = buf;
    size_t xSize = 1;
    size_t xItems = n;
    FF_FILE * pxStream = cookie_p->ff_file;
    size_t nw = ff_fwrite(pvBuffer, xSize, xItems, pxStream);
    // If the number of items written to the file is less than the xItems value
    // then the task's errno is set to indicate the reason.
    if (nw < xItems) {
        int error = stdioGET_ERRNO();
        if (error) {
            DBG_PRINTF("%s: %s: %s: %s (%d)\n", pcTaskGetName(NULL), __func__, "ff_fwrite",
                    FreeRTOS_strerror(error), error);
        }
        return -1;
    } else {
        return nw;
    }
}

//functions.seek should return -1 on failure, and 0 on success,
// with off set to the current file position.
// It is a cross between lseek and fseek, with the whence argument interpreted in the same manner.
static int cookie_seek_function(void *vcookie_p, off_t *off, int whence) {
    int ff_whence = -1;
    switch (whence) {
        case SEEK_SET:
            ff_whence = FF_SEEK_SET;
            break;
        case SEEK_CUR:
            ff_whence = FF_SEEK_CUR;
            break;
        case SEEK_END:
            ff_whence = FF_SEEK_END;
            break;
        default:
            ASSERT_CASE_NOT(whence);
    }
    cookie_t *cookie_p = vcookie_p;
    int err = ff_fseek(cookie_p->ff_file, *off, ff_whence);
    if (!err)
        *off = ff_ftell(cookie_p->ff_file);
    return err;
}

//functions.close should return -1 on failure, or 0 on success.
// It is similar to close, except that cookie will be passed as the first argument.
// A failed close will still invalidate the stream.
static int cookie_close_function(void *vcookie_p) {
    cookie_t *cookie_p = vcookie_p;
    FF_FILE *pxStream = cookie_p->ff_file;
    vPortFree(cookie_p);
    return ff_fclose(pxStream);
}

FILE *open_file_stream( const char *pcFile, const char *pcMode ) {
    FF_FILE *f = ff_fopen(pcFile, pcMode);
    if (!f) return NULL;

    cookie_t *cookie_p = pvPortMalloc(sizeof(cookie_t));
    if (!cookie_p) {
        ff_fclose(f);
        return NULL;
    }
    cookie_p->ff_file = f;

    cookie_io_functions_t iofs = {
            cookie_read_function,
            cookie_write_function,
            cookie_seek_function,
            cookie_close_function
    };

    /* create the stream */
    FILE *file = fopencookie(cookie_p, pcMode, iofs);
    if (!file) cookie_close_function(cookie_p);
    return (file);
}
