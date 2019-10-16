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

#include <stdlib.h>

#include <CUnit/Basic.h>
#include <ela_hive.h>

#include "../cases/case.h"
#include "../cases/ipfs_file_apis_cases.h"
#include "../test_context.h"
#include "../../config.h"

static CU_TestInfo cases[] = {
    DEFINE_IPFS_FILE_APIS_CASES,
    DEFINE_TESTCASE_NULL
};

CU_TestInfo* ipfs_get_cases()
{
    return cases;
}

int ipfs_suite_init()
{
    int i;

    HiveRpcNode *nodes = calloc(1, sizeof(HiveRpcNode) * global_config.ipfs_rpc_nodes_sz);
    if (!nodes) {
        CU_FAIL("Error: test suite initialize error");
        return -1;
    }

    for (i = 0; i < global_config.ipfs_rpc_nodes_sz; ++i) {
        HiveRpcNode *node = nodes + i;

        node->ipv4 = global_config.ipfs_rpc_nodes[i]->ipv4;
        node->ipv6 = global_config.ipfs_rpc_nodes[i]->ipv6;
        node->port = global_config.ipfs_rpc_nodes[i]->port;
    }

    IPFSConnectOptions opts = {
        .backendType    = HiveBackendType_IPFS,
        .rpc_node_count = global_config.ipfs_rpc_nodes_sz,
        .rpcNodes       = nodes
    };

    test_ctx.connect = hive_client_connect(test_ctx.client, (HiveConnectOptions *)&opts);
    free(nodes);
    if (!test_ctx.connect) {
        CU_FAIL("Error: test suite initialize error");
        return -1;
    }

    return 0;
}

int ipfs_suite_cleanup()
{
    test_context_reset();

    return 0;
}
