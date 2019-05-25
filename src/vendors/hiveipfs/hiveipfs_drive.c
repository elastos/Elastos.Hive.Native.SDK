#include <stddef.h>
#include <assert.h>
#include <crystal.h>

#include "hiveipfs_drive.h"
#include "hiveipfs_client.h"

typedef struct ipfs_drive {
    HiveDrive base;
} ipfs_drv_t;

#define FILES_API "/api/v0/files"

static void get_info_setup_req(http_client_t *req, void *args)
{
    const char *req_path = (const char *)args;

    http_client_set_path(req, req_path);
    http_client_set_method(req, HTTP_METHOD_POST);
    http_client_enable_response_body(req);
}

static int ipfs_drive_get_info(HiveDrive *obj, char **result)
{
    ipfs_drv_t *drv = (ipfs_drv_t *)obj;
    const char *resp_body;
    size_t resp_body_len;
    const char *req_path = FILES_API"/stat";
    ipfs_tsx_t tsx = {
        .setup_req = &get_info_setup_req,
        .user_data = (void *)req_path
    };
    int rc;

    rc = hive_client_perform_transaction(drv->base.client, &tsx);
    if (rc)
        return -1;

    return 0;
}

static int ipfs_drive_file_stat(HiveDrive *obj, const char *file_path, char **result)
{
    return -1;
}

static int ipfs_drive_list_files(HiveDrive *obj, const char *dir_path, char **result)
{
    return -1;
}

static int ipfs_drive_makedir(HiveDrive *obj, const char *path)
{
    return -1;
}

static int ipfs_drive_move_file(HiveDrive *obj, const char *old, const char *new)
{
    return -1;
}

static int ipfs_drive_copy_file(HiveDrive *obj, const char *src_path, const char *dest_path)
{
    return -1;

}

static int ipfs_drive_delete_file(HiveDrive *obj, const char *path)
{
    return -1;

}

static void ipfs_drive_close(HiveDrive *obj)
{
}

HiveDrive *ipfs_drive_open(HiveClient *client)
{
    ipfs_drv_t *drv;

    assert(client);

    drv = (ipfs_drv_t *)rc_zalloc(sizeof(ipfs_drv_t), NULL);
    if (!drv)
        return NULL;

    drv->base.client      = client;
    drv->base.get_info    = &ipfs_drive_get_info;
    drv->base.file_stat   = &ipfs_drive_file_stat;
    drv->base.list_files  = &ipfs_drive_list_files;
    drv->base.makedir     = &ipfs_drive_makedir;
    drv->base.move_file   = &ipfs_drive_move_file;
    drv->base.copy_file   = &ipfs_drive_copy_file;
    drv->base.delete_file = &ipfs_drive_delete_file;
    drv->base.close       = &ipfs_drive_close;

    return (HiveDrive *)drv;
}
