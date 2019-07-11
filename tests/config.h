#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdbool.h>
#include <ela_hive.h>
#include <crystal.h>

typedef struct {
    int loglevel;
    char *logfile;
    int log2file;
    char *data_dir;
    int shuffle;
    int ipfs_rpc_nodes_sz;
    HiveRpcNode **ipfs_rpc_nodes;
} test_cfg_t;

extern test_cfg_t global_config;

test_cfg_t *load_config(const char *config_file);
void config_deinit();
void qualified_path(const char *path, const char *ref, char *qualified);

#endif // __CONFIG_H__
