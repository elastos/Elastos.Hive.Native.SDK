#ifndef __ONEDRIVE_CLIENT_H__
#define __ONEDRIVE_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "client.h"

HiveClient *onedrive_client_new(const HiveOptions *);

#ifdef __cplusplus
}
#endif

#endif // __ONEDRIVE_CLIENT_H__
