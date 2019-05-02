#ifndef __HIVE_IPFS_CLIENT_H__
#define __HIVE_IPFS_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "client.h"

HiveClient *hiveipfs_client_new(const HiveOptions *);

#ifdef __cplusplus
}
#endif

#endif // __HIVE_IPFS_CLIENT_H__
