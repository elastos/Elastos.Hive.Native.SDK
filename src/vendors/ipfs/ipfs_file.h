#ifndef __HIVE_IPFS_FILE_H__
#define __HIVE_IPFS_FILE_H__

#include "ipfs_token.h"

int ipfs_file_open(ipfs_token_t *token, const char *path, int flags, HiveFile **file);

#endif // __HIVE_IPFS_FILE_H__
