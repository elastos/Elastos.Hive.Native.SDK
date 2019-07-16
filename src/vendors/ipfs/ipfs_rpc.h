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

#ifndef __IPFS_RPC_H__
#define __IPFS_RPC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <cjson/cJSON.h>

#include "ela_hive.h"
#include "ipfs_constants.h"

typedef struct ipfs_rpc ipfs_rpc_t;

typedef struct rpc_node {
    char ipv4[HIVE_MAX_IPV4_ADDRESS_LEN  + 1];
    char ipv6[HIVE_MAX_IPV6_ADDRESS_LEN  + 1];
    uint16_t port;
} rpc_node_t;

typedef struct ipfs_rpc_options {
    cJSON *store;
    char uid[HIVE_MAX_IPFS_UID_LEN + 1];
    size_t rpc_nodes_count;
    rpc_node_t rpc_nodes[0];
} ipfs_rpc_options_t;

typedef int ipfs_rpc_writeback_func_t(const cJSON *json, void *user_data);

ipfs_rpc_t *ipfs_rpc_new(ipfs_rpc_options_t *options,
                         ipfs_rpc_writeback_func_t cb,
                         void *user_data);
int ipfs_rpc_close(ipfs_rpc_t *rpc);
int ipfs_rpc_synchronize(ipfs_rpc_t *rpc);
int ipfs_rpc_reset(ipfs_rpc_t *rpc);
int ipfs_rpc_get_uid_info(ipfs_rpc_t *rpc, char **result);
const char *ipfs_rpc_get_uid(ipfs_rpc_t *rpc);
const char *ipfs_rpc_get_current_node(ipfs_rpc_t *rpc);
int ipfs_rpc_check_reachable(ipfs_rpc_t *rpc);
void ipfs_rpc_mark_node_unreachable(ipfs_rpc_t *rpc);

#ifdef __cplusplus
}
#endif

#endif // __IPFS_RPC_H__
