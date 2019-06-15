#ifndef __IPFS_CLIENT_H__
#define __IPFS_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ela_hive.h"
#include "http_client.h"

typedef struct IPFSTokenOptions {
    char uid[HIVE_MAX_USER_ID_LEN+1];
    size_t bootstraps_size;
    char bootstraps_ip[0][HIVE_MAX_IP_STRING_LEN+1];
} IPFSTokenOptions;

typedef struct IPFSOptionsDeprecated {
    HiveOptions base;
    IPFSTokenOptions token_options;
} IPFSOptionsDeprecated;

HiveClient *ipfs_client_new(const HiveOptions *);

int ipfs_client_synchronize(HiveClient *);
#ifdef __cplusplus
}
#endif

#endif // __IPFS_CLIENT_H__
