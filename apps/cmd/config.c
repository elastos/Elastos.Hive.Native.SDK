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

static void config_destructor(void *p)
{
    cmd_cfg_t *config = (cmd_cfg_t *)p;

    if (!config)
        return;

    if (config->logfile)
        free(config->logfile);

    if (config->persistent_location)
        free(config->persistent_location);

    if (config->uid)
        free(config->uid);

    if (config->ipfs_rpc_nodes) {
        int i;

        for (i = 0; i < config->ipfs_rpc_nodes_sz; ++i)
            deref(config->ipfs_rpc_nodes[i]);

        free(config->ipfs_rpc_nodes);
    }
}

static void qualified_path(const char *path, const char *ref, char *qualified)
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
                snprintf(qualified, p - ref + 1, "%s", ref);
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

cmd_cfg_t *load_config(const char *config_file)
{
    cmd_cfg_t *config;
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

    config = (cmd_cfg_t *)rc_zalloc(sizeof(cmd_cfg_t), config_destructor);
    if (!config) {
        fprintf(stderr, "Load configuration failed, out of memory.\n");
        config_destroy(&cfg);
        return NULL;
    }

    config->loglevel = 3;
    config_lookup_int(&cfg, "loglevel", &config->loglevel);

    rc = config_lookup_string(&cfg, "logfile", &stropt);
    if (rc && *stropt) {
        config->logfile = strdup(stropt);
    }

    rc = config_lookup_string(&cfg, "persistent_location", &stropt);
    if (!rc || !*stropt) {
        fprintf(stderr, "Missing datadir option.\n");
        config_destroy(&cfg);
        deref(config);
        return NULL;
    } else {
        char path[PATH_MAX];
        qualified_path(stropt, config_file, path);
        config->persistent_location = strdup(path);
    }

    rc = config_lookup_string(&cfg, "ipfs_uid", &stropt);
    if (rc && *stropt) {
        config->uid = strdup(stropt);
    }

    rpc_nodes = config_lookup(&cfg, "ipfs_rpc_nodes");
    if (!rpc_nodes || !config_setting_is_list(rpc_nodes)) {
        fprintf(stderr, "Missing ipfs_rpc_nodes section.\n");
        config_destroy(&cfg);
        deref(config);
        return NULL;
    }

    entries = config_setting_length(rpc_nodes);
    if (entries <= 0) {
        fprintf(stderr, "Empty bootstraps option.\n");
        config_destroy(&cfg);
        deref(config);
        return NULL;
    }

    config->ipfs_rpc_nodes_sz = entries;
    config->ipfs_rpc_nodes = (HiveRpcNode **)calloc(1, entries * sizeof(HiveRpcNode *));
    if (!config->ipfs_rpc_nodes) {
        fprintf(stderr, "Out of memory.\n");
        config_destroy(&cfg);
        deref(config);
        return NULL;
    }

    for (i = 0; i < entries; i++) {
        HiveRpcNode *node;

        node = rc_zalloc(sizeof(HiveRpcNode), rpc_node_destructor);
        if (!node) {
            fprintf(stderr, "Out of memory.\n");
            config_destroy(&cfg);
            deref(config);
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
            deref(config);
            return NULL;
        }

        rc = config_setting_lookup_int(nd, "port", &intopt);
        if (rc && intopt) {
            sprintf(number, "%d", intopt);
            node->port = (const char *)strdup(number);
        } else
            node->port = NULL;

        config->ipfs_rpc_nodes[i] = node;
    }

    config_destroy(&cfg);

    return config;
}
