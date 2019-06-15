#ifndef __IPFS_TOKEN_H__
#define __IPFS_TOKEN_H__

#include "ela_hive.h"

typedef struct ipfs_token ipfs_token_t;

int ipfs_token_new(const char *uid, size_t bootstraps_size,
                   char bootstraps_ip[][HIVE_MAX_IP_STRING_LEN+1],
                   ipfs_token_t **token);
int ipfs_token_close(ipfs_token_t *token);
int ipfs_token_get_uid_info(ipfs_token_t *token, char **result);
void ipfs_token_get_uid(ipfs_token_t *token, char *uid, size_t uid_len);
void ipfs_token_get_node_in_use(ipfs_token_t *token, char *node_ip, size_t node_ip_len);

#endif // __IPFS_TOKEN_H__
