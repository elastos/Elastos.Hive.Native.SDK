#ifndef __DRIVE_IMPL_H__
#define __DRIVE_IMPL_H__

#include "drive.h"
#include "client_impl.h"
#include "http_client.h"

struct HiveDrive {
    int (*file_stat)(HiveDrive *, const char *file_path, char **result);
    int (*list_files)(HiveDrive *, const char *dir_path, char **result);
    int (*makedir)(HiveDrive *, const char *path);
    int (*move_file)(HiveDrive *, const char *old, const char *new);
    int (*copy_file)(HiveDrive *, const char *src_path, const char *dest_path);
    int (*delete_file)(HiveDrive *, const char *path);
    void (*close)(HiveDrive *);

    HiveClient *client;
};

int hive_drive_http_request(HiveDrive *,
    void (*setup_req)(http_client_t *req, void *user_data),
    void *user_data, http_client_t **resp);

#endif // __DRIVE_IMPL_H__
