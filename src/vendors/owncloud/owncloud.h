#ifndef __OWNCLOUD_CLIENT_H__
#define __OWNCLOUD_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ela_hive.h"

int owncloud_client_new(const HiveOptions *, HiveClient **);

#ifdef __cplusplus
}
#endif

#endif // __OWNCLOUD_CLIENT_H__
