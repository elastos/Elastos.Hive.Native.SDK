#ifndef __IPFS_TOKEN_H__
#define __IPFS_TOKEN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <cjson/cJSON.h>

#include "ela_hive.h"
#include "ipfs_constants.h"

typedef struct ipfs_token ipfs_token_t;

typedef struct rpc_node {
    char ipv4[HIVE_MAX_IPV4_ADDRESS_LEN  + 1];
    char ipv6[HIVE_MAX_IPV6_ADDRESS_LEN  + 1];
    uint16_t port;
} rpc_node_t;

typedef struct ipfs_token_options {
    cJSON *store;
    char uid[HIVE_MAX_IPFS_UID_LEN + 1];
    size_t rpc_nodes_count;
    rpc_node_t rpc_nodes[0];
} ipfs_token_options_t;

typedef int ipfs_token_writeback_func_t(const cJSON *json, void *user_data);

ipfs_token_t *ipfs_token_new(ipfs_token_options_t *options,
                             ipfs_token_writeback_func_t cb,
                             void *user_data);
int ipfs_token_close(ipfs_token_t *token);
int ipfs_token_synchronize(ipfs_token_t *token);
int ipfs_token_reset(ipfs_token_t *token);
int ipfs_token_get_uid_info(ipfs_token_t *token, char **result);
const char *ipfs_token_get_uid(ipfs_token_t *token);
const char *ipfs_token_get_current_node(ipfs_token_t *token);

#ifdef __cplusplus
}
#endif

#endif // __IPFS_TOKEN_H__
