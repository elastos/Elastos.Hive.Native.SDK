#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#include <windows.h>
#include <shellapi.h>
#endif

#include "test_helper.h"
#include "constants.h"
#include "config.h"

HiveClient *onedrive_client_new()
{
    OneDriveOptions options = {
        .base.drive_type          = HiveDriveType_OneDrive,
        .base.persistent_location = global_config.data_dir,
        .redirect_url             = HIVETEST_REDIRECT_URL,
        .scope                    = HIVETEST_SCOPE,
        .client_id                = HIVETEST_ONEDRIVE_CLIENT_ID
    };

    return hive_client_new((HiveOptions *)&options);
}

HiveClient *ipfs_client_new()
{
    int i;
    HiveClient *client;

    HiveRpcNode *nodes = calloc(1, sizeof(HiveRpcNode) * global_config.ipfs_rpc_nodes_sz);
    if (!nodes)
        return NULL;

    for (i = 0; i < global_config.ipfs_rpc_nodes_sz; ++i) {
        HiveRpcNode *node = nodes + i;

        node->ipv4 = global_config.ipfs_rpc_nodes[i]->ipv4;
        node->ipv6 = global_config.ipfs_rpc_nodes[i]->ipv6;
        node->port = global_config.ipfs_rpc_nodes[i]->port;
    }

    IPFSOptions options = {
        .base.drive_type          = HiveDriveType_IPFS,
        .base.persistent_location = global_config.data_dir,
        .uid                      = NULL,
        .rpc_node_count           = global_config.ipfs_rpc_nodes_sz,
        .rpcNodes                 = nodes
    };

    client = hive_client_new((HiveOptions *)&options);
    free(nodes);
    return client;
}

int open_authorization_url(const char *url, void *context)
{
#if defined(_WIN32) || defined(_WIN64)
ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    return 0;
#elif defined(__linux__)
char cmd[strlen("xdg-open ") + strlen(url) + 3];
    sprintf(cmd, "xdg-open '%s'", url);
    system(cmd);
    return 0;
#elif defined(__APPLE__)
char cmd[strlen("open ") + strlen(url) + 3];
sprintf(cmd, "open '%s'", url);
system(cmd);
return 0;
#else
#   error "Unsupported Os."
#endif
}
