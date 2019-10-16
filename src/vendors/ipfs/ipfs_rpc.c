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
#include "ipfs_constants.h"
#include "http_client.h"
#include "http_status.h"

struct ipfs_rpc {
    char current_node_ip[HIVE_MAX_IPV6_ADDRESS_LEN  + 1];
    uint16_t current_node_port;
    size_t rpc_nodes_count;
    rpc_node_t rpc_nodes[0];
};

static int test_reachable(const char *ipaddr, uint16_t port)
{
    char url[MAXPATHLEN + 1];
    http_client_t *httpc;
    int rc;
    long resp_code;

    rc = snprintf(url, sizeof(url), "http://%s:%d/version", ipaddr, port);
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

    if (resp_code != HttpStatus_OK) {
        vlogE("IpfsToken: error from response (%d).", resp_code);
        return HIVE_HTTP_STATUS_ERROR(resp_code);
    }

    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static int select_bootstrap(rpc_node_t *rpc_nodes, size_t nodes_cnt,
                            char *selected_ip, uint16_t *selected_port)
{
    size_t i;
    size_t base;
    int rc;

    srand((unsigned)time(NULL));
    base = (size_t)rand() % nodes_cnt;
    i = base;

    do {
        if (rpc_nodes[i].ipv4[0]) {
            rc = test_reachable(rpc_nodes[i].ipv4, rpc_nodes[i].port);
            if (!rc) {
                strcpy(selected_ip, rpc_nodes[i].ipv4);
                *selected_port = rpc_nodes[i].port;
                vlogI("IpfsToken: node selected: %s.", selected_ip);
                return 0;
            }
        }

        if (rpc_nodes[i].ipv6[0]) {
            rc = test_reachable(rpc_nodes[i].ipv6, rpc_nodes[i].port);
            if (!rc) {
                strcpy(selected_ip, rpc_nodes[i].ipv6);
                *selected_port = rpc_nodes[i].port;
                vlogI("IpfsToken: node selected: %s.", selected_ip);
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

    if (rpc->current_node_ip[0])
        return 0;

    rc = select_bootstrap(rpc->rpc_nodes, rpc->rpc_nodes_count,
                          rpc->current_node_ip, &rpc->current_node_port);
    if (rc < 0)
        vlogE("IpfsToken: no node configured is reachable.");

    return rc;
}

void ipfs_rpc_mark_node_unreachable(ipfs_rpc_t *rpc)
{
    rpc->current_node_ip[0] = '\0';
}

const char *ipfs_rpc_get_current_node_ip(ipfs_rpc_t *rpc)
{
    return rpc->current_node_ip;
}

uint16_t ipfs_rpc_get_current_node_port(ipfs_rpc_t *rpc)
{
    return rpc->current_node_port;
}

ipfs_rpc_t *ipfs_rpc_new(ipfs_rpc_options_t *options, void *user_data)
{
    ipfs_rpc_t *tmp;
    size_t bootstraps_nbytes;
    int rc;

    bootstraps_nbytes = sizeof(options->rpc_nodes[0]) * options->rpc_nodes_count;
    tmp = rc_zalloc(sizeof(ipfs_rpc_t) + bootstraps_nbytes, NULL);
    if (!tmp) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        return NULL;
    }

    memcpy(tmp->rpc_nodes, options->rpc_nodes, bootstraps_nbytes);
    tmp->rpc_nodes_count = options->rpc_nodes_count;

    rc = select_bootstrap(options->rpc_nodes, options->rpc_nodes_count,
                          tmp->current_node_ip, &tmp->current_node_port);
    if (rc < 0) {
        vlogE("IpfsToken: No configured node is reachable.");
        hive_set_error(rc);
        deref(tmp);
        return NULL;
    }

    return tmp;
}

int ipfs_rpc_close(ipfs_rpc_t *rpc)
{
    deref(rpc);
    return 0;
}
