#ifndef  __CONFIG_H__
#define  __CONFIG_H__

#include <stdbool.h>
#include <ela_hive.h>
#include <crystal.h>

typedef struct {
    int loglevel;
    char *logfile;

    char *persistent_location;

    char *uid;

    int ipfs_rpc_nodes_sz;
    HiveRpcNode **ipfs_rpc_nodes;
} cmd_cfg_t;

cmd_cfg_t *load_config(const char *config_file);

#endif // __CONFIG_H__
