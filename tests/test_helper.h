#ifndef __TEST_HELPER_H__
#define __TEST_HELPER_H__

#include <ela_hive.h>

typedef struct {
    char *name;
    char *type;
} dir_entry;

#define ONEDRIVE_DIR_ENTRY(n, t) \
    { \
        .name = n, \
        .type = t  \
    }

#define IPFS_DIR_ENTRY(n, t) \
    { \
        .name = n, \
        .type = NULL  \
    }

HiveClient *onedrive_client_new();
HiveClient *ipfs_client_new();
int open_authorization_url(const char *url, void *context);
char *get_random_file_name();
int list_files_test_scheme(HiveDrive *drive, const char *dir,
                           dir_entry *expected_entries, int size);
#endif // __TEST_HELPER_H__
