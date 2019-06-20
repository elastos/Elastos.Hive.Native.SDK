#ifndef __COMMON_OPS_H__
#define __COMMON_OPS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ipfs_token.h"

int ipfs_synchronize(ipfs_token_t *token);
int ipfs_publish(ipfs_token_t *token, const char *path);
int ipfs_stat_file(ipfs_token_t *token, const char *file_path, HiveFileInfo *info);

static inline bool is_string_item(cJSON *item)
{
    return cJSON_IsString(item) && item->valuestring && *item->valuestring;
}

int publish_root_hash(ipfs_token_t *token, char *buf, size_t length);

#ifdef __cplusplus
}
#endif

#endif // __COMMON_OPS_H__
