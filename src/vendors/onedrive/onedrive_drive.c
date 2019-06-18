#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include <crystal.h>
#include <cjson/cJSON.h>

#include "onedrive_misc.h"
#include "onedrive_constants.h"
#include "http_client.h"

#define ARGV(args, index) ((void **)args)[index]

typedef struct OneDriveDrive {
    HiveDrive base;
    oauth_token_t *token;
} OneDriveDrive ;

#define DECODE_INFO_FIELD(json, name, field) do { \
        int rc; \
        rc = decode_info_field(json, name, field, sizeof(field)); \
        if (rc < 0) \
            cJSON_Delete(json); \
        return rc; \
    } while(0)

static
int onedrive_decode_drive_info(const char *info_str, HiveDriveInfo *info)
{
    cJSON *json;

    assert(info_str);
    assert(info);

    json = cJSON_Parse(info_str);
    if (!json)
        return -1;

    DECODE_INFO_FIELD(json, "id", info->driveid);

    cJSON_Delete(json);
    return 0;
}

static
int onedrive_drive_get_info(HiveDrive *base, HiveDriveInfo *info)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    http_client_t *httpc;
    char *p = NULL;
    long resp_code = 0;
    int rc;

    assert(drive);
    assert(drive->token);
    assert(info);

    rc = oauth_token_check_expire(drive->token);
    if (rc < 0) {
        vlogE("Hive: Checking access token expired error.");
        return rc;
    }

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    http_client_set_url_escape(httpc, URL_API);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_header(httpc, "Authorization", get_bearer_token(drive->token));
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc) {
        // TODO: rc;
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0) {
        // TODO: rc;
        goto error_exit;
    }

    if (resp_code == 401) {
        oauth_token_set_expired(drive->token);
        rc = HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
        goto error_exit;
    }

    if (resp_code != 200) {
        // TODO: rc;
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    rc = onedrive_decode_drive_info(p, info);
    free(p);

    switch(rc) {
    case -1:
        rc = HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
        break;

    case -2:
        rc = HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);
        break;

    default:
        rc = 0;
        break;
    }

    return rc;

error_exit:
    http_client_close(httpc);
    return rc;
}

static
int onedrive_decode_file_info(const char *info_str, HiveFileInfo *info)
{
    // TODO;
    return -1;
}

static
int onedrive_drive_stat_file(HiveDrive *base, const char *path, HiveFileInfo *info)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    http_client_t *httpc;
    char url[MAX_URL_LEN] = {0};
    char *escaped_url;
    char *p;
    long resp_code = 0;
    int rc;

    assert(drive);
    assert(drive->token);
    assert(path);
    assert(*path == '/');
    assert(info);

    rc = oauth_token_check_expire(drive->token);
    if (rc < 0) {
        vlogE("Hive: Checking access token expired error.");
        return rc;
    }

    if (strlen(path) >= MAX_URL_PARAM_LEN)
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    if (!strcmp(path, "/"))
        sprintf(url, "%s/root", URL_API);
    else
        sprintf(url, "%s/root:%s", URL_API, path);

    escaped_url = http_client_escape(httpc, url, strlen(path));
    http_client_reset(httpc);

    if (!escaped_url)
        goto error_exit;

    http_client_set_url(httpc, escaped_url);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_header(httpc, "Authorization", get_bearer_token(drive->token));
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    http_client_memory_free(escaped_url);

    if (rc < 0) {
        // TODO: rc;
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0) {
        // TODO: rc;
        goto error_exit;
    }

    if (resp_code == 401) {
        oauth_token_set_expired(drive->token);
        rc = HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
        goto error_exit;
    }

    if (resp_code != 200) {
        // TODO: rc;
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p) {
        // TODO: rc;
        return rc;
    }

    rc = onedrive_decode_file_info(p, info);
    free(p);

    switch(rc) {
    case -1:
        rc = HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
        break;

    case -2:
        rc = HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);
        break;

    default:
        rc = 0;
        break;
    }

    return rc;

error_exit:
    http_client_close(httpc);
    return rc;
}

static
int merge_array(cJSON *sub, cJSON *array)
{
    cJSON *item;
    size_t sub_sz = cJSON_GetArraySize(sub);
    size_t i;

    for (i = 0; i < sub_sz; ++i) {
        cJSON *name;
        cJSON *file;
        cJSON *folder;

        item = cJSON_GetArrayItem(sub, i);

        name = cJSON_GetObjectItemCaseSensitive(item, "name");
        if (!name || !cJSON_IsString(name) || !name->valuestring || !*name->valuestring)
            return -1;

        file = cJSON_GetObjectItemCaseSensitive(item, "file");
        folder = cJSON_GetObjectItemCaseSensitive(item, "folder");
        if ((file && folder) || (!file && !folder))
            return -1;

        if (file && !cJSON_IsObject(file))
            return -1;

        if (folder && !cJSON_IsObject(folder))
            return -1;

        cJSON_AddItemToArray(array, cJSON_DetachItemFromArray(sub, i));
    }

    return 0;
}

static
void notify_user_files(cJSON *array, HiveFilesIterateCallback *callback,
                       void *context)
{
    cJSON *item;

    cJSON_ArrayForEach(item, array) {
        cJSON *name;
        char *type;
        KeyValue properties[2];
        bool resume;

        name = cJSON_GetObjectItemCaseSensitive(item, "name");
        assert(name);

        if (cJSON_GetObjectItemCaseSensitive(item, "file"))
            type = "file";
        else
            type = "directory";

        properties[0].key   = "name";
        properties[0].value = name->valuestring;

        properties[1].key   = "type";
        properties[1].value = type;

        resume = callback(properties, sizeof(properties) / sizeof(properties[0]),
                          context);
        if (!resume)
            return;
    }
    callback(NULL, 0, context);
}

static
int onedrive_drive_list_files(HiveDrive *base, const char *path,
                              HiveFilesIterateCallback *callback, void *context)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    http_client_t *httpc;
    char url[MAX_URL_LEN] = {0};
    char *next_url = NULL;
    long resp_code;
    cJSON *array;
    cJSON *json = NULL;
    int rc;

    assert(drive);
    assert(drive->token);
    assert(path);
    assert(*path == '/');
    assert(callback);

    rc = oauth_token_check_expire(drive->token);
    if (rc < 0) {
        vlogE("Hive: Checking access token expired error.");
        return rc;
    }

    if (strlen(path) >= MAX_URL_PARAM_LEN)
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    httpc = http_client_new();
    if (!httpc)
        return rc;

    if (!strcmp(path, "/"))
        sprintf(url, "%s/drive/root/children", MY_DRIVE);
    else
        sprintf(url, "%s/drive/root:%s:/children", MY_DRIVE, path);

    array = cJSON_CreateArray();
    if (!array) {
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    next_url = url;
    while (next_url) {
        char *p;
        cJSON *sub_array;
        cJSON *next_link;

        http_client_reset(httpc);
        http_client_set_url(httpc, next_url);
        http_client_set_method(httpc, HTTP_METHOD_GET);
        http_client_set_header(httpc, "Authorization", get_bearer_token(drive->token));
        http_client_enable_response_body(httpc);

        rc = http_client_request(httpc);
        if (json)
            cJSON_Delete(json);

        if (rc < 0)
            break;

        rc = http_client_get_response_code(httpc, &resp_code);
        if (rc < 0)
            break;

        if (resp_code == 401) {
            oauth_token_set_expired(drive->token);
            rc = HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
            break;
        }

        if (resp_code != 200) {
            rc = -1; // TODO;
            break;
        }

        p = http_client_move_response_body(httpc, NULL);
        if (!p) {
            rc = -1;
            break;
        }

        json = cJSON_Parse(p);
        http_client_memory_free(p);
        if (!json) {
            rc = -1;
            break;
        }

        sub_array = cJSON_GetObjectItemCaseSensitive(json, "value");
        if (!sub_array || !cJSON_IsArray(sub_array) || !cJSON_GetArraySize(sub_array)) {
            cJSON_Delete(json);
            rc = -1;
            break;
        }

        rc = merge_array(sub_array, array);
        if (rc < 0) {
            cJSON_Delete(json);
            break;
        }

        next_link = cJSON_GetObjectItemCaseSensitive(json, "@odata.nextLink");
        if (next_link && (!cJSON_IsString(next_link) || !next_link->valuestring ||
                          !*next_link->valuestring)) {
            cJSON_Delete(json);
            rc = -1;
            break;
        }

        if (next_link)
            next_url = next_link->valuestring;
        else {
            cJSON_Delete(json);
            notify_user_files(array, callback, context);
            rc = 0;
        }
    }

    cJSON_Delete(array);
    http_client_close(httpc);
    return rc;

error_exit:
    http_client_close(httpc);
    return rc;
}

static char *create_mkdir_request_body(const char *path)
{
    cJSON *body;
    char *body_str;
    char *p;

    assert(path);

    body = cJSON_CreateObject();
    if (!body)
        return NULL;

    p = basename((char *)path);
    if (!cJSON_AddStringToObject(body, "name", p) ||
        !cJSON_AddStringToObject(body, "folder", NULL) || // TODO
        !cJSON_AddStringToObject(body, "@microsoft.graph.conflictBehavior", "rename")){

        cJSON_Delete(body);
        return NULL;
    }

    body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    return body_str;
}

static int onedrive_drive_mkdir(HiveDrive *base, const char *path)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    http_client_t *httpc;
    char url[MAX_URL_LEN] = {0};
    char *escaped_url;
    char *body;
    char *dir;
    char *p;
    long resp_code = 0;
    int rc;

    assert(drive);
    assert(drive->token);
    assert(path);
    assert(*path);

    rc = oauth_token_check_expire(drive->token);
    if (rc < 0) {
        vlogE("Hive: Checking access token expired error.");
        return rc;
    }

    if (strlen(path) >= MAX_URL_PARAM_LEN)
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    dir = dirname((char *)path);
    if (!strcmp(dir, "/"))
        sprintf(url, "%s/root/children", URL_API);
    else
        sprintf(url, "%s/root:%s:/chidlren", URL_API, dir);

    escaped_url = http_client_escape(httpc, url, strlen(url));
    http_client_reset(httpc);

    if (!escaped_url) {
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    body = create_mkdir_request_body(path);
    if (!body) {
        http_client_memory_free(escaped_url);
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    http_client_set_url(httpc, escaped_url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_header(httpc, "Authorization", get_bearer_token(drive->token));
    http_client_set_request_body_instant(httpc, body, strlen(body));

    rc = http_client_request(httpc);
    http_client_memory_free(escaped_url);
    free(body);

    if (rc < 0) {
        // TODO: rc;
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);

    if (rc < 0) {
        // TODO: rc;
        return rc;
    }

    if (resp_code == 401) {
        oauth_token_set_expired(drive->token);
        return HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
    }

    if (resp_code != 201) {
        // TODO: rc;
        return rc;
    }

    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static char *create_cp_mv_request_body(const char *path, char *url)
{
    cJSON *body;
    cJSON *item;
    char *body_str;
    int rc;

    body = cJSON_CreateObject();
    if (!body)
        return NULL;

    sprintf(url, "%s/root:%s", URL_API, dirname((char *)path));
    item = cJSON_AddStringToObject(body, "parentReference", url); //TODO
    if (!item)
        goto error_exit;

    if (!cJSON_AddStringToObject(item, "path", url) ||
        !cJSON_AddStringToObject(body, "name", basename((char *)path)))
        goto error_exit;

    body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    return body_str;

error_exit:
    cJSON_Delete(body);
    return NULL;
}

static
int onedrive_drive_move_file(HiveDrive *base, const char *old, const char *new)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    http_client_t *httpc;
    char url[MAX_URL_LEN] = {0};
    char *escaped_url;
    char *body;
    long resp_code = 0;
    int rc;

    assert(drive);
    assert(drive->token);
    assert(old);
    assert(*old);
    assert(new);
    assert(*new);

    rc = oauth_token_check_expire(drive->token);
    if (rc < 0) {
        vlogE("Hive: Checking access token expired error.");
        return rc;
    }

    if (strlen(old) >= MAX_URL_PARAM_LEN ||
        strlen(new) >= MAX_URL_PARAM_LEN)
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    if (!strcmp(old, "/"))
        sprintf(url, "%s/root", URL_API);
    else
        sprintf(url, "%s/root:%s", URL_API, old);

    escaped_url = http_client_escape(httpc, url, strlen(url));
    http_client_reset(httpc);
    memset(url, 0, sizeof(url));

    if (!escaped_url) {
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    body = create_cp_mv_request_body(new, url);
    if (!body) {
        http_client_memory_free(escaped_url);
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    http_client_set_url(httpc, escaped_url);
    http_client_set_method(httpc, HTTP_METHOD_PATCH);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_header(httpc, "Authorization", get_bearer_token(drive->token));
    http_client_set_request_body_instant(httpc, body, strlen(body));

    rc = http_client_request(httpc);
    http_client_memory_free(escaped_url);
    free(body);

    if (rc < 0) {
        // TODO: rc;
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);

    if (rc < 0) {
        // TODO: rc;
        return rc;
    }

    if (resp_code == 401) {
        oauth_token_set_expired(drive->token);
        return HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
    }

    if (resp_code != 200) {
        // TODO;
        return rc;
    }

    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static
size_t handle_response_header(char *buffer, size_t size, size_t nitems,
                              void *user_data)
{
#define LOC_KEY "Location: "

    char *buf = (char *)ARGV(user_data, 0);
    size_t sz = *(size_t *)ARGV(user_data, 1);
    size_t total_sz = sz * nitems;
    size_t loc_len = strlen(LOC_KEY);

    assert(total_sz > strlen(LOC_KEY));
    assert(strlen(LOC_KEY) > sz);

    if (strncmp(buffer, LOC_KEY, loc_len))
        return  total_sz;

    memcpy(buf, buffer + strlen(LOC_KEY), loc_len);
    buf[loc_len + 1] = '\0';
    return total_sz;
}

static
int onedrive_drive_copy_file(HiveDrive *base, const char *src, const char *dest)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    http_client_t *httpc;
    char url[MAX_URL_LEN] = {0};
    char *escaped_url;
    char *body;
    long resp_code = 0;
    size_t url_len = sizeof(url);
    void *args[] = {url, &url_len};
    int rc;

    rc = oauth_token_check_expire(drive->token);
    if (rc < 0) {
        vlogE("Hive: Checking access token expired error.");
        return rc;
    }

    if (strlen(src) >= MAX_URL_PARAM_LEN ||
        strlen(dest) >= MAX_URL_PARAM_LEN)
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    if (!strcmp(src, "/"))
        sprintf(url, "%s/root/copy", URL_API);
    else
        sprintf(url, "%s/root:%s:/copy", URL_API, src);

    escaped_url = http_client_escape(httpc, url, strlen(url));
    http_client_reset(httpc);
    memset(url, 0, sizeof(url));

    if (!escaped_url) {
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    body = create_cp_mv_request_body(dest, url);
    if (!body) {
        http_client_memory_free(escaped_url);
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    http_client_set_url(httpc, escaped_url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_header(httpc, "Authorization", get_bearer_token(drive->token));
    http_client_set_response_header(httpc, &handle_response_header, args); //TODO:
    http_client_set_request_body_instant(httpc, body, strlen(body));

    rc = http_client_request(httpc);
    http_client_memory_free(escaped_url);
    free(body);

    if (rc < 0) {
        // TODO: rc;
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);

    if (rc < 0) {
        // TODO: rc;
        return rc;
    }

    if (resp_code == 400) {
        oauth_token_set_expired(drive->token);
        return HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
    }

    if (resp_code != 202) {
        // TODO;
        return rc;
    }

    // We will not wait for the completation of copy action.
    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static int onedrive_drive_delete_file(HiveDrive *base, const char *path)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    http_client_t *httpc;
    char url[MAX_URL_LEN] = {0};
    char *escaped_url;
    long resp_code = 0;
    int rc;

    assert(drive);
    assert(drive->token);
    assert(path);

    rc = oauth_token_check_expire(drive->token);
    if (rc < 0) {
        vlogE("Hive: Checking access token expired error.");
        return rc;
    }

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    if (strlen(path) >= MAX_URL_PARAM_LEN) {
        rc = HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);
        goto error_exit;
    }

    sprintf(url, "%s/root:%s", URL_API, path);
    escaped_url = http_client_escape(httpc, url, strlen(url));
    http_client_reset(httpc);

    if (!escaped_url) {
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    http_client_set_url(httpc, escaped_url);
    http_client_set_method(httpc, HTTP_METHOD_DELETE);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_header(httpc, "Authorization", get_bearer_token(drive->token));

    rc = http_client_request(httpc);
    http_client_memory_free(escaped_url);

    if (rc < 0) {
        // TODO: rc;
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);

    if (rc < 0) {
        // TODO: rc;
        return rc;
    }

    if (resp_code == 401) {
        oauth_token_set_expired(drive->token);
        return HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
    }

    if (resp_code != 200) {
        // TODO;
        return rc;
    }

    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static void onedrive_drive_close(HiveDrive *base)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    assert(drive);

    if (drive->token) {
        deref(drive->token);
        drive->token = NULL;
    }

    deref(drive);
}

int onedrive_drive_open(oauth_token_t *token, const char *driveid,
                        HiveDrive **drive)
{
    OneDriveDrive *tmp;
    char path[128] = {0};
    size_t url_len;
    int rc;

    assert(token);
    assert(driveid);
    assert(drive);

    /*
     * If param @driveid equals "default", then use the default drive.
     * otherwise, use the drive with specific driveid.
     */

    tmp = (OneDriveDrive *)rc_zalloc(sizeof(OneDriveDrive), NULL);
    if (!tmp)
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    // Add reference of token to drive.
    tmp->token = ref(token);

    tmp->base.get_info    = onedrive_drive_get_info;
    tmp->base.stat_file   = onedrive_drive_stat_file;
    tmp->base.list_files  = onedrive_drive_list_files;
    tmp->base.make_dir    = onedrive_drive_mkdir;
    tmp->base.move_file   = onedrive_drive_move_file;
    tmp->base.copy_file   = onedrive_drive_copy_file;
    tmp->base.delete_file = onedrive_drive_delete_file;
    tmp->base.close       = onedrive_drive_close;

    *drive = &tmp->base;

    return 0;
}
