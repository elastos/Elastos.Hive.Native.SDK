#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif

#include <libconfig.h>
#include <crystal.h>

#include "config.h"

test_cfg_t global_config;

void qualified_path(const char *path, const char *ref, char *qualified)
{
    if (*path == '/') {
        strcpy(qualified, path);
    } else if (*path == '~') {
        sprintf(qualified, "%s%s", getenv("HOME"), path+1);
    } else {
        if (ref) {
            const char *p = strrchr(ref, '/');
            if (!p) p = ref;

            if (p - ref > 0)
                strncpy(qualified, ref, p - ref);
            else
                *qualified = 0;
        } else {
            getcwd(qualified, PATH_MAX);
        }

        if (*qualified)
            strcat(qualified, "/");

        strcat(qualified, path);
    }
}

static void rpc_node_destructor(void *obj)
{
    HiveRpcNode *node = (HiveRpcNode *)obj;

    if (!node)
        return;

    if (node->ipv4)
        free((void *)node->ipv4);

    if (node->ipv6)
        free((void *)node->ipv6);

    if (node->port)
        free((void *)node->port);
}

void config_deinit()
{
    if (global_config.logfile)
        free(global_config.logfile);

    if (global_config.data_dir)
        free(global_config.data_dir);

    if (global_config.ipfs_rpc_nodes) {
        int i;

        for (i = 0; i < global_config.ipfs_rpc_nodes_sz; ++i)
            deref(global_config.ipfs_rpc_nodes[i]);

        free(global_config.ipfs_rpc_nodes);
    }
}

test_cfg_t *load_config(const char *config_file)
{
    config_t cfg;
    config_setting_t *rpc_nodes;
    const char *stropt;
    char number[64];
    int intopt;
    int entries;
    int i;
    int rc;

    config_init(&cfg);

    rc = config_read_file(&cfg, config_file);
    if (!rc) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return NULL;
    }

    global_config.loglevel = 3;
    config_lookup_int(&cfg, "loglevel", &global_config.loglevel);

    rc = config_lookup_string(&cfg, "logfile", &stropt);
    if (rc && *stropt)
        global_config.logfile = strdup(stropt);

    global_config.log2file = 0;
    config_lookup_int(&cfg, "log2file", &global_config.log2file);

    rc = config_lookup_string(&cfg, "data-dir", &stropt);
    if (!rc || !*stropt) {
        fprintf(stderr, "Missing datadir option.\n");
        config_destroy(&cfg);
        config_deinit();
        return NULL;
    } else {
        char path[PATH_MAX];
        qualified_path(stropt, config_file, path);
        global_config.data_dir = strdup(path);
    }

    global_config.shuffle = 0;
    config_lookup_int(&cfg, "shuffle", &global_config.shuffle);

    rpc_nodes = config_lookup(&cfg, "ipfs_rpc_nodes");
    if (!rpc_nodes || !config_setting_is_list(rpc_nodes)) {
        fprintf(stderr, "Missing ipfs_rpc_nodes section.\n");
        config_destroy(&cfg);
        config_deinit();
        return NULL;
    }

    entries = config_setting_length(rpc_nodes);
    if (entries <= 0) {
        fprintf(stderr, "Empty bootstraps option.\n");
        config_destroy(&cfg);
        config_deinit();
        return NULL;
    }

    global_config.ipfs_rpc_nodes_sz = entries;
    global_config.ipfs_rpc_nodes = (HiveRpcNode **)calloc(1, entries * sizeof(HiveRpcNode *));
    if (!global_config.ipfs_rpc_nodes) {
        fprintf(stderr, "Out of memory.\n");
        config_destroy(&cfg);
        config_deinit();
        return NULL;
    }

    for (i = 0; i < entries; i++) {
        HiveRpcNode *node;

        node = rc_zalloc(sizeof(HiveRpcNode), rpc_node_destructor);
        if (!node) {
            fprintf(stderr, "Out of memory.\n");
            config_destroy(&cfg);
            config_deinit();
            return NULL;
        }

        config_setting_t *nd = config_setting_get_elem(rpc_nodes, i);

        rc = config_setting_lookup_string(nd, "ipv4", &stropt);
        if (rc && *stropt)
            node->ipv4 = (const char *)strdup(stropt);
        else
            node->ipv4 = NULL;

        rc = config_setting_lookup_string(nd, "ipv6", &stropt);
        if (rc && *stropt)
            node->ipv6 = (const char *)strdup(stropt);
        else
            node->ipv6 = NULL;

        if (!node->ipv4 && !node->ipv6) {
            fprintf(stderr, "Missing IPFS RPC node ip address.\n");
            config_destroy(&cfg);
            config_deinit();
            return NULL;
        }

        rc = config_setting_lookup_int(nd, "port", &intopt);
        if (rc && intopt) {
            sprintf(number, "%d", intopt);
            node->port = (const char *)strdup(number);
        } else
            node->port = NULL;

        global_config.ipfs_rpc_nodes[i] = node;
    }

    config_destroy(&cfg);
    return &global_config;
}

