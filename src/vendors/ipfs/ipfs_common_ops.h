#ifndef __COMMON_OPS_H__
#define __COMMON_OPS_H__

#include "ipfs_token.h"

int ipfs_synchronize(ipfs_token_t *token);
int ipfs_publish(ipfs_token_t *token, const char *path);

#endif // __COMMON_OPS_H__
