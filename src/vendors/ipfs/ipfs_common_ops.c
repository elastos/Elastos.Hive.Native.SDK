#include <stdlib.h>
#include <assert.h>
#include <crystal.h>
#include <cjson/cJSON.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "ipfs_common_ops.h"
#include "ipfs_constants.h"
#include "http_client.h"
#include "hive_error.h"

static int ipfs_resolve(ipfs_token_t *token, const char *peerid, char **result)
{
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    char *p;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/name/resolve",
                  ipfs_token_get_current_node(token), NODE_API_PORT);
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
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/uid/login",
                  ipfs_token_get_current_node(token), NODE_API_PORT);
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
    http_client_set_query(httpc, "uid", ipfs_token_get_uid(token));
    http_client_set_query(httpc, "hash", hash);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_request_body_instant(httpc, NULL, 0);

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

int ipfs_stat_file(ipfs_token_t *token, const char *file_path, HiveFileInfo *info)
{
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    cJSON *response;
    cJSON *hash;
    long resp_code;
    char *p;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/files/stat",
                  ipfs_token_get_current_node(token), NODE_API_PORT);
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
    http_client_set_query(httpc, "uid", ipfs_token_get_uid(token));
    http_client_set_query(httpc, "path", file_path);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_request_body_instant(httpc, NULL, 0);
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

    response = cJSON_Parse(p);
    free(p);
    if (!response)
        return -1;

    hash = cJSON_GetObjectItemCaseSensitive(response, "Hash");
    if (!hash || !cJSON_IsString(hash) || !hash->valuestring ||
        !*hash->valuestring || strlen(hash->valuestring) >= sizeof(info->fileid)) {
        cJSON_Delete(response);
        return -1;
    }

    rc = snprintf(info->fileid, sizeof(info->fileid), "/ipfs/%s", hash->valuestring);
    cJSON_Delete(response);
    if (rc < 0 || rc >= sizeof(info->fileid))
        return -1;
    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

int ipfs_publish(ipfs_token_t *token, const char *path)
{
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    HiveFileInfo info;
    long resp_code;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/name/publish",
                  ipfs_token_get_current_node(token), NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    rc = ipfs_stat_file(token, path, &info);
    if (rc < 0)
        return -1;

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", ipfs_token_get_uid(token));
    http_client_set_query(httpc, "path", info.fileid);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_request_body_instant(httpc, NULL, 0);

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
