#ifndef __HIVEIPFS_DRIVE_H__
#define __HIVEIPFS_DRIVE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ela_hive.h"
#include "ipfs_token.h"

HiveDrive *ipfs_drive_open(ipfs_token_t *token);

#ifdef __cplusplus
}
#endif

#endif // __HIVEIPFS_DRIVE_H__
