#ifndef __ONEDRIVE_CLIENT_H__
#define __ONEDRIVE_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ela_hive.h"

int onedrive_client_new(const HiveOptions *, HiveClient **);

#ifdef __cplusplus
}
#endif

#endif // __ONEDRIVE_CLIENT_H__
