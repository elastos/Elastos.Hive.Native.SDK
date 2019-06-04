#ifndef __ELA_ERROR_H__
#define __ELA_ERROR_H__

#include "ela_hive.h"

#define ELA_MK_ERROR(facility, code)  (0x80000000 | ((facility) << 24) | \
                    ((((code) & 0x80000000) >> 8) | ((code) & 0x7FFFFFFF)))

#define ELA_GENERAL_ERROR(code)       ELA_MK_ERROR(ELAF_GENERAL, code)
#define ELA_SYS_ERROR(code)           ELA_MK_ERROR(ELAF_SYS, code)
#define ELA_HTTPC_ERROR(code)         ELA_MK_ERROR(ELAF_HTTP_CLIENT, code)
#define ELA_HTTPS_ERROR(code)         ELA_MK_ERROR(ELAF_HTTP_SERVER, code)

void ela_set_error(int error);

typedef int (*strerror_t)(int errnum, char *, size_t);

int ela_register_strerror(int facility, strerror_t strerr);

#endif // __ELA_ERROR_H__
