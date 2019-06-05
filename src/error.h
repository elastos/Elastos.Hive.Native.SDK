#ifndef __ERROR_H__
#define __ERROR_H__

#include "ela_hive.h"

#define HIVE_MK_ERROR(facility, code)  (0x80000000 | ((facility) << 24) | \
                    ((((code) & 0x80000000) >> 8) | ((code) & 0x7FFFFFFF)))

#define HIVE_GENERAL_ERROR(code)       HIVE_MK_ERROR(HIVEF_GENERAL, code)
#define HIVE_SYS_ERROR(code)           HIVE_MK_ERROR(HIVEF_SYS, code)
#define HIVE_HTTPC_ERROR(code)         HIVE_MK_ERROR(HIVEF_HTTP_CLIENT, code)
#define HIVE_HTTPS_ERROR(code)         HIVE_MK_ERROR(HIVEF_HTTP_SERVER, code)
#define HIVE_HTTP_TSX_ERROR(code)      HIVE_MK_ERROR(HIVEF_HTTP_TRANSACTION, code)

void hive_set_error(int err);

typedef int (*strerror_t)(int errnum, char *, size_t);

int hive_register_strerror(int facility, strerror_t strerr);

#endif // __ERROR_H__
