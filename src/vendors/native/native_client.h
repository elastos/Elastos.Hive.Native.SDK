#ifndef __HIVE_NATIVE_CLIENT_H__
#define __HIVE_NATIVE_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ela_hive.h"

int native_client_new(const HiveOptions *, HiveClient **);

#ifdef __cplusplus
}
#endif

#endif // __HIVE_NATIVE_CLIENT_H__
