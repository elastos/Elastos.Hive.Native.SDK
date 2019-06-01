#ifndef __ONEDRIVE_CLIENT_H__
#define __ONEDRIVE_CLIENT_H__

#include "client.h"
#include "http_client.h"

typedef struct onedrive_transaction {
    void (*setup_req)(http_client_t *, void *);
    void *user_data;
    http_client_t *resp;
} onedrv_tsx_t;

typedef struct HttpRequestAux {
    void (*prepare_cb)(http_client_t *, void *);
    void *user_data;
    http_client_t *resp;
} HttpRequestAux;

HiveClient *onedrive_client_new(const HiveOptions *);

#endif // __ONEDRIVE_CLIENT_H__
