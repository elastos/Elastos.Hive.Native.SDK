#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include <crystal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <cjson/cJSON.h>
#include <http_client.h>

#include "ela_hive.h"
#include "ipfs_client.h"
#include "ipfs_drive.h"

typedef struct IPFSClient {
    HiveClient base;

    char *uid;
    char *server_addr;
} IPFSClient;

#define CLUSTER_API_PORT (9094)
#define NODE_API_PORT (9095)

static int ipfs_client_get_info_internal(HiveClient *base, char **result);

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
                  client->server_addr, NODE_API_PORT);
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
                  client->server_addr, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        return rc;
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
    char *resp;
    cJSON *json = NULL;
    cJSON *peer_id;
    cJSON *hash;
    int rc;

    rc = ipfs_client_get_info_internal((HiveClient *)client, &resp);
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

    rc = 0;
end:
    if (json)
        cJSON_Delete(json);
    return rc;
}

int ipfs_client_synchronize(HiveClient *base)
{
    IPFSClient *client = (IPFSClient *)base;
    int rc;

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

    rc = ipfs_client_synchronize_intl(client);
    if (rc < 0) {
        // recover back to 'RAW' state.
        hive_set_error(-1);
        return -1;
    }

    vlogI("Hive: This client logined onto Hive IPFS in success");
    return 0;
}

static int ipfs_client_logout(HiveClient *base)
{
    // do we need do some logout stuff here.
    return 0;
}

static int ipfs_client_get_info_internal(HiveClient *base, char **result)
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
                  client->server_addr, NODE_API_PORT);
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

static int ipfs_client_get_info(HiveClient *base, HiveClientInfo *result)
{
    IPFSClient *client = (IPFSClient *)base;
    int rc;

    assert(client);
    assert(result);

    rc = snprintf(result->user_id, sizeof(result->user_id), "%s", client->uid);
    if (rc < 0 || rc >= sizeof(result->user_id))
        return -1;

    rc = snprintf(result->display_name, sizeof(result->display_name), "");
    if (rc < 0 || rc >= sizeof(result->display_name))
        return -1;

    rc = snprintf(result->email, sizeof(result->email), "");
    if (rc < 0 || rc >= sizeof(result->email))
        return -1;

    rc = snprintf(result->phone_number, sizeof(result->phone_number), "");
    if (rc < 0 || rc >= sizeof(result->phone_number))
        return -1;

    rc = snprintf(result->region, sizeof(result->region), "");
    if (rc < 0 || rc >= sizeof(result->region))
        return -1;

    return 0;
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
                  client->server_addr, NODE_API_PORT);
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

static int ipfs_client_drive_open(HiveClient *base, HiveDrive **drive)
{
    HiveDrive *tmp;
    assert(base);

    tmp = ipfs_drive_open(base);
    if (!tmp) {
        // TODO
        return -1;
    }

    *drive = tmp;
    return 0;
}

static int ipfs_client_close(HiveClient *base)
{
    assert(base);

    deref(base);
    return 0;
}

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

static void ipfs_client_destructor(void *p)
{
    IPFSClient *client = (IPFSClient *)p;

    if (client->uid)
        free(client->uid);

    if (client->server_addr)
        free(client->server_addr);
}

int ipfs_client_new(const HiveOptions * options, HiveClient **client)
{
    IPFSOptions *opts = (IPFSOptions *)options;
    IPFSClient *tmp;
    int rc = -1;
    size_t i;

    assert(client);

    if (!opts->uid || !*opts->uid || !opts->bootstraps_size) {
        // TODO:
        return rc;
    }

    for (i = 0; i < opts->bootstraps_size; ++i) {
        rc = test_reachable(opts->bootstraps_ip[i]);
        if (!rc)
            break;
    }
    if (i >= opts->bootstraps_size) {
        // TODO;
        return rc;
    }

    tmp = (IPFSClient *)rc_zalloc(sizeof(IPFSClient), ipfs_client_destructor);
    if (!tmp) {
        // TODO;
        return rc;
    }

    tmp->uid = strdup(opts->uid);
    if (!tmp->uid) {
        deref(tmp);
        // TODO;
        return rc;
    }

    tmp->server_addr = strdup(opts->bootstraps_ip[i]);
    if (!tmp->server_addr) {
        deref(tmp);
        // TODO;
        return rc;
    }

    tmp->base.login      = &ipfs_client_login;
    tmp->base.logout     = &ipfs_client_logout;
    tmp->base.get_info   = &ipfs_client_get_info;
    tmp->base.list_drives= &ipfs_client_list_drives;
    tmp->base.get_drive  = &ipfs_client_drive_open;
    tmp->base.finalize   = &ipfs_client_close;

    tmp->base.invalidate_credential = &ipfs_client_invalidate_credential;

    rc = ipfs_client_get_info_internal(&tmp->base, NULL);
    if (rc) {
        deref(tmp);
        return rc;
    }

    *client = (HiveClient *)tmp;
    return 0;
}
