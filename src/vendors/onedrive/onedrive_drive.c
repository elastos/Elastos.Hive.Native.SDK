#include <limits.h>
#include <errno.h>
#include <crystal.h>
#include <stdio.h>
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include <cjson/cJSON.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <string.h>

#include "onedrive_drive.h"
#include "onedrive_common.h"
#include "http_client.h"

typedef struct OneDriveDrive {
    HiveDrive base;
    char id[0];
} OneDriveDrive ;

static void stat_setup_req(http_client_t *req, void *args)
{
    const char *url = (const char *)((void **)args)[0];
    char *resp_body = (char *)((void **)args)[1];
    size_t *resp_body_len = (size_t *)((void **)args)[2];

    http_client_set_url_escape(req, url);
    http_client_set_method(req, HTTP_METHOD_GET);
    http_client_set_response_body_instant(req, resp_body, *resp_body_len);
}

static int onedrive_drive_file_stat(HiveDrive *obj, const char *file_path, char **result)
{
#define RESP_BODY_MAX_SZ 16384
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    char url[MAXPATHLEN + 1];
    int rc;
    char *path_esc;
    char resp_body[RESP_BODY_MAX_SZ];
    size_t resp_body_len = sizeof(resp_body);
    long resp_code;
    http_client_t *http_cli;

    if (!strcmp(file_path, "/"))
        rc = snprintf(url, sizeof(url), "%s/drives/%s/root", ONEDRV_ROOT, drv->id);
    else {
        http_cli = http_client_new();
        if (!http_cli)
            return -1;
        path_esc = http_client_escape(http_cli, file_path, strlen(file_path));
        http_client_close(http_cli);
        if (!path_esc)
            return -1;
        rc = snprintf(url, sizeof(url), "%s/drives/%s/root:%s:",
            ONEDRV_ROOT, drv->id, path_esc);
        http_client_memory_free(path_esc);
    }
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    void *args[] = {url, resp_body, &resp_body_len};
    rc = hive_drive_http_request(obj, &stat_setup_req, args, &http_cli);
    if (rc)
        return -1;

    rc = http_client_get_response_code(http_cli, &resp_code);
    if (rc < 0 || resp_code != 200) {
        http_client_close(http_cli);
        return -1;
    }

    resp_body_len = http_client_get_response_body_length(http_cli);
    http_client_close(http_cli);
    if (resp_body_len == sizeof(resp_body))
        return -1;
    resp_body[resp_body_len] = '\0';

    *result = malloc(resp_body_len + 1);
    if (!*result)
        return -1;

    memcpy(*result, resp_body, resp_body_len + 1);
    return 0;
#undef RESP_BODY_MAX_SZ
}

static void list_setup_req(http_client_t *req, void *args)
{
    const char *url = (const char *)((void **)args)[0];
    char *resp_body = (char *)((void **)args)[1];
    size_t *resp_body_len = (size_t *)((void **)args)[2];

    http_client_set_url_escape(req, url);
    http_client_set_method(req, HTTP_METHOD_GET);
    http_client_set_response_body_instant(req, resp_body, *resp_body_len);
}

static int onedrive_drive_list_files(HiveDrive *obj, const char *dir_path, char **result)
{
#define RESP_BODY_MAX_SZ 16384
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    char url[MAXPATHLEN + 1];
    int rc, val_sz, i;
    char *path_esc;
    char resp_body[RESP_BODY_MAX_SZ];
    size_t resp_body_len = sizeof(resp_body);
    long resp_code;
    cJSON *resp_part, *next_link, *resp, *val_part, *elem, *val;
    http_client_t *http_cli;

    if (!strcmp(dir_path, "/"))
        rc = snprintf(url, sizeof(url), "%s/drives/%s/root/children",
            ONEDRV_ROOT, drv->id);
    else {
        http_cli = http_client_new();
        if (!http_cli)
            return -1;
        path_esc = http_client_escape(http_cli, dir_path, strlen(dir_path));
        http_client_close(http_cli);
        if (!path_esc)
            return -1;
        rc = snprintf(url, sizeof(url), "%s/drives/%s/root:%s:/children",
            ONEDRV_ROOT, drv->id, path_esc);
        http_client_memory_free(path_esc);
    }
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    resp = cJSON_CreateObject();
    if (!resp)
        return -1;

    val = cJSON_AddArrayToObject(resp, "value");
    if (!val) {
        cJSON_Delete(resp);
        return -1;
    }

    while (true) {
        void *args[] = {url, resp_body, &resp_body_len};
        rc = hive_drive_http_request(obj, &list_setup_req, args, &http_cli);
        if (rc) {
            cJSON_Delete(resp);
            return -1;
        }

        rc = http_client_get_response_code(http_cli, &resp_code);
        if (rc < 0 || resp_code != 200) {
            cJSON_Delete(resp);
            http_client_close(http_cli);
            return -1;
        }

        resp_body_len = http_client_get_response_body_length(http_cli);
        http_client_close(http_cli);
        if (resp_body_len == sizeof(resp_body)) {
            cJSON_Delete(resp);
            return -1;
        }
        resp_body[resp_body_len] = '\0';

        resp_part = cJSON_Parse(resp_body);
        if (!resp_part) {
            cJSON_Delete(resp);
            return -1;
        }

        val_part = cJSON_GetObjectItemCaseSensitive(resp_part, "value");
        if (!val_part || !cJSON_IsArray(val_part)) {
            cJSON_Delete(resp);
            cJSON_Delete(resp_part);
            return -1;
        }

        val_sz = cJSON_GetArraySize(val_part);
        for (i = 0; i < val_sz; ++i) {
            elem = cJSON_DetachItemFromArray(val_part, 0);
            cJSON_AddItemToArray(val, elem);
        }

        next_link = cJSON_GetObjectItemCaseSensitive(resp_part, "@odata.nextLink");
        cJSON_Delete(resp_part);
        if (next_link && (!cJSON_IsString(next_link) || !*next_link->valuestring)) {
            cJSON_Delete(resp);
            return -1;
        }

        if (next_link) {
            rc = snprintf(url, sizeof(url), "%s", next_link->valuestring);
            if (rc < 0 || rc >= sizeof(url)) {
                cJSON_Delete(resp);
                return -1;
            }
            resp_body_len = sizeof(resp_body);
            continue;
        }

        *result = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return *result ? 0 : -1;
    }
#undef RESP_BODY_MAX_SZ
}

static void mkdir_setup_req(http_client_t *req, void *args)
{
    const char *url = (const char *)((void **)args)[0];
    const char *req_body = (const char *)((void **)args)[1];
    char *resp_body = (char *)((void **)args)[2];
    size_t *resp_body_len = (size_t *)((void **)args)[3];

    http_client_set_url_escape(req, url);
    http_client_set_method(req, HTTP_METHOD_POST);
    http_client_set_header(req, "Content-Type", "application/json");
    http_client_set_request_body_instant(req, (void *)req_body, strlen(req_body));
    http_client_set_response_body_instant(req, resp_body, *resp_body_len);
}

static int onedrive_drive_mkdir(HiveDrive *obj, const char *path)
{
#define RESP_BODY_MAX_SZ 2048
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    char url[MAXPATHLEN + 1];
    cJSON *req_body;
    int rc;
    char resp_body[RESP_BODY_MAX_SZ];
    size_t resp_body_len = sizeof(resp_body);
    long resp_code;
    char *req_body_str;
    char *dir;
    char *dir_esc;
    http_client_t *http_cli;

    dir = dirname((char *)path);
    if (!strcmp(dir, "/"))
        rc = snprintf(url, sizeof(url), "%s/drives/%s/root/children",
            ONEDRV_ROOT, drv->id);
    else {
        http_cli = http_client_new();
        if (!http_cli)
            return -1;
        dir_esc = http_client_escape(http_cli, dir, strlen(dir));
        http_client_close(http_cli);
        if (!dir_esc)
            return -1;
        rc = snprintf(url, sizeof(url), "%s/drives/%s/root:%s:/children",
            ONEDRV_ROOT, drv->id, dir_esc);
        http_client_memory_free(dir_esc);
    }
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    req_body = cJSON_CreateObject();
    if (!req_body)
        return -1;

    if (!cJSON_AddStringToObject(req_body, "name", basename((char *)path))) {
        cJSON_Delete(req_body);
        return -1;
    }

    if (!cJSON_AddObjectToObject(req_body, "folder")) {
        cJSON_Delete(req_body);
        return -1;
    }

    if (!cJSON_AddStringToObject(req_body, "@microsoft.graph.conflictBehavior", "rename")) {
        cJSON_Delete(req_body);
        return -1;
    }

    req_body_str = cJSON_PrintUnformatted(req_body);
    cJSON_Delete(req_body);
    if (!req_body_str)
        return -1;

    void *args[] = {url, req_body_str, resp_body, &resp_body_len};
    rc = hive_drive_http_request(obj, &mkdir_setup_req, args, &http_cli);
    free(req_body_str);
    if (rc)
        return -1;

    rc = http_client_get_response_code(http_cli, &resp_code);
    http_client_close(http_cli);
    return !rc && resp_code == 201 ? 0 : -1;
#undef RESP_BODY_MAX_SZ
}

static void move_setup_req(http_client_t *req, void *args)
{
    const char *url = (const char *)((void **)args)[0];
    const char *req_body = (const char *)((void **)args)[1];

    http_client_set_url_escape(req, url);
    http_client_set_method(req, HTTP_METHOD_PATCH);
    http_client_set_header(req, "Content-Type", "application/json");
    http_client_set_request_body_instant(req, (void *)req_body, strlen(req_body));
}

static int onedrive_drive_move_file(HiveDrive *obj, const char *old, const char *new)
{
#define is_dir(str) ((str)[strlen(str) - 1] == '/' || (str)[strlen(str) - 1] == '.')
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    char url[MAXPATHLEN + 1];
    int rc;
    http_client_t *http_cli;
    char *old_esc;
    cJSON *req_body, *parent_ref;
    char src_dir[MAXPATHLEN + 1];
    char src_base[MAXPATHLEN + 1];
    const char *dst_dir;
    const char *dst_base;
    bool dir_eq = false, base_eq = false;
    char *req_body_str;
    long resp_code;

    strcpy(src_dir, dirname((char *)old));
    strcpy(src_base, basename((char *)old));
    dst_dir = is_dir(new) ? new : dirname((char *)new);
    dst_base = is_dir(new) ? src_base : basename((char *)new);

    if (!strcmp(src_dir, dst_dir) || !strcmp(dst_dir, ".") || !strcmp(dst_dir, "./"))
        dir_eq = true;
    if (!strcmp(src_base, dst_base))
        base_eq = true;
    if (dir_eq && base_eq)
        return -1;

    if (!strcmp(old, "/"))
        rc = snprintf(url, sizeof(url), "%s/drives/%s/root",
            ONEDRV_ROOT, drv->id);
    else {
        http_cli = http_client_new();
        if (!http_cli)
            return -1;
        old_esc = http_client_escape(http_cli, old, strlen(old));
        http_client_close(http_cli);
        if (!old_esc)
            return -1;
        rc = snprintf(url, sizeof(url), "%s/drives/%s/root:%s:",
            ONEDRV_ROOT, drv->id, old_esc);
        http_client_memory_free(old_esc);
    }
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    req_body = cJSON_CreateObject();
    if (!req_body)
        return -1;

    if (!dir_eq) {
        char parent_dir[MAXPATHLEN + 1];

        if (new[0] == '/')
            rc = snprintf(parent_dir, sizeof(parent_dir), "/drives/%s/root:%s",
                drv->id, dst_dir);
        else
            rc = snprintf(parent_dir, sizeof(parent_dir), "/drives/%s/root:%s/%s",
                drv->id, !strcmp(src_dir, "/") ? "" : src_dir, dst_dir);
        if (rc < 0 || rc >= sizeof(parent_dir)) {
            cJSON_Delete(req_body);
            return -1;
        }
        if (parent_dir[strlen(parent_dir) - 1] == '/')
            parent_dir[strlen(parent_dir) - 1] = '\0';
        parent_ref = cJSON_AddObjectToObject(req_body, "parentReference");
        if (!parent_ref) {
            cJSON_Delete(req_body);
            return -1;
        }
        if (!cJSON_AddStringToObject(parent_ref, "path", parent_dir)) {
            cJSON_Delete(req_body);
            return -1;
        }
    }

    if (!base_eq) {
        if (!cJSON_AddStringToObject(req_body, "name", dst_base)) {
            cJSON_Delete(req_body);
            return -1;
        }
    }

    req_body_str = cJSON_PrintUnformatted(req_body);
    cJSON_Delete(req_body);
    if (!req_body_str)
        return -1;

    void *args[] = {url, req_body_str};
    rc = hive_drive_http_request(obj, &move_setup_req, args, &http_cli);
    free(req_body_str);
    if (rc)
        return -1;

    rc = http_client_get_response_code(http_cli, &resp_code);
    http_client_close(http_cli);
    return !rc && resp_code == 200 ? 0 : -1;
#undef is_dir
}

static void copy_setup_req(http_client_t *req, void *args)
{
    const char *url = (const char *)((void **)args)[0];
    const char *req_body = (const char *)((void **)args)[1];

    http_client_set_url_escape(req, url);
    http_client_set_method(req, HTTP_METHOD_POST);
    http_client_set_header(req, "Content-Type", "application/json");
    http_client_set_request_body_instant(req, (void *)req_body, strlen(req_body));
}

static int onedrive_drive_copy_file(HiveDrive *obj, const char *src_path, const char *dest_path)
{
#define is_dir(str) ((str)[strlen(str) - 1] == '/' || (str)[strlen(str) - 1] == '.')
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    char url[MAXPATHLEN + 1];
    int rc;
    http_client_t *http_cli;
    char *src_path_esc;
    cJSON *req_body, *parent_ref;
    char src_dir[MAXPATHLEN + 1];
    char src_base[MAXPATHLEN + 1];
    const char *dst_dir;
    const char *dst_base;
    bool dir_eq = false, base_eq = false;
    char *req_body_str;
    long resp_code;

    strcpy(src_dir, dirname((char *)src_path));
    strcpy(src_base, basename((char *)src_path));
    dst_dir = is_dir(dest_path) ? dest_path : dirname((char *)dest_path);
    dst_base = is_dir(dest_path) ? src_base : basename((char *)dest_path);

    if (!strcmp(src_dir, dst_dir) || !strcmp(dst_dir, ".") || !strcmp(dst_dir, "./"))
        dir_eq = true;
    if (!strcmp(src_base, dst_base))
        base_eq = true;
    if (dir_eq && base_eq)
        return -1;

    if (!strcmp(src_path, "/"))
        rc = snprintf(url, sizeof(url), "%s/drives/%s/root/copy",
            ONEDRV_ROOT, drv->id);
    else {
        http_cli = http_client_new();
        if (!http_cli)
            return -1;
        src_path_esc = http_client_escape(http_cli, src_path, strlen(src_path));
        http_client_close(http_cli);
        if (!src_path_esc)
            return -1;
        rc = snprintf(url, sizeof(url), "%s/drives/%s/root:%s:/copy",
            ONEDRV_ROOT, drv->id, src_path_esc);
        http_client_memory_free(src_path_esc);
    }
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    req_body = cJSON_CreateObject();
    if (!req_body)
        return -1;

    if (!dir_eq) {
        char parent_dir[MAXPATHLEN + 1];

        if (dest_path[0] == '/')
            rc = snprintf(parent_dir, sizeof(parent_dir), "/drives/%s/root:%s",
                drv->id, dst_dir);
        else
            rc = snprintf(parent_dir, sizeof(parent_dir), "/drives/%s/root:%s/%s",
                drv->id, !strcmp(src_dir, "/") ? "" : src_dir, dst_dir);
        if (rc < 0 || rc >= sizeof(parent_dir)) {
            cJSON_Delete(req_body);
            return -1;
        }
        if (parent_dir[strlen(parent_dir) - 1] == '/')
            parent_dir[strlen(parent_dir) - 1] = '\0';
        parent_ref = cJSON_AddObjectToObject(req_body, "parentReference");
        if (!parent_ref) {
            cJSON_Delete(req_body);
            return -1;
        }
        if (!cJSON_AddStringToObject(parent_ref, "path", parent_dir)) {
            cJSON_Delete(req_body);
            return -1;
        }
    }

    if (!base_eq) {
        if (!cJSON_AddStringToObject(req_body, "name", dst_base)) {
            cJSON_Delete(req_body);
            return -1;
        }
    }

    req_body_str = cJSON_PrintUnformatted(req_body);
    cJSON_Delete(req_body);
    if (!req_body_str)
        return -1;

    void *args[] = {url, req_body_str};
    rc = hive_drive_http_request(obj, &copy_setup_req, args, &http_cli);
    free(req_body_str);
    if (rc)
        return -1;

    rc = http_client_get_response_code(http_cli, &resp_code);
    http_client_close(http_cli);
    return !rc && resp_code == 202 ? 0 : -1;
#undef is_dir
}

static void delete_setup_req(http_client_t *req, void *args)
{
    const char *url = (const char *)((void **)args)[0];

    http_client_set_url_escape(req, url);
    http_client_set_method(req, HTTP_METHOD_DELETE);
}

static int onedrive_drive_delete_file(HiveDrive *obj, const char *path)
{
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    char url[MAXPATHLEN + 1];
    int rc;
    long resp_code;
    http_client_t *http_cli;
    char *path_esc;

    http_cli = http_client_new();
    if (!http_cli)
        return -1;

    path_esc = http_client_escape(http_cli, path, strlen(path));
    http_client_close(http_cli);
    if (!path_esc)
        return -1;

    rc = snprintf(url, sizeof(url), "%s/drives/%s/root:%s:",
        ONEDRV_ROOT, drv->id, path_esc);
    http_client_memory_free(path_esc);
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    void *args[] = {url};
    rc = hive_drive_http_request(obj, delete_setup_req, args, &http_cli);
    if (rc)
        return -1;

    rc = http_client_get_response_code(http_cli, &resp_code);
    http_client_close(http_cli);
    return !rc && resp_code == 204 ? 0 : -1;
}

static void onedrive_drive_close(HiveDrive *drv)
{
    deref(drv);
}

static void onedrive_drive_destructor(void *obj)
{
    OneDriveDrive *drv = (OneDriveDrive *)obj;

    deref(drv->base.client);
}

HiveDrive *onedrive_drive_open(HiveClient *onedrv_client, const char *drive_id)
{
    OneDriveDrive *drv = (OneDriveDrive *)rc_zalloc(
        sizeof(OneDriveDrive) + strlen(drive_id) + 1, &onedrive_drive_destructor);
    if (!drv)
        return NULL;

    strcpy(drv->id, drive_id);

    ref(onedrv_client);
    drv->base.client = onedrv_client;

    drv->base.file_stat   = &onedrive_drive_file_stat;
    drv->base.list_files  = &onedrive_drive_list_files;
    drv->base.makedir     = &onedrive_drive_mkdir;
    drv->base.move_file   = &onedrive_drive_move_file;
    drv->base.copy_file   = &onedrive_drive_copy_file;
    drv->base.delete_file = &onedrive_drive_delete_file;
    drv->base.close       = &onedrive_drive_close;

    return (HiveDrive *)drv;
}
