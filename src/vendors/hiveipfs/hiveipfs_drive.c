#include <stddef.h>
#include <assert.h>
#include <crystal.h>
#include <stdlib.h>

#include "hiveipfs_drive.h"
#include "hiveipfs_client.h"

typedef struct ipfs_drive {
    HiveDrive base;
} ipfs_drv_t;

#define FILES_API "/api/v0/files"

static void publish_setup_req(http_client_t *req, void *args)
{
    const char *req_path = (const char *)((void **)args)[0];
    const char *path = (const char *)((void **)args)[1];

    http_client_set_path(req, req_path);
    http_client_set_query(req, "path", path);
    http_client_set_method(req, HTTP_METHOD_POST);
}

static int ipfs_drive_publish(ipfs_drv_t *drv, const char *path)
{
    const char *req_path = "/api/v0/name/publish";
    void *args[] = {(void *)req_path, (void *)path};
    ipfs_tsx_t tsx = {
        .setup_req = &publish_setup_req,
        .user_data = args
    };
    int rc;
    long resp_code;

    rc = hive_client_perform_transaction(drv->base.client, &tsx);
    if (rc)
        return -1;

    rc = http_client_get_response_code(tsx.resp, &resp_code);
    http_client_close(tsx.resp);
    if (rc || resp_code != 200)
        return -1;

    return 0;
}

static void get_info_setup_req(http_client_t *req, void *args)
{
    const char *req_path = (const char *)args;

    http_client_set_path(req, req_path);
    http_client_set_query(req, "path", "/");
    http_client_set_method(req, HTTP_METHOD_POST);
    http_client_enable_response_body(req);
}

static int ipfs_drive_get_info(HiveDrive *obj, char **result)
{
    ipfs_drv_t *drv = (ipfs_drv_t *)obj;
    const char *req_path = FILES_API"/stat";
    ipfs_tsx_t tsx = {
        .setup_req = &get_info_setup_req,
        .user_data = (void *)req_path
    };
    int rc;
    long resp_code;

    rc = ipfs_client_synchronize(drv->base.client);
    if (rc)
        return -1;

    rc = hive_client_perform_transaction(drv->base.client, &tsx);
    if (rc)
        return -1;

    rc = http_client_get_response_code(tsx.resp, &resp_code);
    if (rc || resp_code != 200) {
        http_client_close(tsx.resp);
        return -1;
    }

    *result = http_client_move_response_body(tsx.resp, NULL);
    http_client_close(tsx.resp);
    if (!*result)
        return -1;

    return 0;
}

static void file_stat_setup_req(http_client_t *req, void *args)
{
    const char *req_path = (const char *)((void **)args)[0];
    const char *file_path = (const char *)((void **)args)[1];

    http_client_set_path(req, req_path);
    http_client_set_query(req, "path", file_path);
    http_client_set_method(req, HTTP_METHOD_POST);
    http_client_enable_response_body(req);
}

static int ipfs_drive_file_stat(HiveDrive *obj, const char *file_path, char **result)
{
    ipfs_drv_t *drv = (ipfs_drv_t *)obj;
    const char *req_path = FILES_API"/stat";
    void *args[] = {(void *)req_path, (void *)file_path};
    ipfs_tsx_t tsx = {
        .setup_req = &file_stat_setup_req,
        .user_data = args
    };
    int rc;
    long resp_code;

    rc = ipfs_client_synchronize(drv->base.client);
    if (rc)
        return -1;

    rc = hive_client_perform_transaction(drv->base.client, &tsx);
    if (rc)
        return -1;

    rc = http_client_get_response_code(tsx.resp, &resp_code);
    if (rc || resp_code != 200) {
        http_client_close(tsx.resp);
        return -1;
    }

    *result = http_client_move_response_body(tsx.resp, NULL);
    http_client_close(tsx.resp);
    if (!*result)
        return -1;

    return 0;
}

static void list_files_setup_req(http_client_t *req, void *args)
{
    const char *req_path = (const char *)((void **)args)[0];
    const char *dir_path = (const char *)((void **)args)[1];

    http_client_set_path(req, req_path);
    http_client_set_query(req, "path", dir_path);
    http_client_set_method(req, HTTP_METHOD_POST);
    http_client_enable_response_body(req);
}

static int ipfs_drive_list_files(HiveDrive *obj, const char *dir_path, char **result)
{
    ipfs_drv_t *drv = (ipfs_drv_t *)obj;
    const char *req_path = FILES_API"/ls";
    void *args[] = {(void *)req_path, (void *)dir_path};
    ipfs_tsx_t tsx = {
        .setup_req = &list_files_setup_req,
        .user_data = args
    };
    int rc;
    long resp_code;

    rc = ipfs_client_synchronize(drv->base.client);
    if (rc)
        return -1;

    rc = hive_client_perform_transaction(drv->base.client, &tsx);
    if (rc)
        return -1;

    rc = http_client_get_response_code(tsx.resp, &resp_code);
    if (rc || resp_code != 200) {
        http_client_close(tsx.resp);
        return -1;
    }

    *result = http_client_move_response_body(tsx.resp, NULL);
    http_client_close(tsx.resp);
    if (!*result)
        return -1;

    return 0;
}

static void mkdir_setup_req(http_client_t *req, void *args)
{
    const char *req_path = (const char *)((void **)args)[0];
    const char *dir_path = (const char *)((void **)args)[1];

    http_client_set_path(req, req_path);
    http_client_set_query(req, "path", dir_path);
    http_client_set_query(req, "parents", "true");
    http_client_set_method(req, HTTP_METHOD_POST);
}

static int ipfs_drive_makedir(HiveDrive *obj, const char *path)
{
    ipfs_drv_t *drv = (ipfs_drv_t *)obj;
    const char *req_path = FILES_API"/mkdir";
    void *args[] = {(void *)req_path, (void *)path};
    ipfs_tsx_t tsx = {
        .setup_req = &mkdir_setup_req,
        .user_data = args
    };
    int rc;
    long resp_code;

    rc = ipfs_client_synchronize(drv->base.client);
    if (rc)
        return -1;

    rc = hive_client_perform_transaction(drv->base.client, &tsx);
    if (rc)
        return -1;

    rc = http_client_get_response_code(tsx.resp, &resp_code);
    http_client_close(tsx.resp);
    if (rc || resp_code != 200)
        return -1;

    rc = ipfs_drive_publish(drv, "/");
    return rc;
}

static void move_file_setup_req(http_client_t *req, void *args)
{
    const char *req_path = (const char *)((void **)args)[0];
    const char *old = (const char *)((void **)args)[1];
    const char *new = (const char *)((void **)args)[2];

    http_client_set_path(req, req_path);
    http_client_set_query(req, "source", old);
    http_client_set_query(req, "dest", new);
    http_client_set_method(req, HTTP_METHOD_POST);
}

static int ipfs_drive_move_file(HiveDrive *obj, const char *old, const char *new)
{
    ipfs_drv_t *drv = (ipfs_drv_t *)obj;
    const char *req_path = FILES_API"/mv";
    void *args[] = {(void *)req_path, (void *)old, (void *)new};
    ipfs_tsx_t tsx = {
        .setup_req = &move_file_setup_req,
        .user_data = args
    };
    int rc;
    long resp_code;

    rc = ipfs_client_synchronize(drv->base.client);
    if (rc)
        return -1;

    rc = hive_client_perform_transaction(drv->base.client, &tsx);
    if (rc)
        return -1;

    rc = http_client_get_response_code(tsx.resp, &resp_code);
    http_client_close(tsx.resp);
    if (rc || resp_code != 200)
        return -1;

    rc = ipfs_drive_publish(drv, "/");
    return rc;
}

static void copy_file_setup_req(http_client_t *req, void *args)
{
    const char *req_path = (const char *)((void **)args)[0];
    const char *src_path = (const char *)((void **)args)[1];
    const char *dest_path = (const char *)((void **)args)[2];

    http_client_set_path(req, req_path);
    http_client_set_query(req, "source", src_path);
    http_client_set_query(req, "dest", dest_path);
    http_client_set_method(req, HTTP_METHOD_POST);
}

static int ipfs_drive_copy_file(HiveDrive *obj, const char *src_path, const char *dest_path)
{
    ipfs_drv_t *drv = (ipfs_drv_t *)obj;
    const char *req_path = FILES_API"/cp";
    void *args[] = {(void *)req_path, (void *)src_path, (void *)dest_path};
    ipfs_tsx_t tsx = {
        .setup_req = &copy_file_setup_req,
        .user_data = args
    };
    int rc;
    long resp_code;

    rc = ipfs_client_synchronize(drv->base.client);
    if (rc)
        return -1;

    rc = hive_client_perform_transaction(drv->base.client, &tsx);
    if (rc)
        return -1;

    rc = http_client_get_response_code(tsx.resp, &resp_code);
    http_client_close(tsx.resp);
    if (rc || resp_code != 200)
        return -1;

    rc = ipfs_drive_publish(drv, "/");
    return rc;
}

static void delete_file_setup_req(http_client_t *req, void *args)
{
    const char *req_path = (const char *)((void **)args)[0];
    const char *path = (const char *)((void **)args)[1];

    http_client_set_path(req, req_path);
    http_client_set_query(req, "path", path);
    http_client_set_query(req, "recursive", "true");
    http_client_set_method(req, HTTP_METHOD_POST);
}

static int ipfs_drive_delete_file(HiveDrive *obj, const char *path)
{
    ipfs_drv_t *drv = (ipfs_drv_t *)obj;
    const char *req_path = FILES_API"/rm";
    void *args[] = {(void *)req_path, (void *)path};
    ipfs_tsx_t tsx = {
        .setup_req = &delete_file_setup_req,
        .user_data = args
    };
    int rc;
    long resp_code;

    rc = ipfs_client_synchronize(drv->base.client);
    if (rc)
        return -1;

    rc = hive_client_perform_transaction(drv->base.client, &tsx);
    if (rc)
        return -1;

    rc = http_client_get_response_code(tsx.resp, &resp_code);
    http_client_close(tsx.resp);
    if (rc || resp_code != 200)
        return -1;

    rc = ipfs_drive_publish(drv, "/");
    return rc;
}

static void ipfs_drive_close(HiveDrive *obj)
{
    deref(obj);
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
