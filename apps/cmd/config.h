#ifndef  __CONFIG_H__
#define  __CONFIG_H__

#include <stdbool.h>
#include <ela_hive.h>
#include <crystal.h>

typedef struct {
    char *name;
    char *id;
} drive_t;

typedef struct {
    int vendor;
} client_base_t;

typedef struct {
    client_base_t base;
    char *client_id;
    char *scope;
    char *redirect_url;
} onedrive_client_t;

typedef struct {
    client_base_t base;
    char *uid;
    int rpc_nodes_sz;
    HiveRpcNode **rpc_nodes;
} ipfs_client_t;

typedef struct {
    int loglevel;
    char *logfile;

    char *persistent_location;

    client_base_t *client;
} cmd_cfg_t;

cmd_cfg_t *load_config(const char *config_file);

#endif // __CONFIG_H__
