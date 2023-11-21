
#include "FreeRTOS_errno_FAT.h"
//
#include "FreeRTOS_strerror.h"

char *FreeRTOS_strerror(int errnum) {
    switch (errnum) {
        case pdFREERTOS_ERRNO_NONE         : return "No errors";
        case pdFREERTOS_ERRNO_ENOENT       : return "No such file or directory";
        case pdFREERTOS_ERRNO_EIO          : return "I/O error";
        case pdFREERTOS_ERRNO_ENXIO        : return "No such device or address";
        case pdFREERTOS_ERRNO_EBADF        : return "Bad file number";
        // case pdFREERTOS_ERRNO_EAGAIN       : return "No more processes";
        case pdFREERTOS_ERRNO_EWOULDBLOCK  : return "Operation would block (Resource temporarily unavailable)";
        case pdFREERTOS_ERRNO_ENOMEM       : return "Not enough memory";
        case pdFREERTOS_ERRNO_EACCES       : return "Permission denied";
        case pdFREERTOS_ERRNO_EFAULT       : return "Bad address";
        case pdFREERTOS_ERRNO_EBUSY        : return "Mount device busy";
        case pdFREERTOS_ERRNO_EEXIST       : return "File exists";
        case pdFREERTOS_ERRNO_EXDEV        : return "Cross-device link";
        case pdFREERTOS_ERRNO_ENODEV       : return "No such device";
        case pdFREERTOS_ERRNO_ENOTDIR      : return "Not a directory";
        case pdFREERTOS_ERRNO_EISDIR       : return "Is a directory";
        case pdFREERTOS_ERRNO_EINVAL       : return "Invalid argument";
        case pdFREERTOS_ERRNO_ENOSPC       : return "No space left on device";
        case pdFREERTOS_ERRNO_ESPIPE       : return "Illegal seek";
        case pdFREERTOS_ERRNO_EROFS        : return "Read only file system";
        case pdFREERTOS_ERRNO_EUNATCH      : return "Protocol driver not attached";
        case pdFREERTOS_ERRNO_EBADE        : return "Invalid exchange";
        case pdFREERTOS_ERRNO_EFTYPE       : return "Inappropriate file type or format";
        case pdFREERTOS_ERRNO_ENMFILE      : return "No more files";
        case pdFREERTOS_ERRNO_ENOTEMPTY    : return "Directory not empty";
        case pdFREERTOS_ERRNO_ENAMETOOLONG : return "File or path name too long";
        case pdFREERTOS_ERRNO_EOPNOTSUPP   : return "Operation not supported on transport endpoint";
        case pdFREERTOS_ERRNO_ENOBUFS      : return "No buffer space available";
        case pdFREERTOS_ERRNO_ENOPROTOOPT  : return "Protocol not available";
        case pdFREERTOS_ERRNO_EADDRINUSE   : return "Address already in use";
        case pdFREERTOS_ERRNO_ETIMEDOUT    : return "Connection timed out";
        case pdFREERTOS_ERRNO_EINPROGRESS  : return "Connection already in progress";
        case pdFREERTOS_ERRNO_EALREADY     : return "Socket already connected";
        case pdFREERTOS_ERRNO_EADDRNOTAVAIL: return "Address not available";
        case pdFREERTOS_ERRNO_EISCONN      : return "Socket is already connected";
        case pdFREERTOS_ERRNO_ENOTCONN     : return "Socket is not connected";
        case pdFREERTOS_ERRNO_ENOMEDIUM    : return "No medium inserted";
        case pdFREERTOS_ERRNO_EILSEQ       : return "An invalid UTF-16 sequence was encountered.";
        case pdFREERTOS_ERRNO_ECANCELED    : return "Operation canceled.";
    }
    return "";
}