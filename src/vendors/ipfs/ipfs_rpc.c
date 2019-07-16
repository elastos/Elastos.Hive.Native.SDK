/*
 * Copyright (c) 2019 Elastos Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <crystal.h>
#include <cjson/cJSON.h>

#include "ela_hive.h"
#include "hive_error.h"
#include "ipfs_rpc.h"
#include "ipfs_utils.h"
#include "ipfs_constants.h"
#include "ipfs_drive.h"
#include "http_client.h"
#include "http_status.h"

struct ipfs_rpc {
    char uid[HIVE_MAX_IPFS_UID_LEN + 1];
    char current_node[HIVE_MAX_IPV6_ADDRESS_LEN  + 1];
    ipfs_rpc_writeback_func_t *writeback_cb;
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
    if (rc < 0 || rc >= sizeof(url)) {
        vlogE("IpfsToken: URL too long.");
        return HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);
    }

    httpc = http_client_new();
    if (!httpc) {
        vlogE("IpfsToken: failed to create http client instance.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    http_client_set_url(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_request_body_instant(httpc, NULL, 0);
    http_client_set_timeout(httpc, 5);

    rc = http_client_request(httpc);
    if (rc) {
        rc = HIVE_CURL_ERROR(rc);
        vlogE("IpfsToken: failed to perform http request.");
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);

    if (rc) {
        vlogE("IpfsToken: failed to get http response code.");
        return HIVE_CURL_ERROR(rc);
    }

    if (resp_code != HttpStatus_MethodNotAllowed) {
        vlogE("IpfsToken: error from response (%d).", resp_code);
        return HIVE_HTTP_STATUS_ERROR(resp_code);
    }

    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static int _ipfs_token_get_uid_info(const char *node_ip,
                                    const char *uid,
                                    char **result)
{
    char url[MAXPATHLEN + 1];
    http_client_t *httpc;
    long resp_code = 0;
    int rc;

    vlogD("IpfsToken: Calling _ipfs_token_get_uid_info().");

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/uid/info",
                  node_ip, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        vlogE("IpfsToken: URL too long.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    httpc = http_client_new();
    if (!httpc) {
        vlogE("IpfsToken: failed to create http client instance.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", uid);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_request_body_instant(httpc, NULL, 0);
    if (result)
        http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc) {
        rc = HIVE_CURL_ERROR(rc);
        vlogE("IpfsToken: failed to perform http request.");
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc)  {
        rc = HIVE_CURL_ERROR(rc);
        vlogE("IpfsToken: failed to get http response code.");
        goto error_exit;
    }

    if (resp_code != HttpStatus_OK) {
        rc = HIVE_HTTP_STATUS_ERROR(resp_code);
        vlogE("IpfsToken: error from http response (%d).", resp_code);
        goto error_exit;
    }

    if (result) {
        char *p;

        p = http_client_move_response_body(httpc, NULL);
        if (!p) {
            rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
            vlogE("IpfsToken: failed to get http response body.");
            goto error_exit;
        }

        *result = p;
    }

    http_client_close(httpc);
    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

int ipfs_rpc_get_uid_info(ipfs_rpc_t *rpc, char **result)
{
    assert(rpc);

    return _ipfs_token_get_uid_info(rpc->current_node, rpc->uid, result);
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
                vlogI("IpfsToken: node selected: %s.", selected);
                return 0;
            }
        }

        if (rpc_nodes[i].ipv6[0]) {
            rc = test_reachable(rpc_nodes[i].ipv6);
            if (!rc) {
                strcpy(selected, rpc_nodes[i].ipv6);
                vlogI("IpfsToken: node selected: %s.", selected);
                return 0;
            }
        }

        i = (i + 1) % nodes_cnt;
    } while (i != base);

    vlogE("IpfsToken: No node configured is reachable.");
    return HIVE_GENERAL_ERROR(HIVEERR_BAD_BOOTSTRAP_HOST);
}

int ipfs_rpc_check_reachable(ipfs_rpc_t *rpc)
{
    int rc;

    if (rpc->current_node[0])
        return 0;

    rc = select_bootstrap(rpc->rpc_nodes, rpc->rpc_nodes_count,
                          rpc->current_node);
    if (rc < 0) {
        vlogE("IpfsToken: no node configured is reachable.");
        return rc;
    }

    rc = ipfs_synchronize(rpc);
    if (rc < 0)
        rpc->current_node[0] = '\0';

    return rc;
}

void ipfs_rpc_mark_node_unreachable(ipfs_rpc_t *rpc)
{
    rpc->current_node[0] = '\0';
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
    if (rc < 0 || rc >= sizeof(url)) {
        vlogE("IpfsToken: URL too long.");
        return HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);
    }

    httpc = http_client_new();
    if (!httpc) {
        vlogE("IpfsToken: failed to create http client instance.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    http_client_set_url(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_request_body_instant(httpc, NULL, 0);
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc) {
        rc = HIVE_CURL_ERROR(rc);
        vlogE("IpfsToken: failed to perform http request.");
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc) {
        rc = HIVE_CURL_ERROR(rc);
        vlogE("IpfsToken: failed to get http response code.");
        goto error_exit;
    }

    if (resp_code != HttpStatus_OK) {
        rc = HIVE_HTTP_STATUS_ERROR(resp_code);
        vlogE("IpfsToken: error from http response (%d).", resp_code);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);
    if (!p) {
        vlogE("IpfsToken: failed to get http response body.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    json = cJSON_Parse(p);
    free(p);
    if (!json) {
        vlogE("IpfsToken: invalid json format for http response.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    uid_json = cJSON_GetObjectItemCaseSensitive(json, "UID");
    if (!cJSON_IsString(uid_json) || !uid_json->valuestring || !*uid_json->valuestring) {
        vlogE("IpfsToken: missing UID json object for http response body.");
        cJSON_Delete(json);
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
    }

    rc = snprintf(uid, uid_len, "%s", uid_json->valuestring);
    cJSON_Delete(json);
    if (rc < 0 || rc >= uid_len) {
        vlogE("IpfsToken: uid length too long.");
        return HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);
    }

    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

const char *ipfs_rpc_get_uid(ipfs_rpc_t *rpc)
{
    return rpc->uid;
}

const char *ipfs_rpc_get_current_node(ipfs_rpc_t *rpc)
{
    return rpc->current_node;
}

static int load_store(const cJSON *store, char *uid, size_t len)
{
    cJSON *uid_json;

    vlogD("IpfsToken: Calling load_store().");

    uid_json = cJSON_GetObjectItemCaseSensitive(store, "uid");
    if (!uid_json || !cJSON_IsString(uid_json) || !uid_json->valuestring ||
        !*uid_json->valuestring || strlen(uid_json->valuestring) >= len) {
        vlogE("IpfsToken: uid length too long.");
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
    }

    strcpy(uid, uid_json->valuestring);
    return 0;
}

static int writeback_tokens(ipfs_rpc_t *rpc)
{
    cJSON *json;
    int rc;

    json = cJSON_CreateObject();
    if (!json) {
        vlogE("IpfsToken: failed to create json object.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    if (!cJSON_AddStringToObject(json, "uid", rpc->uid)) {
        vlogE("IpfsToken: failed to add uid json object.");
        cJSON_Delete(json);
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    rc = rpc->writeback_cb(json, rpc->user_data);
    if (rc < 0)
        vlogE("IpfsToken: failed to write to file.");
    cJSON_Delete(json);
    return rc;
}

int ipfs_rpc_reset(ipfs_rpc_t *rpc)
{
    (void)rpc;
    return 0;
}

int ipfs_rpc_synchronize(ipfs_rpc_t *rpc)
{
    return ipfs_synchronize(rpc);
}

ipfs_rpc_t *ipfs_rpc_new(ipfs_rpc_options_t *options,
                         ipfs_rpc_writeback_func_t cb,
                         void *user_data)
{
    ipfs_rpc_t *tmp;
    size_t bootstraps_nbytes;
    char url[MAX_URL_LEN] = {0};
    int rc;

    bootstraps_nbytes = sizeof(options->rpc_nodes[0]) * options->rpc_nodes_count;
    tmp = rc_zalloc(sizeof(ipfs_rpc_t) + bootstraps_nbytes, NULL);
    if (!tmp) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        return NULL;
    }

    memcpy(tmp->rpc_nodes, options->rpc_nodes, bootstraps_nbytes);
    tmp->rpc_nodes_count = options->rpc_nodes_count;
    tmp->writeback_cb    = cb;
    tmp->user_data       = user_data;

    rc = select_bootstrap(options->rpc_nodes, options->rpc_nodes_count,
                          tmp->current_node);
    if (rc < 0) {
        vlogE("IpfsToken: No configured node is reachable.");
        hive_set_error(rc);
        deref(tmp);
        return NULL;
    }

    if (options->uid[0]) {
        rc = _ipfs_token_get_uid_info(tmp->current_node, options->uid, NULL);
        if (rc < 0) {
            vlogE("IpfsToken: failed to get info of configured uid.");
            hive_set_error(rc);
            deref(tmp);
            return NULL;
        }
        strcpy(tmp->uid, options->uid);
        vlogI("IpfsToken: Use configured uid: %s.", tmp->uid);
    } else if (options->store) {
        rc = load_store(options->store, tmp->uid, sizeof(tmp->uid));
        if (rc < 0) {
            vlogE("IpfsToken: failed to restore from cache.");
            hive_set_error(rc);
            deref(tmp);
            return NULL;
        }

        rc = _ipfs_token_get_uid_info(tmp->current_node, tmp->uid, NULL);
        if (rc < 0) {
            vlogE("IpfsToken: failed to get info of cached uid.");
            hive_set_error(rc);
            deref(tmp);
            return NULL;
        }

        vlogI("IpfsToken: Use cached uid: %s.", tmp->uid);
    } else {
        vlogI("IpfsToken: No uid configured or cache, create one for user.");

        rc = uid_new(tmp->current_node, tmp->uid, sizeof(tmp->uid));
        if (rc < 0) {
            vlogE("IpfsToken: failed to create uid.");
            hive_set_error(rc);
            deref(tmp);
            return NULL;
        }

        rc = publish_root_hash(tmp, url, sizeof(url));
        if (rc < 0) {
            vlogE("IpfsToken: failed to publish root hash.");
            hive_set_error(rc);
            deref(tmp);
            return NULL;
        }

        vlogI("IpfsToken: Use uid created: %s.", tmp->uid);
    }

    writeback_tokens(tmp);

    return tmp;
}

int ipfs_rpc_close(ipfs_rpc_t *rpc)
{
    deref(rpc);
    return 0;
}
