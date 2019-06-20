#ifndef __COMMON_OPS_H__
#define __COMMON_OPS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ipfs_token.h"

int ipfs_synchronize(ipfs_token_t *token);
int ipfs_publish(ipfs_token_t *token, const char *path);
int ipfs_stat_file(ipfs_token_t *token, const char *file_path, HiveFileInfo *info);

#ifdef __cplusplus
}
#endif

#endif // __COMMON_OPS_H__
