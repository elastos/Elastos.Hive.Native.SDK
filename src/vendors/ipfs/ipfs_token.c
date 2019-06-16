#include <stddef.h>
#include <crystal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <string.h>
#include <cjson/cJSON.h>
#include <stdlib.h>

#include "ela_hive.h"
#include "ipfs_token.h"
#include "ipfs_constants.h"
#include "http_client.h"

struct ipfs_token {
    size_t bootstrap_in_use;
    ipfs_token_options_t options;
};

static int test_reachable(const char *ipaddr)
{
    char url[MAXPATHLEN + 1];
    http_client_t *httpc;
    int rc;
    long resp_code;

    rc = snprintf(url, sizeof(url), "http://%s:%d/version", ipaddr, CLUSTER_API_PORT);
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    httpc = http_client_new();
    if (!httpc)
        return -1;

    http_client_set_url(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_POST);

    rc = http_client_request(httpc);
    if (rc < 0)
        goto error_exit;

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

static int _ipfs_token_get_uid_info(const char *node_ip,
                                    const char *uid,
                                    char **result)
{
    char url[MAXPATHLEN + 1];
    http_client_t *httpc;
    long resp_code = 0;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/uid/info",
                  node_ip, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    httpc = http_client_new();
    if (!httpc)
        return -1;

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", uid);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    if (result)
        http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc)
        goto error_exit;

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc || resp_code != 200)
        goto error_exit;

    if (result) {
        char *p;

        p = http_client_move_response_body(httpc, NULL);
        if (!p)
            goto error_exit;

        *result = p;
    }

    http_client_close(httpc);
    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

int ipfs_token_get_uid_info(ipfs_token_t *token, char **result)
{
    return -1;
    /* assert(token);

    return _ipfs_token_get_uid_info(token->options.bootstraps_ip[token->bootstrap_in_use],
                                    token->options.uid, result); */
}

/* static int get_usable_bootstrap(size_t bootstraps_size,
                                char bootstraps_ip[][HIVE_MAX_IP_STRING_LEN+1],
                                size_t *usable_bootstrap_idx)
{
    size_t i;
    int rc;

    for (i = 0; i < bootstraps_size; ++i) {
        rc = test_reachable(bootstraps_ip[i]);
        if (!rc) {
            *usable_bootstrap_idx = i;
            return 0;
        }
    }
    return -1;
} */

static int uid_new(const char *node_ip, char *uid, size_t uid_len)
{
    char url[MAXPATHLEN + 1];
    http_client_t *httpc;
    long resp_code = 0;
    cJSON *uid_json;
    cJSON *json;
    char *p;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/uid/new",
                  node_ip, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    httpc = http_client_new();
    if (!httpc)
        return -1;

    http_client_set_url(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc)
        goto error_exit;

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc || resp_code != 200)
        goto error_exit;

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);
    if (!p)
        return -1;

    json = cJSON_Parse(p);
    free(p);
    if (!json)
        return -1;

    uid_json = cJSON_GetObjectItemCaseSensitive(json, "UID");
    if (!cJSON_IsString(uid_json) || !uid_json->valuestring || !*uid_json->valuestring) {
        cJSON_Delete(json);
        return -1;
    }

    rc = snprintf(uid, uid_len, "%s", uid_json->valuestring);
    cJSON_Delete(json);
    if (rc < 0 || rc >= uid_len)
        return -1;

    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

const char *ipfs_token_get_uid(ipfs_token_t *token)
{
    return NULL;
    /* int rc;

    assert(token);
    assert(uid);
    assert(uid_len);

    rc = snprintf(uid, uid_len, "%s", token->options.uid);
    assert(rc > 0 && rc < uid_len); */
}

const char *ipfs_token_get_node_in_use(ipfs_token_t *token)
{
    return NULL;
    /* int rc;

    assert(token);
    assert(node_ip);
    assert(node_ip_len);

    rc = snprintf(node_ip, node_ip_len, "%s",
                  token->options.bootstraps_ip[token->bootstrap_in_use]);
    assert(rc > 0 && rc < node_ip_len); */
}

ipfs_token_t *ipfs_token_new(ipfs_token_options_t *options,
                             ipfs_token_writeback_func_t cb,
                             void *user_data)
{
    return NULL;
    /* size_t candidate_bootstrap;
    ipfs_token_t *tmp;
    size_t bootstraps_nbytes;
    int rc;

    rc = get_usable_bootstrap(bootstraps_size, bootstraps_ip, &candidate_bootstrap);
    if (rc < 0)
        return -1;

    if (uid) {
        rc = _ipfs_token_get_uid_info(bootstraps_ip[candidate_bootstrap], uid, NULL);
        if (rc < 0)
            return -1;
    } else {
        size_t uid_max_len = HIVE_MAX_USER_ID_LEN + 1;
        uid = alloca(uid_max_len);

        rc = uid_new(bootstraps_ip[candidate_bootstrap], (char *)uid, uid_max_len);
        if (rc < 0)
            return -1;
    }

    bootstraps_nbytes = sizeof(bootstraps_ip[0]) * bootstraps_size;
    tmp = rc_zalloc(sizeof(ipfs_token_t) + bootstraps_nbytes, NULL);
    if (!tmp)
        return -1;

    tmp->bootstrap_in_use = candidate_bootstrap;
    strcpy(tmp->options.uid, uid);
    tmp->options.bootstraps_size = bootstraps_size;
    memcpy(tmp->options.bootstraps_ip, bootstraps_ip, bootstraps_nbytes);

    *token = tmp;
    return 0; */
}

void ipfs_token_reset(ipfs_token_t *token)
{
}

int ipfs_token_close(ipfs_token_t *token)
{
    deref(token);
    return 0;
}
