
#include "lwip/apps/fs.h"
#include "lwip/apps/httpd.h"
//
#include "FreeRTOS.h"
//
#include "ff_headers.h"
#include "ff_sddisk.h"
#include "ff_stdio.h"
#include "ff_utils.h"

static char const *const mnt_pnt = "sd0";

void sd_init_mount() {
    FF_Disk_t *pxDisk = FF_SDDiskInit(mnt_pnt);
    if (!pxDisk)
        exit(1);
    FF_Error_t e = FF_Mount(pxDisk, 0);
    if (FF_ERR_NONE != e) {
        FF_PRINTF("FF_Mount error: %s\n", FF_GetErrMessage(e));
        exit(1);
    }
    FF_FS_Add("/sd0", pxDisk);
}

int fs_open_custom(struct fs_file *file, const char *name) {
    char buf[LWIP_HTTPD_MAX_REQUEST_URI_LEN + strlen(mnt_pnt) + 1];
    snprintf(buf, sizeof buf, "/%s%s", mnt_pnt, name);
    FF_FILE *pxFile = ff_fopen(buf, "r");
    if (!pxFile) {
        FF_PRINTF("ff_fopen(,\"%s\") failed: %s (%d)\n", buf,
                  strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
        return false;
    }
    file->data = NULL;
    file->len = ff_filelength(pxFile);
    file->index = 0;
    file->pextension = pxFile;
    return true;
}
void fs_close_custom(struct fs_file *file) {
    if (-1 == ff_fclose(file->pextension)) {
        FF_PRINTF("ff_fclose failed: %s (%d)\n",
                  strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
    }
}
int fs_read_custom(struct fs_file *file, char *buffer, int count) {
    if (file->index < file->len) {
        // Read "count" bytes from SD, fill in buffer, and return the amount of bytes read
        size_t br = ff_fread(buffer, 1, count, file->pextension);
        if (br < (size_t)count) {
            FF_PRINTF("ff_fread(,,%d,) returned %zu bytes: %s (%d)\n",
                      count, br, strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
        }
        file->index += br;
        return br;
    } else {
        return FS_READ_EOF;
    }
}
