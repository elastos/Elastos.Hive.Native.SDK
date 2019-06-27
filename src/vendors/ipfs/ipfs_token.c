#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <crystal.h>
#include <cjson/cJSON.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "ela_hive.h"
#include "ipfs_token.h"
#include "ipfs_utils.h"
#include "ipfs_constants.h"
#include "ipfs_drive.h"
#include "http_client.h"

struct ipfs_token {
    char uid[HIVE_MAX_IPFS_UID_LEN + 1];
    char current_node[HIVE_MAX_IPV6_ADDRESS_LEN  + 1];
    ipfs_token_writeback_func_t *writeback_cb;
    void *user_data;
    size_t rpc_nodes_count;
    rpc_node_t rpc_nodes[0];
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
    http_client_set_request_body_instant(httpc, NULL, 0);
    http_client_set_timeout(httpc, 5);

    rc = http_client_request(httpc);
    if (rc)
        goto error_exit;

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);

    if (rc)
        return -1;

    if (resp_code != 405)
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
    http_client_set_request_body_instant(httpc, NULL, 0);
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
    assert(token);

    return _ipfs_token_get_uid_info(token->current_node, token->uid, result);
}

static int select_bootstrap(rpc_node_t *rpc_nodes, size_t nodes_cnt, char *selected)
{
    size_t i;
    size_t base;
    int rc;

    srand((unsigned)time(NULL));
    base = (size_t)rand() % nodes_cnt;
    i = base;

    do {
        if (rpc_nodes[i].ipv4[0]) {
            rc = test_reachable(rpc_nodes[i].ipv4);
            if (!rc) {
                strcpy(selected, rpc_nodes[i].ipv4);
                return 0;
            }
        }

        if (rpc_nodes[i].ipv6[0]) {
            rc = test_reachable(rpc_nodes[i].ipv6);
            if (!rc) {
                strcpy(selected, rpc_nodes[i].ipv6);
                return 0;
            }
        }

        i = (i + 1) % nodes_cnt;
    } while (i != base);

    return -1;
}

int ipfs_token_check_reachable(ipfs_token_t *token)
{
    int rc;

    if (token->current_node[0])
        return 0;

    rc = select_bootstrap(token->rpc_nodes, token->rpc_nodes_count,
                          token->current_node);
    if (rc < 0)
        return -1;

    rc = ipfs_synchronize(token);
    if (rc < 0)
        token->current_node[0] = '\0';

    return rc;
}

void ipfs_token_mark_node_unreachable(ipfs_token_t *token)
{
    token->current_node[0] = '\0';
}

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
    http_client_set_request_body_instant(httpc, NULL, 0);
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
    return token->uid;
}

const char *ipfs_token_get_current_node(ipfs_token_t *token)
{
    return token->current_node;
}

static int load_store(const cJSON *store, char *uid, size_t len)
{
    cJSON *uid_json;

    uid_json = cJSON_GetObjectItemCaseSensitive(store, "uid");
    if (!uid_json || !cJSON_IsString(uid_json) || !uid_json->valuestring ||
        !*uid_json->valuestring || strlen(uid_json->valuestring) >= len)
        return -1;

    strcpy(uid, uid_json->valuestring);
    return 0;
}

static int writeback_tokens(ipfs_token_t *token)
{
    cJSON *json;
    int rc;

    json = cJSON_CreateObject();
    if (!json)
        return -1;

    if (!cJSON_AddStringToObject(json, "uid", token->uid)) {
        cJSON_Delete(json);
        return -1;
    }

    rc = token->writeback_cb(json, token->user_data);
    cJSON_Delete(json);
    return rc;
}

int ipfs_token_reset(ipfs_token_t *token)
{
    return 0;
}

int ipfs_token_synchronize(ipfs_token_t *token)
{
    return ipfs_synchronize(token);
}

ipfs_token_t *ipfs_token_new(ipfs_token_options_t *options,
                             ipfs_token_writeback_func_t cb,
                             void *user_data)
{
    ipfs_token_t *tmp;
    size_t bootstraps_nbytes;
    char url[MAX_URL_LEN] = {0};
    int rc;

    bootstraps_nbytes = sizeof(options->rpc_nodes[0]) * options->rpc_nodes_count;
    tmp = rc_zalloc(sizeof(ipfs_token_t) + bootstraps_nbytes, NULL);
    if (!tmp)
        return NULL;

    memcpy(tmp->rpc_nodes, options->rpc_nodes, bootstraps_nbytes);
    tmp->rpc_nodes_count = options->rpc_nodes_count;
    tmp->writeback_cb    = cb;
    tmp->user_data       = user_data;

    rc = select_bootstrap(options->rpc_nodes, options->rpc_nodes_count,
                          tmp->current_node);
    if (rc < 0) {
        deref(tmp);
        return NULL;
    }

    if (options->uid[0]) {
        rc = _ipfs_token_get_uid_info(tmp->current_node, options->uid, NULL);
        if (rc < 0) {
            deref(tmp);
            return NULL;
        }
        strcpy(tmp->uid, options->uid);
    } else if (options->store) {
        rc = load_store(options->store, tmp->uid, sizeof(tmp->uid));
        if (rc < 0) {
            deref(tmp);
            return NULL;
        }

        rc = _ipfs_token_get_uid_info(tmp->current_node, tmp->uid, NULL);
        if (rc < 0) {
            deref(tmp);
            return NULL;
        }
    } else {
        rc = uid_new(tmp->current_node, tmp->uid, sizeof(tmp->uid));
        if (rc < 0) {
            deref(tmp);
            return NULL;
        }

        rc = publish_root_hash(tmp, url, sizeof(url));
        if (rc < 0) {
            deref(tmp);
            return NULL;
        }
    }

    writeback_tokens(tmp);

    return tmp;
}

int ipfs_token_close(ipfs_token_t *token)
{
    deref(token);
    return 0;
}
