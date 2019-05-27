#ifndef __HIVE_IPFS_CLIENT_H__
#define __HIVE_IPFS_CLIENT_H__

#include "client.h"
#include "http_client.h"

typedef struct ipfs_transaction {
    void (*setup_req)(http_client_t *, void *);
    void *user_data;
    http_client_t *resp;
} ipfs_tsx_t;

HiveClient *hiveipfs_client_new(const HiveOptions *);

int ipfs_client_synchronize(HiveClient *);

#endif // __HIVE_IPFS_CLIENT_H__
