#ifndef __HIVEIPFS_DRIVE_H__
#define __HIVEIPFS_DRIVE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ela_hive.h"
#include "ipfs_rpc.h"

HiveDrive *ipfs_drive_open(ipfs_rpc_t *rpc);

#ifdef __cplusplus
}
#endif

#endif // __HIVEIPFS_DRIVE_H__
