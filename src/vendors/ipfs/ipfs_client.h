#ifndef __IPFS_CLIENT_H__
#define __IPFS_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ela_hive.h"
#include "http_client.h"

HiveClient *ipfs_client_new(const HiveOptions *);

#ifdef __cplusplus
}
#endif

#endif // __IPFS_CLIENT_H__
