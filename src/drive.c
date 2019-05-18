#include <crystal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <stdbool.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "drive_impl.h"

int hive_drive_getinfo(HiveDrive *drive, HiveDriveInfo *info);

int hive_drive_file_stat(HiveDrive *drv, const char *file_path, char **result)
{
    int rc;

    if (!drv || !file_path || !result || file_path[0] != '/')
        return -1;

    ref(drv);
    rc = drv->file_stat(drv, file_path, result);
    deref(drv);

    return rc;
}

int hive_drive_list_files(HiveDrive *drv, const char *dir_path, char **result)
{
    int rc;

    if (!drv || !dir_path || dir_path[0] != '/' || !result)
        return -1;

    ref(drv);
    rc = drv->list_files(drv, dir_path, result);
    deref(drv);

    return rc;
}

int hive_drive_mkdir(HiveDrive *drv, const char *path)
{
    int rc;

    if (!drv || !path || path[0] != '/' || path[1] == '\0')
        return -1;

    ref(drv);
    rc = drv->makedir(drv, path);
    deref(drv);

    return rc;
}

int hive_drive_move_file(HiveDrive *drv, const char *old, const char *new)
{
    int rc;

    if (!drv || !old || !new || old[0] != '/' ||
        new[0] == '\0' || strlen(old) > MAXPATHLEN ||
        strlen(new) > MAXPATHLEN || !strcmp(old, new))
        return -1;

    ref(drv);
    rc = drv->move_file(drv, old, new);
    deref(drv);

    return rc;
}

int hive_drive_copy_file(HiveDrive *drv, const char *src_path, const char *dest_path)
{
    int rc;

    if (!drv || !src_path || !dest_path || src_path[0] != '/' ||
        dest_path[0] == '\0' || strlen(src_path) > MAXPATHLEN ||
        strlen(dest_path) > MAXPATHLEN || !strcmp(src_path, dest_path))
        return -1;

    ref(drv);
    rc = drv->copy_file(drv, src_path, dest_path);
    deref(drv);

    return rc;
}

int hive_drive_delete_file(HiveDrive *drv, const char *path)
{
    int rc;

    if (!drv || !path || path[0] != '/' || path[1] == '\0')
        return -1;

    ref(drv);
    rc = drv->delete_file(drv, path);
    deref(drv);

    return rc;
}

void hive_drive_close(HiveDrive *drv)
{
    if (!drv)
        return;

    drv->close(drv);
}

int hive_drive_http_request(HiveDrive *drv,
    void (*setup_req)(http_client_t *, void *),
    void *user_data, http_client_t **resp)
{
    int rc;
    long resp_code;
    char *access_token;
    http_client_t *http_client;

    http_client = http_client_new();
    if (!http_client)
        return -1;

    rc = hive_client_get_access_token(drv->client, &access_token);
    if (rc) {
        http_client_close(http_client);
        return -1;
    }

    while (true) {
        setup_req(http_client, user_data);
        http_client_set_header(http_client, "Authorization", access_token);
        free(access_token);

        rc = http_client_request(http_client);
        if (rc) {
            http_client_close(http_client);
            return -1;
        }

        rc = http_client_get_response_code(http_client, &resp_code);
        if (rc) {
            http_client_close(http_client);
            return -1;
        }

        if (resp_code == 401) {
            rc = hive_client_refresh_access_token(drv->client, &access_token);
            if (rc) {
                http_client_close(http_client);
                return -1;
            }
            http_client_reset(http_client);
            continue;
        }

        *resp = http_client;
        return 0;
    }
}
