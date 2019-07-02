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
    demo_cfg_t *config = (demo_cfg_t *)p;

    if (!config)
        return;

    if (config->logfile)
        free(config->logfile);

    if (config->persistent_location)
        free(config->persistent_location);

    if (config->client)
        deref(config->client);
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

static inline void str_tolower(char *str)
{
    char *c;

    for (c = str; *c; ++c)
        *c = tolower(*c);
}

static void onedrive_client_destructor(void *obj)
{
    onedrive_client_t *client = (onedrive_client_t *)obj;

    if (client->client_id)
        free(client->client_id);

    if (client->scope)
        free(client->scope);

    if (client->redirect_url)
        free(client->redirect_url);
}

static void ipfs_client_destructor(void *obj)
{
    ipfs_client_t *client = (ipfs_client_t *)obj;

    if (client->uid)
        free(client->uid);

    if (client->rpc_nodes) {
        int i;

        for (i = 0; i < client->rpc_nodes_sz; ++i)
            deref(client->rpc_nodes[i]);

        free(client->rpc_nodes);
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

demo_cfg_t *load_config(const char *config_file)
{
    demo_cfg_t *config;
    config_t cfg;
    config_setting_t *client;
    const char *stropt;
    char *strtmp;
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

    config = (demo_cfg_t *)rc_zalloc(sizeof(demo_cfg_t), config_destructor);
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

    client = config_lookup(&cfg, "client");
    if (!client || !config_setting_is_group(client)) {
        fprintf(stderr, "Missing client section.\n");
        config_destroy(&cfg);
        deref(config);
        return NULL;
    }

    rc = config_setting_lookup_string(client, "vendor", &stropt);
    if (!rc || !*stropt) {
        fprintf(stderr, "Missing client.vendor section.\n");
        config_destroy(&cfg);
        deref(config);
        return NULL;
    }

    strtmp = strdup(stropt);
    str_tolower(strtmp);

    if (!strcmp(strtmp, "onedrive")) {
        onedrive_client_t *c;

        free(strtmp);

        config->client = rc_zalloc(sizeof(onedrive_client_t),
                                   onedrive_client_destructor);
        if (!config->client) {
            fprintf(stderr, "Out of momery.\n");
            config_destroy(&cfg);
            deref(config);
            return NULL;
        }

        c = (onedrive_client_t *)config->client;

        c->base.vendor = HiveDriveType_OneDrive;

        rc = config_setting_lookup_string(client, "client_id", &stropt);
        if (!rc || !*stropt) {
            fprintf(stderr, "Missing client.client_id section.\n");
            config_destroy(&cfg);
            deref(config);
            return NULL;
        }
        c->client_id = strdup(stropt);

        rc = config_setting_lookup_string(client, "scope", &stropt);
        if (!rc || !*stropt) {
            fprintf(stderr, "Missing client.scope section.\n");
            config_destroy(&cfg);
            deref(config);
            return NULL;
        }
        c->scope = strdup(stropt);

        rc = config_setting_lookup_string(client, "redirect_url", &stropt);
        if (!rc || !*stropt) {
            fprintf(stderr, "Missing client.redirect_url section.\n");
            config_destroy(&cfg);
            deref(config);
            return NULL;
        }
        c->redirect_url = strdup(stropt);

        config_destroy(&cfg);
        return config;
    } else if (!strcmp(strtmp, "ipfs")) {
        ipfs_client_t *c;
        config_setting_t *rpc_nodes;

        free(strtmp);

        config->client = rc_zalloc(sizeof(ipfs_client_t),
                                   ipfs_client_destructor);
        if (!config->client) {
            fprintf(stderr, "Out of momery.\n");
            config_destroy(&cfg);
            deref(config);
            return NULL;
        }

        c = (ipfs_client_t *)config->client;

        c->base.vendor = HiveDriveType_IPFS;

        rc = config_setting_lookup_string(client, "uid", &stropt);
        if (rc)
            c->uid = strdup(stropt);

        rpc_nodes = config_setting_lookup(client, "rpc_nodes");
        if (!rpc_nodes || !config_setting_is_list(rpc_nodes)) {
            fprintf(stderr, "Missing client.rpc_nodes section.\n");
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

        c->rpc_nodes_sz = entries;
        c->rpc_nodes = (HiveRpcNode **)calloc(1, c->rpc_nodes_sz *
                                                 sizeof(HiveRpcNode *));
        if (!c->rpc_nodes) {
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

            rc = config_setting_lookup_int(nd, "port", &intopt);
            if (rc && intopt) {
                sprintf(number, "%d", intopt);
                node->port = (const char *)strdup(number);
            } else
                node->port = NULL;

            c->rpc_nodes[i] = node;
        }

        config_destroy(&cfg);
        return config;
    } else {
        fprintf(stderr, "Missing client.vendor section.\n");
        free(strtmp);
        config_destroy(&cfg);
        deref(config);
        return NULL;
    }
}
