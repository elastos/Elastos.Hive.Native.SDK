#ifndef __IPFS_CLIENT_H__
#define __IPFS_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ela_hive.h"
#include "http_client.h"

typedef struct ipfs_transaction {
    void (*setup_req)(http_client_t *, void *);
    void *user_data;
    http_client_t *resp;
} ipfs_tsx_t;

int ipfs_client_new(const HiveOptions *, HiveClient **);

int ipfs_client_synchronize(HiveClient *);

#ifdef __cplusplus
}
#endif

#endif // __IPFS_CLIENT_H__
