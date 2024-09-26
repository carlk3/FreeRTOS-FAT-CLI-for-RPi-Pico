
#include "pico/error.h"
#include "pico_error_str.h"

char const *pico_error_str(int error) {
    switch (error) {
    case PICO_OK: return "No error; the operation succeeded";
    //case PICO_ERROR_NONE: return "No error; the operation succeeded";
    case PICO_ERROR_GENERIC: return "An unspecified error occurred";
    case PICO_ERROR_TIMEOUT: return "The function failed due to timeout";
    case PICO_ERROR_NO_DATA: return "Attempt for example to read from an empty buffer/FIFO";
    case PICO_ERROR_NOT_PERMITTED: return "Permission violation e.g. write to read-only flash partition, or security violation";
    case PICO_ERROR_INVALID_ARG: return "Argument is outside of range of supported values`";
    case PICO_ERROR_IO: return "An I/O error occurred";
    case PICO_ERROR_BADAUTH: return "The authorization failed due to bad credentials";
    case PICO_ERROR_CONNECT_FAILED: return "The connection failed";
    case PICO_ERROR_INSUFFICIENT_RESOURCES: return "Dynamic allocation of resources failed";
    case PICO_ERROR_INVALID_ADDRESS: return "Address argument was out-of-bounds or was determined to be an address that the caller may not access";
    case PICO_ERROR_BAD_ALIGNMENT: return "Address was mis-aligned (usually not on word boundary)";
    case PICO_ERROR_INVALID_STATE: return "Something happened or failed to happen in the past, and consequently we (currently) can't service the request";
    case PICO_ERROR_BUFFER_TOO_SMALL: return "A user-allocated buffer was too small to hold the result or working state of this function";
    case PICO_ERROR_PRECONDITION_NOT_MET: return "The call failed because another function must be called first";
    case PICO_ERROR_MODIFIED_DATA: return "Cached data was determined to be inconsistent with the actual version of the data";
    case PICO_ERROR_INVALID_DATA: return "A data structure failed to validate";
    case PICO_ERROR_NOT_FOUND: return "Attempted to access something that does not exist; or, a search failed";
    case PICO_ERROR_UNSUPPORTED_MODIFICATION: return "Write is impossible based on previous writes; e.g. attempted to clear an OTP bit";
    case PICO_ERROR_LOCK_REQUIRED: return "A required lock is not owned";
    case PICO_ERROR_VERSION_MISMATCH: return "A version mismatch occurred (e.g. trying to run PIO version 1 code on RP2040)";
    case PICO_ERROR_RESOURCE_IN_USE: return "The call could not proceed because requires resourcesw were unavailable";
    }
    return "Unknown error";
}
