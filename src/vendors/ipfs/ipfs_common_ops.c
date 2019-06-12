#include <cjson/cJSON.h>
#include <stdlib.h>
#include <assert.h>
#include <crystal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "ipfs_common_ops.h"
#include "ipfs_constants.h"
#include "http_client.h"
#include "error.h"

static int ipfs_resolve(ipfs_token_t *token, const char *peerid, char **result)
{
    char node_ip[HIVE_MAX_IP_STRING_LEN+1];
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    char *p;
    int rc;

    ipfs_token_get_node_in_use(token, node_ip, sizeof(node_ip));
    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/name/resolve",
                  node_ip, NODE_API_PORT);
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

static int ipfs_login(ipfs_token_t *token, const char *hash)
{
    char node_ip[HIVE_MAX_IP_STRING_LEN+1];
    char uid[HIVE_MAX_USER_ID_LEN + 1];
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    int rc;

    ipfs_token_get_node_in_use(token, node_ip, sizeof(node_ip));
    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/uid/login",
                  node_ip, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    ipfs_token_get_uid(token, uid, sizeof(uid));

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", uid);
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

int ipfs_synchronize(ipfs_token_t *token)
{
    char *resp;
    cJSON *json = NULL;
    cJSON *peer_id;
    cJSON *hash;
    int rc;

    assert(token);

    rc = ipfs_token_get_uid_info(token, &resp);
    if (rc)
        return -1;

    json = cJSON_Parse(resp);
    free(resp);
    if (!json)
        return -1;

    peer_id = cJSON_GetObjectItemCaseSensitive(json, "PeerID");
    if (!cJSON_IsString(peer_id) || !peer_id->valuestring || !*peer_id->valuestring) {
        cJSON_Delete(json);
        return -1;
    }

    rc = ipfs_resolve(token, peer_id->valuestring, &resp);
    cJSON_Delete(json);
    if (rc)
        return -1;

    json = cJSON_Parse(resp);
    free(resp);
    if (!json)
        return -1;

    hash = cJSON_GetObjectItemCaseSensitive(json, "Path");
    if (!cJSON_IsString(hash) || !hash->valuestring || !*hash->valuestring) {
        cJSON_Delete(json);
        return -1;
    }

    rc = ipfs_login(token, hash->valuestring);
    cJSON_Delete(json);
    if (rc < 0)
        return -1;

    return 0;
}

int ipfs_publish(ipfs_token_t *token, const char *path)
{
    char node_ip[HIVE_MAX_IP_STRING_LEN + 1];
    char uid[HIVE_MAX_USER_ID_LEN + 1];
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code;
    int rc;

    ipfs_token_get_node_in_use(token, node_ip, sizeof(node_ip));
    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/name/publish",
                  node_ip, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    ipfs_token_get_uid(token, uid, sizeof(uid));

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", uid);
    http_client_set_query(httpc, "path", path);
    http_client_set_method(httpc, HTTP_METHOD_POST);

    rc = http_client_request(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    if (rc < 0)
        return -1;

    if (resp_code != 200)
        return -1;

    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}
