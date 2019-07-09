#ifndef __HIVE_IPFS_FILE_H__
#define __HIVE_IPFS_FILE_H__

#include "ipfs_rpc.h"

int ipfs_file_open(ipfs_rpc_t *rpc, const char *path, int flags, HiveFile **file);

#endif // __HIVE_IPFS_FILE_H__
