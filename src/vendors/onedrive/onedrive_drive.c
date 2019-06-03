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
    oauth_client_t *credential;
    char drv_url[0];
} OneDriveDrive ;

static int onedrive_drive_get_info(HiveDrive *obj, char **result)
{
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    int rc;
    long resp_code;
    http_client_t *httpc = NULL;
    char *access_token = NULL;

    rc = oauth_client_get_access_token(drv->credential, &access_token);
    if (rc) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        goto error_exit;
    }

    http_client_set_url_escape(httpc, drv->drv_url);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_enable_response_body(httpc);
    http_client_set_header(httpc, "Authorization", access_token);
    free(access_token);
    access_token = NULL;

    rc = http_client_request(httpc);
    if (rc) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (resp_code != 200) {
        if (resp_code == 401)
            oauth_client_set_expired(drv->credential);
        hive_set_error(-1);
        goto error_exit;
    }

    *result = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);
    httpc = NULL;

    if (!*result) {
        hive_set_error(-1);
        goto error_exit;
    }

    return 0;

error_exit:
    if (httpc)
        http_client_close(httpc);
    if (access_token)
        free(access_token);
    return -1;
}

static int onedrive_drive_file_stat(HiveDrive *obj, const char *file_path, char **result)
{
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    char url[MAXPATHLEN + 1];
    int rc;
    long resp_code;
    http_client_t *httpc = NULL;
    char *access_token = NULL;

    rc = oauth_client_get_access_token(drv->credential, &access_token);
    if (rc) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (!strcmp(file_path, "/"))
        rc = snprintf(url, sizeof(url), "%s/root", drv->drv_url);
    else {
        char *path_esc;

        path_esc = http_client_escape(httpc, file_path, strlen(file_path));
        http_client_reset(httpc);
        if (!path_esc) {
            hive_set_error(-1);
            goto error_exit;
        }

        rc = snprintf(url, sizeof(url), "%s/root:%s:", drv->drv_url, path_esc);
        http_client_memory_free(path_esc);
    }

    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        goto error_exit;
    }

    http_client_set_url_escape(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_enable_response_body(httpc);
    http_client_set_header(httpc, "Authorization", access_token);
    free(access_token);
    access_token = NULL;

    rc = http_client_request(httpc);
    if (rc) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (resp_code != 200) {
        if (resp_code == 401)
            oauth_client_set_expired(drv->credential);
        hive_set_error(-1);
        goto error_exit;
    }

    *result = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);
    httpc = NULL;
    if (!*result) {
        hive_set_error(-1);
        goto error_exit;
    }

    return 0;

error_exit:
    if (httpc)
        http_client_close(httpc);
    if (access_token)
        free(access_token);
    return -1;
}

static int onedrive_drive_list_files(HiveDrive *obj, const char *dir_path, char **result)
{
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    char url[MAXPATHLEN + 1];
    int rc, val_sz, i;
    long resp_code;
    cJSON *resp_part = NULL, *next_link, *resp = NULL, *val_part, *elem, *val;
    http_client_t *httpc = NULL;
    char *access_token = NULL;
    char *resp_body_str = NULL;

    rc = oauth_client_get_access_token(drv->credential, &access_token);
    if (rc) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (!strcmp(dir_path, "/"))
        rc = snprintf(url, sizeof(url), "%s/root/children", drv->drv_url);
    else {
        char *path_esc;

        path_esc = http_client_escape(httpc, dir_path, strlen(dir_path));
        http_client_reset(httpc);
        if (!path_esc) {
            hive_set_error(-1);
            goto error_exit;
        }

        rc = snprintf(url, sizeof(url), "%s/root:%s:/children", drv->drv_url, path_esc);
        http_client_memory_free(path_esc);
    }

    if (rc < 0 || rc >= sizeof(url))
        return -1;

    resp = cJSON_CreateObject();
    if (!resp) {
        hive_set_error(-1);
        goto error_exit;
    }

    val = cJSON_AddArrayToObject(resp, "value");
    if (!val) {
        hive_set_error(-1);
        goto error_exit;
    }

    while (true) {
        http_client_set_url_escape(httpc, url);
        http_client_set_method(httpc, HTTP_METHOD_GET);
        http_client_enable_response_body(httpc);
        http_client_set_header(httpc, "Authorization", access_token);

        rc = http_client_request(httpc);
        if (rc) {
            hive_set_error(-1);
            goto error_exit;
        }

        rc = http_client_get_response_code(httpc, &resp_code);
        if (rc < 0) {
            hive_set_error(-1);
            goto error_exit;
        }

        if (resp_code != 200) {
            if (resp_code == 401)
                oauth_client_set_expired(drv->credential);
            hive_set_error(-1);
            goto error_exit;
        }

        resp_body_str = http_client_move_response_body(httpc, NULL);

        if (!resp_body_str) {
            hive_set_error(-1);
            goto error_exit;
        }

        resp_part = cJSON_Parse(resp_body_str);
        free(resp_body_str);
        resp_body_str = NULL;
        if (!resp_part) {
            hive_set_error(-1);
            goto error_exit;
        }

        val_part = cJSON_GetObjectItemCaseSensitive(resp_part, "value");
        if (!val_part || !cJSON_IsArray(val_part)) {
            hive_set_error(-1);
            goto error_exit;
        }

        val_sz = cJSON_GetArraySize(val_part);
        for (i = 0; i < val_sz; ++i) {
            elem = cJSON_DetachItemFromArray(val_part, 0);
            cJSON_AddItemToArray(val, elem);
        }

        next_link = cJSON_GetObjectItemCaseSensitive(resp_part, "@odata.nextLink");
        cJSON_Delete(resp_part);
        resp_part = NULL;
        if (next_link && (!cJSON_IsString(next_link) || !*next_link->valuestring)) {
            hive_set_error(-1);
            goto error_exit;
        }

        if (next_link) {
            rc = snprintf(url, sizeof(url), "%s", next_link->valuestring);
            if (rc < 0 || rc >= sizeof(url)) {
                hive_set_error(-1);
                goto error_exit;
            }
            http_client_reset(httpc);
            continue;
        }

        http_client_close(httpc);
        httpc = NULL;
        *result = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        resp = NULL;
        free(access_token);
        access_token = NULL;

        if (!*result) {
            hive_set_error(-1);
            goto error_exit;
        }

        return 0;
    }

error_exit:
    if (resp_part)
        cJSON_Delete(resp);
    if (resp)
        cJSON_Delete(resp);
    if (httpc)
        http_client_close(httpc);
    if (access_token)
        free(access_token) ;
    if (resp_body_str)
        free(resp_body_str);
    return -1;
}

static int onedrive_drive_mkdir(HiveDrive *base, const char *path)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    char url[ONEDRIVE_MAX_URL_LENGTH] = {0};
    http_client_t *httpc = NULL;
    long resp_code = 0;
    cJSON *req_body = NULL;
    char *req_body_str = NULL;
    int rc;
    char *access_token = NULL;
    char *dir;

    assert(drive);
    assert(path);
    assert(path[0] == '/');

    rc = oauth_client_get_access_token(drive->credential, &access_token);
    if (rc) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        goto error_exit;
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

    req_body = cJSON_CreateObject();
    if (!req_body) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (!cJSON_AddStringToObject(req_body, "name", basename((char *)path))) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (!cJSON_AddObjectToObject(req_body, "folder")) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (!cJSON_AddStringToObject(req_body, "@microsoft.graph.conflictBehavior", "rename")) {
        hive_set_error(-1);
        goto error_exit;
    }

    req_body_str = cJSON_PrintUnformatted(req_body);
    cJSON_Delete(req_body);
    req_body = NULL;
    if (!req_body_str) {
        hive_set_error(-1);
        goto error_exit;
    }

    http_client_set_url_escape(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_request_body_instant(httpc, req_body_str, strlen(req_body_str));
    http_client_set_header(httpc, "Authorization", access_token);
    free(access_token);
    access_token = NULL;

    rc = http_client_request(httpc);
    free(req_body_str);
    req_body_str = NULL;

    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);

    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (resp_code != 201) {
        if (resp_code == 401)
            oauth_client_set_expired(drive->credential);
        hive_set_error(-1);
        goto error_exit;
    }

    return 0;

error_exit:
    if (httpc)
        http_client_close(httpc);
    if (req_body)
        cJSON_Delete(req_body);
    if (req_body_str)
        free(req_body_str);
    if (access_token)
        free(access_token);
    return -1;
}

static int onedrive_drive_move_file(HiveDrive *obj, const char *old, const char *new)
{
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    char url[MAXPATHLEN + 1];
    int rc;
    http_client_t *httpc = NULL;
    cJSON *req_body = NULL, *parent_ref;
    char *req_body_str = NULL;
    char *access_token = NULL;
    long resp_code;
    char parent_dir[MAXPATHLEN + 1];

    rc = oauth_client_get_access_token(drv->credential, &access_token);
    if (rc) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (!strcmp(old, "/"))
        rc = snprintf(url, sizeof(url), "%s/root", drv->drv_url);
    else {
        char *old_esc;

        old_esc = http_client_escape(httpc, old, strlen(old));
        http_client_reset(httpc);
        if (!old_esc) {
            hive_set_error(-1);
            goto error_exit;
        }

        rc = snprintf(url, sizeof(url), "%s/root:%s:", drv->drv_url, old_esc);
        http_client_memory_free(old_esc);
    }

    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        goto error_exit;
    }

    req_body = cJSON_CreateObject();
    if (!req_body) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = snprintf(parent_dir, sizeof(parent_dir), "%s/root:%s",
        drv->drv_url + strlen(ONEDRV_ME), dirname((char *)new));
    if (rc < 0 || rc >= sizeof(parent_dir)) {
        hive_set_error(-1);
        goto error_exit;
    }

    parent_ref = cJSON_AddObjectToObject(req_body, "parentReference");
    if (!parent_ref) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (!cJSON_AddStringToObject(parent_ref, "path", parent_dir)) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (!cJSON_AddStringToObject(req_body, "name", basename((char *)new))) {
        hive_set_error(-1);
        goto error_exit;
    }

    req_body_str = cJSON_PrintUnformatted(req_body);
    cJSON_Delete(req_body);
    req_body = NULL;
    if (!req_body_str) {
        hive_set_error(-1);
        goto error_exit;
    }

    http_client_set_url_escape(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_PATCH);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_request_body_instant(httpc, (void *)req_body_str, strlen(req_body_str));
    http_client_set_header(httpc, "Authorization", access_token);
    free(access_token);
    access_token = NULL;

    rc = http_client_request(httpc);
    free(req_body_str);
    req_body_str = NULL;
    if (rc) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    httpc = NULL;
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (resp_code != 200) {
        if (resp_code == 401)
            oauth_client_set_expired(drv->credential);
        hive_set_error(-1);
        goto error_exit;
    }

    return 0;

error_exit:
    if (httpc)
        http_client_close(httpc);
    if (req_body)
        cJSON_Delete(req_body);
    if (req_body_str)
        free(req_body_str);
    if (access_token)
        free(access_token);
    return -1;
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
    OneDriveDrive *drv = (OneDriveDrive *)obj;
    char url[MAXPATHLEN + 1];
    int rc;
    cJSON *req_body = NULL, *parent_ref;
    char *req_body_str = NULL;
    long resp_code;
    char parent_dir[MAXPATHLEN + 1];
    http_client_t *httpc = NULL;
    char *access_token = NULL;
    char prog_qry_url[MAXPATHLEN + 1];
    size_t prog_qry_url_sz = sizeof(prog_qry_url);

    rc = oauth_client_get_access_token(drv->credential, &access_token);
    if (rc) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (!strcmp(src_path, "/"))
        rc = snprintf(url, sizeof(url), "%s/root/copy", drv->drv_url);
    else {
        char *src_path_esc;

        src_path_esc = http_client_escape(httpc, src_path, strlen(src_path));
        http_client_reset(httpc);
        if (!src_path_esc) {
            hive_set_error(-1);
            goto error_exit;
        }
        rc = snprintf(url, sizeof(url), "%s/root:%s:/copy", drv->drv_url, src_path_esc);
        http_client_memory_free(src_path_esc);
    }

    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        goto error_exit;
    }

    req_body = cJSON_CreateObject();
    if (!req_body) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = snprintf(parent_dir, sizeof(parent_dir), "%s/root:%s",
        drv->drv_url + strlen(ONEDRV_ME), dirname((char *)dest_path));
    if (rc < 0 || rc >= sizeof(parent_dir)) {
        hive_set_error(-1);
        goto error_exit;
    }

    parent_ref = cJSON_AddObjectToObject(req_body, "parentReference");
    if (!parent_ref) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (!cJSON_AddStringToObject(parent_ref, "path", parent_dir)) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (!cJSON_AddStringToObject(req_body, "name", basename((char *)dest_path))) {
        hive_set_error(-1);
        goto error_exit;
    }

    req_body_str = cJSON_PrintUnformatted(req_body);
    cJSON_Delete(req_body);
    req_body = NULL;
    if (!req_body_str) {
        hive_set_error(-1);
        goto error_exit;
    }

    void *args[] = {prog_qry_url, &prog_qry_url_sz};
    http_client_set_url_escape(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_request_body_instant(httpc, (void *)req_body_str, strlen(req_body_str));
    http_client_set_response_header(httpc, &copy_resp_hdr_cb, args);
    http_client_set_header(httpc, "Authorization", access_token);
    free(access_token);
    access_token = NULL;

    rc = http_client_request(httpc);
    free(req_body_str);
    req_body_str = NULL;
    if (rc) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    httpc = NULL;
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (resp_code != 202) {
        if (resp_code == 401)
            oauth_client_set_expired(drv->credential);
        hive_set_error(-1);
        goto error_exit;
    }

    return wait_til_complete(prog_qry_url);

error_exit:
    if (req_body)
        cJSON_Delete(req_body);
    if (req_body_str)
        free(req_body_str);
    if (httpc)
        http_client_close(httpc);
    if (access_token)
        free(access_token);
    return -1;
}

static int onedrive_drive_delete_file(HiveDrive *base, const char *path)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    char url[ONEDRIVE_MAX_URL_LENGTH] = {0};
    http_client_t *httpc = NULL;
    char *escaped_path = NULL;
    long resp_code = 0;
    char *access_token = NULL;
    int rc;

    rc = oauth_client_get_access_token(drive->credential, &access_token);
    if (rc) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        goto error_exit;
    }

    escaped_path = http_client_escape(httpc, path, strlen(path));
    http_client_reset(httpc);

    if (!escaped_path) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = snprintf(url, sizeof(url), "%s/root:%s:", drive->drv_url, escaped_path);
    http_client_memory_free(escaped_path);
    escaped_path = NULL;

    if (rc < 0 || rc >= (int)sizeof(url)) {
        hive_set_error(-1);
        goto error_exit;
    }

    http_client_set_url_escape(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_DELETE);
    http_client_set_header(httpc, "Authorization", access_token);
    free(access_token);
    access_token = NULL;

    rc = http_client_request(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    httpc = NULL;
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (resp_code != 204) {
        if (resp_code == 401)
            oauth_client_set_expired(drive->credential);
        hive_set_error(-1);
        goto error_exit;
    }

    return 0;

error_exit:
    if (httpc)
        http_client_close(httpc);
    if (access_token)
        free(access_token);
    if (escaped_path)
        http_client_memory_free(escaped_path);
    return -1;
}

static void onedrive_drive_close(HiveDrive *base)
{
    assert(base);
    deref(base);
}

static void onedrive_drive_destroy(void *obj)
{
    OneDriveDrive *drive = (OneDriveDrive *)obj;

    deref(drive->credential);
}

HiveDrive *onedrive_drive_open(oauth_client_t *credential, const char *drive_id)
{
    OneDriveDrive *drive;
    char path[512] = {0};
    size_t url_len;

    assert(credential);
    assert(drive_id);

    /*
     * If param @driveid equals "default", then use the default drive.
     * otherwise, use the drive with specific driveid.
     */

    if (!strcmp(drive_id, "default"))
        snprintf(path, sizeof(path), "/drive");
    else
        snprintf(path, sizeof(path), "/drives/%s", drive_id);

    url_len = strlen(ONEDRV_ME) + strlen(path) + 1;
    drive = (OneDriveDrive *)rc_zalloc(sizeof(OneDriveDrive) + url_len,
        &onedrive_drive_destroy);
    if (!drive) {
        hive_set_error(-1);
        return NULL;
    }

    snprintf(drive->drv_url, url_len, "%s%s", ONEDRV_ME, path);

    ref(credential);
    drive->credential = credential;

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
