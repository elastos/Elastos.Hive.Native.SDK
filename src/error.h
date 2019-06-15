#ifndef __HIVE_ERROR_H__
#define __HIVE_ERROR_H__

#ifdef __cplusplus
extern "C" {
#endif

void hive_set_error(int err);

typedef int (*strerror_t)(int errnum, char *, size_t);

int hive_register_strerror(int facility, strerror_t strerr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __HIVE_ERROR_H__
