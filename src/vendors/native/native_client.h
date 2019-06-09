#ifndef __NATIVE_CLIENT_H__
#define __NATIVE_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ela_hive.h"

HiveClient *native_client_new(const HiveOptions *);

#ifdef __cplusplus
}
#endif

#endif // __NATIVE_CLIENT_H__
