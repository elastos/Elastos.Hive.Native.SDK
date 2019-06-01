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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>

#include "onedrive_drive.h"
#include "onedrive_common.h"
#include "onedrive_client.h"
#include "http_client.h"
#include "oauth_client.h"

typedef struct OneDriveDrive {
    HiveDrive base;
    oauth_client_t *oauth_client;
    char drv_url[0];
} OneDriveDrive ;

static void get_info_setup_req(http_client_t *req, void *args)
{
    const char *url = (const char *)args;

    http_client_set_url_escape(req, url);
    http_client_set_method(req, HTTP_METHOD_GET);
    http_client_enable_response_body(req);
}

static int onedrive_drive_get_info(HiveDrive *obj, char **result)
{
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    int rc;
    long resp_code;
    onedrv_tsx_t tsx = {
        .setup_req = &get_info_setup_req,
        .user_data = drv->drv_url
    };

    rc = hive_client_perform_transaction(drv->base.client, &tsx);
    if (rc)
        return -1;

    rc = http_client_get_response_code(tsx.resp, &resp_code);
    if (rc < 0 || resp_code != 200) {
        http_client_close(tsx.resp);
        return -1;
    }

    *result = http_client_move_response_body(tsx.resp, NULL);
    http_client_close(tsx.resp);
    if (!*result)
        return -1;
    return 0;
}

static void stat_setup_req(http_client_t *req, void *args)
{
    const char *url = (const char *)args;

    http_client_set_url_escape(req, url);
    http_client_set_method(req, HTTP_METHOD_GET);
    http_client_enable_response_body(req);
}

static int onedrive_drive_file_stat(HiveDrive *obj, const char *file_path, char **result)
{
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    char url[MAXPATHLEN + 1];
    int rc;
    char *path_esc;
    long resp_code;
    http_client_t *http_cli;

    if (!strcmp(file_path, "/"))
        rc = snprintf(url, sizeof(url), "%s/root", drv->drv_url);
    else {
        http_cli = http_client_new();
        if (!http_cli)
            return -1;
        path_esc = http_client_escape(http_cli, file_path, strlen(file_path));
        http_client_close(http_cli);
        if (!path_esc)
            return -1;
        rc = snprintf(url, sizeof(url), "%s/root:%s:", drv->drv_url, path_esc);
        http_client_memory_free(path_esc);
    }
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    onedrv_tsx_t tsx = {
        .setup_req = &stat_setup_req,
        .user_data = url
    };

    rc = hive_client_perform_transaction(drv->base.client, &tsx);
    if (rc)
        return -1;

    rc = http_client_get_response_code(tsx.resp, &resp_code);
    if (rc < 0 || resp_code != 200) {
        http_client_close(tsx.resp);
        return -1;
    }

    *result = http_client_move_response_body(tsx.resp, NULL);
    http_client_close(tsx.resp);
    if (!*result)
        return -1;
    return 0;
}

static void list_setup_req(http_client_t *req, void *args)
{
    const char *url = (const char *)args;

    http_client_set_url_escape(req, url);
    http_client_set_method(req, HTTP_METHOD_GET);
    http_client_enable_response_body(req);
}

static int onedrive_drive_list_files(HiveDrive *obj, const char *dir_path, char **result)
{
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    char url[MAXPATHLEN + 1];
    int rc, val_sz, i;
    char *path_esc;
    long resp_code;
    cJSON *resp_part, *next_link, *resp, *val_part, *elem, *val;
    http_client_t *http_cli;

    if (!strcmp(dir_path, "/"))
        rc = snprintf(url, sizeof(url), "%s/root/children", drv->drv_url);
    else {
        http_cli = http_client_new();
        if (!http_cli)
            return -1;
        path_esc = http_client_escape(http_cli, dir_path, strlen(dir_path));
        http_client_close(http_cli);
        if (!path_esc)
            return -1;
        rc = snprintf(url, sizeof(url), "%s/root:%s:/children", drv->drv_url, path_esc);
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
        onedrv_tsx_t tsx = {
            .setup_req = &list_setup_req,
            .user_data = url
        };
        char *resp_body_str;

        rc = hive_client_perform_transaction(drv->base.client, &tsx);
        if (rc) {
            cJSON_Delete(resp);
            return -1;
        }

        rc = http_client_get_response_code(tsx.resp, &resp_code);
        if (rc < 0 || resp_code != 200) {
            cJSON_Delete(resp);
            http_client_close(tsx.resp);
            return -1;
        }

        resp_body_str = http_client_move_response_body(tsx.resp, NULL);
        http_client_close(tsx.resp);
        if (!resp_body_str) {
            cJSON_Delete(resp);
            return -1;
        }

        resp_part = cJSON_Parse(resp_body_str);
        free(resp_body_str);
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
            continue;
        }

        *result = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return *result ? 0 : -1;
    }
}

static char *create_request_body(char *path)
{
    cJSON *body;
    cJSON *rc;
    char *str;

    body = cJSON_CreateObject();
    if (!body) {
        hive_set_error(-1);
        return NULL;
    }

    rc = cJSON_AddStringToObject(body,  "name", basename(path));
    if (!rc)
        goto error_exit;

    rc = cJSON_AddObjectToObject(body, "folder");
    if (!rc)
        goto error_exit;

    rc = cJSON_AddStringToObject(body, "@microsoft.graph.conflictBehavior", "rename");
    if (!rc)
        goto error_exit;

    str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    if (!str) {
        hive_set_error(-1);
        return NULL;
    }

    return str;

error_exit:
    cJSON_Delete(body);
    hive_set_error(-1);
    return NULL;
}

static int onedrive_drive_mkdir(HiveDrive *base, const char *path)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    char url[ONEDRIVE_MAX_URL_LENGTH] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    char *dir;
    char *body;
    int rc;

    assert(drive);
    assert(path);
    assert(path[0] == '/');

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    dir = dirname((char *)path);
    if (strcmp(dir, "/") == 0) {
        // "mkdir" under the root directory.
        rc = snprintf(url, sizeof(url), "%s/root/children", drive->drv_url);
    } else {
        char *escaped_dir;

        escaped_dir = http_client_escape(httpc, dir, strlen(dir));
        http_client_reset(httpc);

        if (!escaped_dir) {
            hive_set_error(-1);
            goto error_exit;
        }

        rc = snprintf(url, sizeof(url), "%s/root:%s:/children", drive->drv_url,
                      escaped_dir);
        http_client_memory_free(escaped_dir);
    }

    if (rc < 0 || rc >= (int)sizeof(url)) {
        hive_set_error(-1);
        goto error_exit;
    }

    body = create_request_body((char *)path);
    if (!body)
        goto error_exit;

    http_client_set_url_escape(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_header(httpc, "Authorization", token_get_access_token(drive->oauth_client));
    http_client_set_request_body_instant(httpc, body, strlen(body));

    rc = http_client_request(httpc);
    free(body);

    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);

    if (rc < 0) {
        hive_set_error(-1);
        return -1;
    }

    if (resp_code != 201) {
        hive_set_error(-1);
        return -1;
    }

    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
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
        rc = snprintf(url, sizeof(url), "%s/root", drv->drv_url);
    else {
        http_cli = http_client_new();
        if (!http_cli)
            return -1;
        old_esc = http_client_escape(http_cli, old, strlen(old));
        http_client_close(http_cli);
        if (!old_esc)
            return -1;
        rc = snprintf(url, sizeof(url), "%s/root:%s:", drv->drv_url, old_esc);
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
            rc = snprintf(parent_dir, sizeof(parent_dir), "%s/root:%s",
                drv->drv_url + strlen(ONEDRV_ME), dst_dir);
        else
            rc = snprintf(parent_dir, sizeof(parent_dir), "%s/root:%s/%s",
                drv->drv_url + strlen(ONEDRV_ME),
                !strcmp(src_dir, "/") ? "" : src_dir, dst_dir);
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
    onedrv_tsx_t tsx = {
        .setup_req = &move_setup_req,
        .user_data = args
    };

    rc = hive_client_perform_transaction(drv->base.client, &tsx);
    free(req_body_str);
    if (rc)
        return -1;

    rc = http_client_get_response_code(tsx.resp, &resp_code);
    http_client_close(tsx.resp);
    return !rc && resp_code == 200 ? 0 : -1;
#undef is_dir
}

static size_t copy_resp_hdr_cb(
    char *buffer, size_t size, size_t nitems, void *args)
{
    char *resp_hdr_buf = (char *)(((void **)args)[0]);
    size_t resp_hdr_buf_sz = *(size_t *)(((void **)args)[1]);
    size_t total_sz = size * nitems;
    const char *loc_key = "Location: ";
    size_t loc_key_sz = strlen(loc_key);
    size_t loc_val_sz = total_sz - loc_key_sz - 2;

    if (total_sz <= loc_key_sz)
        return total_sz;

    if (loc_val_sz >= resp_hdr_buf_sz)
        return -1;

    if (memcmp(buffer, loc_key, loc_key_sz))
        return total_sz;

    memcpy(resp_hdr_buf, buffer + loc_key_sz, loc_val_sz);
    resp_hdr_buf[loc_val_sz] = '\0';
    return total_sz;
}

static void copy_setup_req(http_client_t *req, void *args)
{
    const char *url = (const char *)(((void **)args)[0]);
    const char *req_body = (const char *)(((void **)args)[1]);

    http_client_set_url_escape(req, url);
    http_client_set_method(req, HTTP_METHOD_POST);
    http_client_set_header(req, "Content-Type", "application/json");
    http_client_set_request_body_instant(req, (void *)req_body, strlen(req_body));
    http_client_set_response_header(req, &copy_resp_hdr_cb, ((void **)args) + 2);
}

static int wait_til_complete(const char *prog_qry_url)
{
    http_client_t *http_cli;
    int rc;

    http_cli = http_client_new();
    if (!http_cli)
        return -1;

    while (true) {
        cJSON *resp_body, *status;
        const char *resp_body_buf;

        http_client_set_url_escape(http_cli, prog_qry_url);
        http_client_set_method(http_cli, HTTP_METHOD_GET);
        http_client_enable_response_body(http_cli);

        rc = http_client_request(http_cli);
        if (rc) {
            http_client_close(http_cli);
            return -1;
        }

        resp_body_buf = http_client_get_response_body(http_cli);
        if (!resp_body_buf) {
            http_client_close(http_cli);
            return -1;
        }

        resp_body = cJSON_Parse(resp_body_buf);
        if (!resp_body) {
            http_client_close(http_cli);
            return -1;
        }

        status = cJSON_GetObjectItemCaseSensitive(resp_body, "status");
        if (!status || !cJSON_IsString(status)) {
            cJSON_Delete(resp_body);
            http_client_close(http_cli);
            return -1;
        }

        if (strcmp(status->valuestring, "completed")) {
            cJSON_Delete(resp_body);
            http_client_reset(http_cli);
            usleep(100 * 1000);
            continue;
        }

        http_client_close(http_cli);
        cJSON_Delete(resp_body);
        return 0;
    }
}

static int onedrive_drive_copy_file(HiveDrive *obj, const char *src_path, const char *dest_path)
{
#define is_dir(str) ((str)[strlen(str) - 1] == '/' || (str)[strlen(str) - 1] == '.')
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    char url[MAXPATHLEN + 1];
    char prog_qry_url[MAXPATHLEN + 1];
    size_t prog_qry_url_sz = sizeof(prog_qry_url);
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
        rc = snprintf(url, sizeof(url), "%s/root/copy", drv->drv_url);
    else {
        http_cli = http_client_new();
        if (!http_cli)
            return -1;
        src_path_esc = http_client_escape(http_cli, src_path, strlen(src_path));
        http_client_close(http_cli);
        if (!src_path_esc)
            return -1;
        rc = snprintf(url, sizeof(url), "%s/root:%s:/copy", drv->drv_url, src_path_esc);
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
            rc = snprintf(parent_dir, sizeof(parent_dir), "%s/root:%s",
                drv->drv_url + strlen(ONEDRV_ME), dst_dir);
        else
            rc = snprintf(parent_dir, sizeof(parent_dir), "%s/root:%s/%s",
                drv->drv_url + strlen(ONEDRV_ME),
                !strcmp(src_dir, "/") ? "" : src_dir, dst_dir);
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

    void *args[] = {url, req_body_str, prog_qry_url, &prog_qry_url_sz};
    onedrv_tsx_t tsx = {
        .setup_req = &copy_setup_req,
        .user_data = args
    };

    rc = hive_client_perform_transaction(drv->base.client, &tsx);
    free(req_body_str);
    if (rc)
        return -1;

    rc = http_client_get_response_code(tsx.resp, &resp_code);
    http_client_close(tsx.resp);
    if (rc || resp_code != 202) {
        return -1;
    }

    return wait_til_complete(prog_qry_url);
#undef is_dir
}

static int onedrive_drive_delete_file(HiveDrive *base, const char *path)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    char url[ONEDRIVE_MAX_URL_LENGTH] = {0};
    http_client_t *httpc;
    char *escaped_path;
    long resp_code = 0;
    int rc;

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    escaped_path = http_client_escape(httpc, path, strlen(path));
    http_client_reset(httpc);

    if (!escaped_path) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = snprintf(url, sizeof(url), "%s/root:%s:", drive->drv_url, escaped_path);
    http_client_memory_free(escaped_path);

    if (rc < 0 || rc >= (int)sizeof(url)) {
        hive_set_error(-1);
        goto error_exit;
    }

    http_client_set_url_escape(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_DELETE);
    http_client_set_header(httpc, "Authorization", token_get_access_token(drive->oauth_client));

    rc = http_client_request(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        return -1;
    }

    if (resp_code != 204) {
        hive_set_error(-1);
        return -1;
    }

    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static void onedrive_drive_close(HiveDrive *base)
{
    assert(base);
    deref(base);
}

HiveDrive *onedrive_drive_open(HiveClient *base, const char *driveid)
{
    OneDriveDrive *drive;
    char path[512] = {0};
    size_t url_len;

    assert(base);
    assert(driveid);

    /*
     * If param @driveid equals "default", then use the default drive.
     * otherwise, use the drive with specific driveid.
     */

    if (!strcmp(driveid, "default"))
        snprintf(path, sizeof(path), "/drive");
    else
        snprintf(path, sizeof(path), "/drives/%s", driveid);

    url_len = strlen(ONEDRV_ME) + strlen(path) + 1;
    drive = (OneDriveDrive *)rc_zalloc(sizeof(OneDriveDrive) + url_len, NULL);
    if (!drive) {
        hive_set_error(-1);
        return NULL;
    }

    snprintf(drive->drv_url, url_len, "%s/%s", ONEDRV_ME, path);

    //drv->oauth_client = onedrv_client->oauth_client;
    drive->base.client = base;

    drive->base.get_info    = &onedrive_drive_get_info;
    drive->base.file_stat   = &onedrive_drive_file_stat;
    drive->base.list_files  = &onedrive_drive_list_files;
    drive->base.makedir     = &onedrive_drive_mkdir;
    drive->base.move_file   = &onedrive_drive_move_file;
    drive->base.copy_file   = &onedrive_drive_copy_file;
    drive->base.delete_file = &onedrive_drive_delete_file;
    drive->base.close       = &onedrive_drive_close;

    return &drive->base;
}
