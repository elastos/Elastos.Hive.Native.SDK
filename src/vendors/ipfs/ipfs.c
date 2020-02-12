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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <winsock2.h>
#include <winnt.h>
#endif

#include <cjson/cJSON.h>
#include <crystal.h>

#include "ela_hive.h"
#include "ipfs_rpc.h"
#include "ipfs_constants.h"
#include "http_client.h"
#include "hive_error.h"
#include "hive_client.h"
#include "http_status.h"

typedef struct IPFSConnect {
    HiveConnect base;
    ipfs_rpc_t *rpc;
} IPFSConnect;

static int put_file_from_buffer(HiveConnect *base, const void *from,
                                size_t length, bool encrypt, IPFSCid *cid)
{
    IPFSConnect *connect = (IPFSConnect *)base;
    char url[MAX_URL_LEN] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    cJSON *resp;
    cJSON *cid_json;
    char *p;
    int rc;

    rc = ipfs_rpc_check_reachable(connect->rpc);
    if (rc < 0)
        return rc;

    sprintf(url, "http://%s:%u/api/v0/add",
            ipfs_rpc_get_current_node_ip(connect->rpc),
            (unsigned)ipfs_rpc_get_current_node_port(connect->rpc));

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    http_client_set_url(httpc, url);
    http_client_set_mime_instant(httpc, "file", NULL, NULL, from, length);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc) {
        if (RC_NODE_UNREACHABLE(rc)) {
            rc = HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
            ipfs_rpc_mark_node_unreachable(connect->rpc);
        } else
            rc = HIVE_CURL_ERROR(rc);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc) {
        rc = HIVE_CURL_ERROR(rc);
        goto error_exit;
    }

    if (resp_code != HttpStatus_OK) {
        rc = HIVE_HTTP_STATUS_ERROR(resp_code);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);
    if (!p)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    resp = cJSON_Parse(p);
    free(p);

    if (!resp)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    cid_json = cJSON_GetObjectItemCaseSensitive(resp, "Hash");
    if (!cid_json || !cJSON_IsString(cid_json) || !cid_json->string ||
        !*cid_json->valuestring || strlen(cid_json->valuestring) >= sizeof(cid->content)) {
        cJSON_Delete(resp);
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
    }

    strcpy(cid->content, cid_json->valuestring);
    cJSON_Delete(resp);

    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static ssize_t get_file_length(HiveConnect *base, const IPFSCid *cid)
{
    IPFSConnect *connect = (IPFSConnect *)base;
    char url[MAX_URL_LEN] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    cJSON *cid_json;
    cJSON *objects;
    ssize_t fsize;
    cJSON *resp;
    cJSON *size;
    char *p;
    int rc;

    rc = ipfs_rpc_check_reachable(connect->rpc);
    if (rc < 0)
        return rc;

    sprintf(url, "http://%s:%u/api/v0/file/ls",
            ipfs_rpc_get_current_node_ip(connect->rpc),
            (unsigned)ipfs_rpc_get_current_node_port(connect->rpc));

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "arg", cid->content);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc) {
        if (RC_NODE_UNREACHABLE(rc)) {
            rc = HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
            ipfs_rpc_mark_node_unreachable(connect->rpc);
        } else
            rc = HIVE_CURL_ERROR(rc);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc) {
        rc = HIVE_CURL_ERROR(rc);
        goto error_exit;
    }

    if (resp_code != HttpStatus_OK) {
        rc = HIVE_HTTP_STATUS_ERROR(resp_code);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);
    if (!p)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    resp = cJSON_Parse(p);
    free(p);

    if (!resp)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    objects = cJSON_GetObjectItemCaseSensitive(resp, "Objects");
    if (!objects || !cJSON_IsObject(objects)) {
        cJSON_Delete(resp);
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
    }

    cid_json = cJSON_GetObjectItemCaseSensitive(objects, cid->content);
    if (!cid_json || !cJSON_IsObject(cid_json)) {
        cJSON_Delete(resp);
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
    }

    size = cJSON_GetObjectItemCaseSensitive(cid_json, "Size");
    if (!size || !cJSON_IsNumber(size) || (ssize_t)size->valuedouble < 0) {
        cJSON_Delete(resp);
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
    }

    fsize = (ssize_t)size->valuedouble;
    cJSON_Delete(resp);

    return fsize;

error_exit:
    http_client_close(httpc);
    return (ssize_t)rc;
}

static size_t get_response_body_cb(char *buffer, size_t size, size_t nitems, void *userdata)
{
    uint8_t *buf = ((void **)userdata)[0];
    size_t bufsz = *((size_t *)(((void **)userdata)[1]));
    size_t *actual_sz = (size_t *)(((void **)userdata)[2]);
    size_t total_sz = size * nitems;

    if (*actual_sz + total_sz > bufsz)
        return 0;

    memcpy(buf + *actual_sz, buffer, total_sz);
    *actual_sz += total_sz;

    return total_sz;
}

static ssize_t get_file_to_buffer(HiveConnect *base, const IPFSCid *cid, bool decrypt, void *to, size_t buflen)
{
    IPFSConnect *connect = (IPFSConnect *)base;
    char url[MAX_URL_LEN] = {0};
    http_client_t *httpc;
    size_t actual_sz = 0;
    long resp_code = 0;
    ssize_t fsize;
    int rc;
    void *args[] = {to, &buflen, &actual_sz};

    fsize = get_file_length(base, cid);
    if (fsize <= 0)
        return fsize;

    if ((ssize_t)buflen > 0 && fsize > (ssize_t)buflen)
        return HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);

    rc = ipfs_rpc_check_reachable(connect->rpc);
    if (rc < 0)
        return rc;

    sprintf(url, "http://%s:%u/api/v0/cat",
            ipfs_rpc_get_current_node_ip(connect->rpc),
            (unsigned)ipfs_rpc_get_current_node_port(connect->rpc));

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "arg", cid->content);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_request_body_instant(httpc, NULL, 0);
    http_client_set_response_body(httpc, get_response_body_cb, args);

    rc = http_client_request(httpc);
    if (rc) {
        if (RC_NODE_UNREACHABLE(rc)) {
            rc = HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
            ipfs_rpc_mark_node_unreachable(connect->rpc);
        } else
            rc = HIVE_CURL_ERROR(rc);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    if (rc)
        return HIVE_CURL_ERROR(rc);

    if (resp_code != HttpStatus_OK)
        return HIVE_HTTP_STATUS_ERROR(resp_code);

    if (fsize != actual_sz)
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_PERSISTENT_DATA);

    return fsize;

error_exit:
    http_client_close(httpc);
    return rc;
}

static int disconnect(HiveConnect *base)
{
    assert(base);

    deref(base);
    return 0;
}

static void ipfs_connect_destructor(void *p)
{
    IPFSConnect *client = (IPFSConnect *)p;

    if (client->rpc)
        ipfs_rpc_close(client->rpc);
}

static inline bool is_valid_ip(const char *ip)
{
    struct sockaddr_in addr;
    return inet_pton(AF_INET, ip, &addr) > 0 ? true : false;
}

static inline bool is_valid_ipv6(const char *ip)
{
    struct sockaddr_in6 addr;
    return inet_pton(AF_INET6, ip, &addr) > 0 ? true : false;
}

HiveConnect *ipfs_client_connect(HiveClient *client, const HiveConnectOptions *options_base)
{
    IPFSConnectOptions *options = (IPFSConnectOptions *)options_base;
    ipfs_rpc_options_t *token_options;
    size_t token_options_sz;
    IPFSConnect *connect;
    size_t i;

    assert(options);

    token_options_sz = sizeof(*token_options) +
                       sizeof(rpc_node_t) * options->rpc_node_count;
    token_options = alloca(token_options_sz);
    memset(token_options, 0, token_options_sz);

    // check bootstraps configuration
    if (!options->rpc_node_count)
        return NULL;

    token_options->rpc_nodes_count = options->rpc_node_count;
    for (i = 0; i < options->rpc_node_count; ++i) {
        IPFSNode *node = &options->rpcNodes[i];
        rpc_node_t *node_options = &token_options->rpc_nodes[i];
        size_t ipv4_len;
        size_t ipv6_len;
        char *endptr;
        long port;

        ipv4_len = node->ipv4 ? strlen(node->ipv4) : 0;
        ipv6_len = node->ipv6 ? strlen(node->ipv6) : 0;

        if (!ipv4_len && !ipv6_len)
            return NULL;

        if (ipv4_len) {
            if (ipv4_len >= sizeof(node_options->ipv4))
                return NULL;

            if (!is_valid_ip(node->ipv4))
                return NULL;

            strcpy(node_options->ipv4, node->ipv4);
        }

        if (ipv6_len) {
            if (ipv6_len >= sizeof(node_options->ipv6))
                return NULL;

            if (!is_valid_ipv6(node->ipv6))
                return NULL;

            strcpy(node_options->ipv6, node->ipv6);
        }

        if (!node->port || !*node->port)
            return NULL;

        port = strtol(node->port, &endptr, 10);
        if (*endptr)
            return NULL;

        if (port < 1 || port > 65535)
            return NULL;

        node_options->port = (uint16_t)port;
    }

    connect = (IPFSConnect *)rc_zalloc(sizeof(IPFSConnect), &ipfs_connect_destructor);
    if (!connect)
        return NULL;

    connect->base.ipfs_put_file_from_buffer = put_file_from_buffer;
    connect->base.ipfs_get_file_length      = get_file_length;
    connect->base.ipfs_get_file_to_buffer   = get_file_to_buffer;
    connect->base.disconnect                = disconnect;

    connect->rpc = ipfs_rpc_new(token_options, connect);
    if (!connect->rpc) {
        deref(connect);
        return NULL;
    }

    return &connect->base;
}
