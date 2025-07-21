#ifndef _XLCD_BBL_ERRORS
#define _XLCD_BBL_ERRORS

#ifdef __cplusplus
extern "C"
{
#endif

#include <pgmspace.h>

extern int hms_error_length;

extern const char* const hms_error_keys[] PROGMEM;

extern const char* const hms_error_values[] PROGMEM;

extern int device_error_length;

extern const char* const device_error_keys[] PROGMEM;

extern const char* const device_error_values[] PROGMEM;

extern int message_containing_retry_total;

extern int message_containing_done_total;

extern const char* const message_containing_retry[] PROGMEM;

extern const char* const message_containing_done[] PROGMEM;

#ifdef __cplusplus
}
#endif

#endif