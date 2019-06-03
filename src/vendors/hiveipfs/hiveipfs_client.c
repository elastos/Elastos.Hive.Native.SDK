#include <stdio.h>
#include <stddef.h>
#include <pthread.h>
#include <crystal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <http_client.h>
#include <stdlib.h>
#include <cjson/cJSON.h>

#include "hiveipfs_client.h"
#include "hiveipfs_drive.h"

enum {
    RAW          = (int)0,    // The primitive state.
    LOGINING     = (int)1,    // Being in the middle of logining.
    LOGINED      = (int)2,    // Already being logined.
    LOGOUTING    = (int)3,    // Being in the middle of logout.
};

typedef struct IPFSClient {
    HiveClient base;

    int login_state;

    char *uid;
    char *server_addr;

    pthread_mutex_t lock;
    pthread_cond_t cond;
    int state;
    char *svr_addr;
} IPFSClient;

#define CLUSTER_API_PORT (9094)
#define NODE_API_PORT (9095)

static int ipfs_client_get_info(HiveClient *obj, char **result);

static
int ipfs_client_resolve(HiveClient *base, const char *peerid, char **result)
{
    IPFSClient *client = (IPFSClient *)base;
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    char *p;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/name/resolve",
                  client->svr_addr, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "arg", peerid);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0 || resp_code != 200) {
        hive_set_error(-1);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p) {
        hive_set_error(-1);
        return -1;
    }

    *result = p;
    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static int ipfs_client_login_internal(HiveClient *base, const char *hash)
{
    IPFSClient *client = (IPFSClient *)base;
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/uid/login",
                  client->svr_addr, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", client->uid);
    http_client_set_query(httpc, "hash", hash);
    http_client_set_method(httpc, HTTP_METHOD_POST);

    rc = http_client_request(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);

    if (rc < 0) {
        hive_set_error(-1);
        return -1;
    }

    if (resp_code != 200) {
        hive_set_error(-1);
        return -1;
    }

    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static int ipfs_client_synchronize_intl(IPFSClient *client)
{

    char url[MAXPATHLEN + 1];
    long resp_code;
    char *resp;
    cJSON *json = NULL;
    cJSON *peer_id;
    cJSON *hash;
    http_client_t *http_client = NULL;
    int rc;
    int ret = -1;

    rc = ipfs_client_get_info((HiveClient *)client, &resp);
    if (rc)
        goto end;

    json = cJSON_Parse(resp);
    free(resp);
    if (!json)
        goto end;

    peer_id = cJSON_GetObjectItemCaseSensitive(json, "PeerID");
    if (!cJSON_IsString(peer_id) || !peer_id->valuestring || !*peer_id->valuestring)
        goto end;

    rc = ipfs_client_resolve(&client->base, peer_id->valuestring, &resp);
    if (rc)
        goto end;
    cJSON_Delete(json);

    json = cJSON_Parse(resp);
    free(resp);
    if (!json)
        goto end;

    hash = cJSON_GetObjectItemCaseSensitive(json, "Path");
    if (!cJSON_IsString(hash) || !hash->valuestring || !*hash->valuestring)
        goto end;

    rc = ipfs_client_login_internal(&client->base, hash->valuestring);
    if (rc < 0)
        goto end;

    ret = 0;
end:
    if (json)
        cJSON_Delete(json);
    if (http_client)
        http_client_close(http_client);
    return ret;
}

int ipfs_client_synchronize(HiveClient *base)
{
    IPFSClient *client = (IPFSClient *)base;
    int rc;

    rc = __sync_val_compare_and_swap(&client->login_state, LOGINED, LOGINED);
    if (rc != LOGINED) {
        hive_set_error(-1);
        return -1;
    }

    rc = ipfs_client_synchronize_intl(client);
    if (rc < 0) {
        hive_set_error(-1);
        return -1;
    }

    return 0;
}

static int ipfs_client_login(HiveClient *base)
{
    IPFSClient *client = (IPFSClient *)base;
    int rc;

    assert(client);

    /*
     * Check login state.
     * 1. If already logined, return OK immediately, else
     * 2. if being in progress of logining, then return error. else
     * 3. It's in raw state, conduct the login process.
     */
    rc = __sync_val_compare_and_swap(&client->login_state, RAW, LOGINING);
    switch(rc) {
    case RAW:
        break; // need to proceed the login routine.

    case LOGINING:
    case LOGOUTING:
    default:
        hive_set_error(-1);
        return -1;

    case LOGINED:
        vlogD("Hive: This client already logined onto Hive IPFS");
        return 0;
    }

    rc = ipfs_client_synchronize_intl(client);
    if (rc < 0) {
        // recover back to 'RAW' state.
        __sync_val_compare_and_swap(&client->login_state, LOGINING, RAW);
        hive_set_error(-1);
        return -1;
    }

    // When conducting all login stuffs successfully, then change to be
    // 'LOGINED'.
    __sync_val_compare_and_swap(&client->login_state,  LOGINING, LOGINED);
    vlogI("Hive: This client logined onto Hive IPFS in success");
    return 0;
}

static int ipfs_client_logout(HiveClient *base)
{
    IPFSClient *client = (IPFSClient *)base;
    int rc;

    rc = __sync_val_compare_and_swap(&client->login_state, LOGINED, LOGOUTING);
    switch(rc) {
    case RAW:
        return 0;

    case LOGINING:
    case LOGOUTING:
    default:
        hive_set_error(-1);
        return -1;

    case LOGINED:
        break;
    }

    // do we need do some logout stuff here.

    __sync_val_compare_and_swap(&client->login_state, LOGOUTING, RAW);
    return 0;
}

static int ipfs_client_get_info(HiveClient *base, char **result)
{
    IPFSClient *client = (IPFSClient *)base;
    char url[MAXPATHLEN + 1];
    http_client_t *httpc;
    long resp_code = 0;
    char *p;
    int rc;

    assert(client);
    assert(result);

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/uid/info",
                  client->svr_addr, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", client->uid);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc || resp_code != 200) {
        hive_set_error(-1);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);
    if (!p) {
        hive_set_error(-1);
        return -1;
    }

    *result = p;
    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static int ipfs_client_list_drives(HiveClient *base, char **result)
{
    IPFSClient *client = (IPFSClient *)base;
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    char *p;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/files/stat",
                  client->svr_addr, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", client->uid);
    http_client_set_query(httpc, "path", "/");
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (resp_code != 200) {
        hive_set_error(-1);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p) {
        hive_set_error(-1);
        return -1;
    }

    *result = p;
    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static HiveDrive *ipfs_client_drive_open(HiveClient *base, const HiveDriveOptions *opts)
{
    IPFSClient *client = (IPFSClient *)base;

    assert(client);
    (void)opts;

    return ipfs_drive_open(base);
}

static int ipfs_client_close(HiveClient *base)
{
    assert(base);

    deref(base);
    return 0;
}

/*
static int ipfs_client_perform_tsx(HiveClient *obj, client_tsx_t *base)
{
    IPFSClient *client = (IPFSClient *)obj;
    ipfs_tsx_t *tsx = (ipfs_tsx_t *)base;
    int rc;
    http_client_t *http_client;
    char url[MAXPATHLEN + 1];

    rc = __sync_val_compare_and_swap(&client->login_state, LOGINED, LOGINED);
    if (rc != LOGINED) {
        hive_set_error(-1);
        return -1;
    }

    rc = snprintf(url, sizeof(url), "http://%s:%d", client->svr_addr, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    http_client = http_client_new();
    if (!http_client)
        return -1;

    http_client_set_url(http_client, url);
    http_client_set_query(http_client, "uid", client->uid);
    tsx->setup_req(http_client, tsx->user_data);

    rc = http_client_request(http_client);
    if (rc) {
        http_client_close(http_client);
        return -1;
    }

    tsx->resp = http_client;
    return 0;
}
*/

static int ipfs_client_invalidate_credential(HiveClient *obj)
{
    (void)obj;
    return 0;
}

static int test_reachable(const char *ipaddr)
{
    char url[MAXPATHLEN + 1];
    http_client_t *httpc;
    int rc;
    long resp_code;

    rc = snprintf(url, sizeof(url), "http://%s:%d/version",
                  ipaddr, CLUSTER_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    http_client_set_url(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_POST);

    rc = http_client_request(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);

    if (rc < 0) {
        hive_set_error(-1);
        return -1;
    }

    if (resp_code != 200) {
        hive_set_error(-1);
        return -1;
    }

    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static void hiveipfs_destructor(void *p)
{
    IPFSClient *client = (IPFSClient *)p;

    if (client->uid)
        free(client->uid);

    if (client->svr_addr)
        free(client->svr_addr);

    pthread_mutex_destroy(&client->lock);
    pthread_cond_destroy(&client->cond);
}

HiveClient *hiveipfs_client_new(const HiveOptions * options)
{
    HiveIpfsOptions *opts = (HiveIpfsOptions *)options;
    IPFSClient *client;
    int rc;
    int i;

    if (!opts->uid || !*opts->uid || !opts->bootstraps_size) {
        hive_set_error(-1);
        return NULL;
    }

    for (i = 0; i < opts->bootstraps_size; ++i) {
        rc = test_reachable(opts->bootstraps_ip[i]);
        if (!rc)
            break;
    }
    if (i == opts->bootstraps_size) {
        hive_set_error(-1);
        return NULL;
    }

    client = (IPFSClient *)rc_zalloc(sizeof(IPFSClient), &hiveipfs_destructor);
    if (!client) {
        hive_set_error(-1);
        return NULL;
    }

    client->uid = strdup(opts->uid);
    if (!client->uid) {
        hive_set_error(-1);
        deref(client);
        return NULL;
    }

    client->svr_addr = strdup(opts->bootstraps_ip[i]);
    if (!client->svr_addr) {
        hive_set_error(-1);
        deref(client);
        return NULL;
    }

    pthread_mutex_init(&client->lock, NULL);
    pthread_cond_init(&client->cond, NULL);

    client->base.login                 = &ipfs_client_login;
    client->base.logout                = &ipfs_client_logout;
    client->base.get_info              = &ipfs_client_get_info;
    client->base.list_drives           = &ipfs_client_list_drives;
    client->base.drive_open            = &ipfs_client_drive_open;
    client->base.finalize              = &ipfs_client_close;

    client->base.invalidate_credential = &ipfs_client_invalidate_credential;

    rc = ipfs_client_get_info(&client->base, NULL);
    if (rc) {
        deref(client);
        return NULL;
    }

    return &client->base;
}
