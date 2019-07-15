#include <stdlib.h>
#include <limits.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#include <windows.h>
#include <shellapi.h>
#include <crystal.h>
#endif

#include <CUnit/Basic.h>

#include <ela_hive.h>

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

char *get_random_file_name()
{
    static char fname[64];

    snprintf(fname, sizeof(fname), "/hivetests%d", rand());

    return fname;
}

static bool list_files_cb(const KeyValue *info, size_t size, void *context)
{
    dir_entry *entries = (dir_entry *)(((void **)context)[0]);
    dir_entry *entry;
    int entries_sz = *(int *)(((void **)context)[1]);
    int *actual_sz = (int *)(((void **)context)[2]);
    int i;

    if (!info)
        return false;

    if (*actual_sz >= entries_sz) {
        ++(*actual_sz);
        return false;
    }

    entry = entries + (*actual_sz)++;

    for (i = 0; i < size; ++i) {
        if (!strcmp(info[i].key, "name")) {
            if (info[i].value)
                entry->name = strdup(info[i].value);
        } else if (!strcmp(info[i].key, "type")) {
            if (info[i].value)
                entry->type = strdup(info[i].value);
        }
    }

    return true;
}

static int dir_entry_cmp(dir_entry *ent1, dir_entry *ent2)
{
#define entry_field_cmp(field1, field2) \
    if (!(((field1) && (field2) && !strcmp(field1, field2)) || \
          (!(field1) && !(field2)))) \
            return -1;

    entry_field_cmp(ent1->type, ent2->type);
    entry_field_cmp(ent1->name, ent2->name);
    return 0;
}

static void cleanup_entries(dir_entry *entries, int size)
{
    int i;

    if (!entries)
        return;

    for (i = 0; i < size; ++i) {
        if (entries[i].name)
            free(entries[i].name);
        if (entries[i].type)
            free(entries[i].type);
    }
}

int list_files_test_scheme(HiveDrive *drive, const char *dir,
                           dir_entry *expected_entries, int size)
{
    int i;
    int rc;
    int actual_sz = 0;
    dir_entry *entries = NULL;

    if (size) {
        entries = alloca(sizeof(dir_entry) * size);
        memset(entries, 0, sizeof(dir_entry) * size);
    }

    void *args[] = {entries, &size, &actual_sz};
    rc = hive_drive_list_files(drive, dir, list_files_cb, args);
    if (rc < 0) {
        CU_FAIL("calling hive_drive_list_files() failed");
        cleanup_entries(entries, size);
        return -1;
    }

    if (actual_sz != size) {
        CU_FAIL("actual dir entries size does not match the expected");
        cleanup_entries(entries, size);
        return -1;
    }

    for (i = 0; i < size; ++i) {
        int j;
        dir_entry *expected_entry = expected_entries + i;

        for (j = 0; j < size; ++j) {
            dir_entry *actual_entry = entries + i;
            if (!dir_entry_cmp(expected_entry, actual_entry))
                break;
        }

        if (j >= size)
            break;
    }

    CU_ASSERT(i == size);

    cleanup_entries(entries, size);
    return 0;
}
