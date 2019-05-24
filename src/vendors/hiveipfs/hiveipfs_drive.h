#ifndef __HIVEIPFS_DRIVE_H__
#define __HIVEIPFS_DRIVE_H__

#include "drive.h"
#include "client.h"

HiveDrive *ipfs_drive_open(HiveClient *ipfs_client, const char *svr_addr, const char *uid);

#endif // __HIVEIPFS_DRIVE_H__
