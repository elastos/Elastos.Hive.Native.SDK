#ifndef __COMMON_OPS_H__
#define __COMMON_OPS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ipfs_rpc.h"

int ipfs_synchronize(ipfs_rpc_t *rpc);
int ipfs_publish(ipfs_rpc_t *rpc, const char *path);
int ipfs_stat_file(ipfs_rpc_t *rpc, const char *file_path, HiveFileInfo *info);

static inline bool is_string_item(cJSON *item)
{
    return cJSON_IsString(item) && item->valuestring && *item->valuestring;
}

int publish_root_hash(ipfs_rpc_t *rpc, char *buf, size_t length);

#ifdef __cplusplus
}
#endif

#endif // __COMMON_OPS_H__
