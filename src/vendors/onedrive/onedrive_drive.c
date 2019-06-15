#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
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
        // TODO: rc;
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
        rc = -1; // TODO;
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
    char url[MAX_URL_LENGTH + 1] = {0};
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

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    if (!strcmp(path, "/"))
        rc = snprintf(url, sizeof(url), "%s/root", URL_API);
    else
        rc = snprintf(url, sizeof(url), "%s/root:%s", URL_API, path);

    if (rc < 0 || rc >= sizeof(url)) {
        vlogE("Hive: Generating url to stat file with path (%s) error", path);
        rc = HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);
        goto error_exit;
    }

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
        // TODO: rc;
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
        rc = -1; // TODO;
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
int parse_response_and_notify_user(const char *response,
                                   HiveFilesIterateCallback *callback,
                                   void *context, char **next_url)
{
    cJSON *json;
    cJSON *array;
    cJSON *next_link;
    cJSON *item;

    assert(response);
    assert(callback);
    assert(next_url);

    json = cJSON_Parse(response);
    if (!json)
        return -1;

    array = cJSON_GetObjectItemCaseSensitive(json,  "value");
    if (!array || !cJSON_IsArray(array) || !cJSON_GetArraySize(array)) {
        cJSON_Delete(json);
        return -1;
    }

    cJSON_ArrayForEach(item, array) {
        cJSON *name;
        char *type;
        KeyValue properties[2];
        bool resume;

        if (!cJSON_IsObject(item)) {
            cJSON_Delete(json);
            return -1;
        }

        name = cJSON_GetObjectItemCaseSensitive(item, "name");
        if (!name || !cJSON_IsString(name) || !name->valuestring ||
            !*name->valuestring) {
            cJSON_Delete(json);
            return -1;
        }

        if (cJSON_GetObjectItemCaseSensitive(item, "file"))
            type = "file";
        else if (cJSON_GetObjectItemCaseSensitive(item, "folder"))
            type = "direcotry";
        else {
            cJSON_Delete(json);
            return -1;
        }

        properties[0].key   = "name";
        properties[0].value = name->valuestring;

        properties[1].key   = "type";
        properties[1].value = type;

        resume = callback(properties, sizeof(properties) / sizeof(properties[0]),
                          context);
        if (!resume) {
            cJSON_Delete(json);
            return 0;
        }
    }

    next_link = cJSON_GetObjectItemCaseSensitive(json, "@odata.nextLink");
    if (next_link && (!cJSON_IsString(next_link) || !next_link->valuestring ||
                      !*next_link->valuestring)) {
        cJSON_Delete(json);
        return -1;
    }

    if (next_link) {
        *next_url = next_link->valuestring;
        next_link->valuestring = NULL;
    } else {
        *next_url = NULL;
        callback(NULL, 0, context);
    }

    return 0;
}

static
int onedrive_drive_list_files(HiveDrive *base, const char *path,
                              HiveFilesIterateCallback *callback, void *context)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    http_client_t *httpc;
    char url[MAX_URL_LENGTH + 1] = {0};
    char *escaped_url = NULL;
    long resp_code;
    bool first_iteration = true;
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

    httpc = http_client_new();
    if (!httpc)
        return rc;

    if (!strcmp(path, "/"))
        rc = snprintf(url, sizeof(url), "%s/root/children", MY_DRIVE);
    else
        rc = snprintf(url, sizeof(url), "%s/root:%s:/children", MY_DRIVE, path);

    if (rc < 0 || rc >= sizeof(url)) {
        vlogE("Hive: Generating url to list files with path (%s) error", path);
        rc = HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);
        goto error_exit;
    }

    escaped_url = http_client_escape(httpc, url, strlen(url));
    http_client_reset(httpc);

    if (!escaped_url) {
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    while (escaped_url) {
        char *p;

        http_client_reset(httpc);
        http_client_set_url_escape(httpc, escaped_url);
        http_client_set_method(httpc, HTTP_METHOD_GET);
        http_client_set_header(httpc, "Authorization", get_bearer_token(drive->token));
        http_client_enable_response_body(httpc);

        rc = http_client_request(httpc);
        first_iteration ? http_client_memory_free(escaped_url) : free(escaped_url);

        if (rc < 0)
            break;

        rc = http_client_get_response_code(httpc, &resp_code);
        if (rc < 0)
            break;

        if (resp_code == 401) {
            oauth_token_set_expired(drive->token);
            rc = -1; // TODO;
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

        // TODO: need invoke callback once all files has been acquired.
        rc = parse_response_and_notify_user(p, callback, context, &escaped_url);
        if (rc < 0)
            break;

        if (!escaped_url) {
            rc = -1;
            break;
        }

        first_iteration = false;
    }

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
    char url[MAX_URL_LENGTH] = {0};
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

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    dir = dirname((char *)path);
    if (!strcmp(dir, "/"))
        rc = snprintf(url, sizeof(url), "%s/root/children", URL_API);
    else
        rc = snprintf(url, sizeof(url), "%s/root:%s:/chidlren", URL_API, dir);

    if (rc < 0 || rc >= (int)sizeof(url)) {
        vlogE("Hive: Generating url to list files with path (%s) error", path);
        rc = HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);
        goto error_exit;
    }

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
        // TODO: rc;
        return rc;
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

static char *create_cp_mv_request_body(const char *path)
{
    char parent[MAX_URL_LENGTH + 1] = {0};
    cJSON *body;
    cJSON *item;
    char *body_str;
    int rc;

    body = cJSON_CreateObject();
    if (!body)
        return NULL;

    rc = snprintf(parent, sizeof(parent), "%s/root:%s", URL_API,
                  dirname((char *)path));
    if (rc < 0 || rc >= (int)sizeof(parent))
        goto error_exit;

    item = cJSON_AddStringToObject(body, "parentReference", parent); //TODO
    if (!item)
        goto error_exit;

    if (!cJSON_AddStringToObject(item, "path", parent) ||
        !cJSON_AddStringToObject(body, "name", basename((char *)path)))
        goto error_exit;

    body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    return body_str;

error_exit:
    cJSON_Delete(body);
    return NULL;
}

static int onedrive_drive_move_file(HiveDrive *base, const char *old, const char *new)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    http_client_t *httpc;
    char url[MAX_URL_LENGTH + 1] = {0};
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

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    if (!strcmp(old, "/"))
        rc = snprintf(url, sizeof(url), "%s/root", URL_API);
    else
        rc = snprintf(url, sizeof(url), "%s/root:%s", URL_API, old);

    if (rc < 0 || rc >= (int)sizeof(url)) {
        vlogE("Hive: Generating url to move file from %s to %s error", old, new);
        rc = HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);
        goto error_exit;
    }

    escaped_url = http_client_escape(httpc, url, strlen(url));
    http_client_reset(httpc);

    if (!escaped_url) {
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    body = create_cp_mv_request_body(new);
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
        // TODO: rc;
        return rc;
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

/*
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
*/

static
int onedrive_drive_copy_file(HiveDrive *base, const char *src, const char *dest)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    http_client_t *httpc;
    char url[MAX_URL_LENGTH + 1];
    char *escaped_url;
    char *body;
    size_t url_len = sizeof(url);
    void *args[] = {url, &url_len};
    int rc;

    cJSON *req_body = NULL, *parent_ref;
    char *req_body_str = NULL;
    long resp_code;
    char parent_dir[MAXPATHLEN + 1];
    char prog_qry_url[MAXPATHLEN + 1];
    size_t prog_qry_url_sz = sizeof(prog_qry_url);

    rc = oauth_token_check_expire(drive->token);
    if (rc < 0) {
        vlogE("Hive: Checking access token expired error.");
        return rc;
    }

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    if (!strcmp(src, "/"))
        rc = snprintf(url, sizeof(url), "%s/root/copy", URL_API);
    else
        rc = snprintf(url, sizeof(url), "%s/root:%s:/copy", URL_API, src);

    if (rc < 0 || rc >= (int)sizeof(url)) {
        vlogE("Hive: Generating url to copy file from %s to %s error",
              src, dest);
        rc = HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);
        goto error_exit;
    }

    escaped_url = http_client_escape(httpc, url, strlen(url));
    http_client_reset(httpc);
    memset(url, 0, sizeof(url));

    if (!escaped_url) {
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    body = create_cp_mv_request_body(src);
    if (!body) {
        http_client_memory_free(escaped_url);
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    http_client_set_url(httpc, escaped_url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_header(httpc, "Authorization", get_bearer_token(drive->token));
    http_client_set_response_header(httpc, &handle_response_header, args);
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
        // TODO;
        return rc;
    }

    if (resp_code != 202) {
        // TODO;
        return rc;
    }

    // TODO: wait for completation.
    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static int onedrive_drive_delete_file(HiveDrive *base, const char *path)
{
    OneDriveDrive *drive = (OneDriveDrive *)base;
    http_client_t *httpc;
    char url[MAX_URL_LENGTH] = {0};
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

    rc = snprintf(url, sizeof(url), "%s/root:%s:", URL_API, path);
    if (rc < 0 || rc >= sizeof(url)) {
        vlogE("Hive: Generating url to delete file with path (%s) error", path);
        rc = HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);
        goto error_exit;
    }

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
        // TODO: rc;
        return rc;
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
    assert(base);
}

int onedrive_drive_open(oauth_token_t *token, const char *driveid,
                        HiveDrive **drive)
{
    OneDriveDrive *tmp;
    char path[512] = {0};
    size_t url_len;
    int rc;

    assert(token);
    assert(driveid);
    assert(drive);

    /*
     * If param @driveid equals "default", then use the default drive.
     * otherwise, use the drive with specific driveid.
     */

    if (!strcmp(driveid, "default"))
        snprintf(path, sizeof(path), "/drive");
    else
        snprintf(path, sizeof(path), "/drives/%s", driveid);

    url_len = strlen(URL_API) + strlen(path) + 1;
    tmp = (OneDriveDrive *)rc_zalloc(sizeof(OneDriveDrive) + url_len, NULL);
    if (!tmp)
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    tmp->token = token;

    tmp->base.get_info    = onedrive_drive_get_info;
    tmp->base.stat_file   = onedrive_drive_stat_file;
    tmp->base.list_files  = onedrive_drive_list_files;
    tmp->base.make_dir    = onedrive_drive_mkdir;
    tmp->base.move_file   = onedrive_drive_move_file;
    tmp->base.copy_file   = onedrive_drive_copy_file;
    tmp->base.delete_file = onedrive_drive_delete_file;
    tmp->base.close       = onedrive_drive_close;

    *drive = &tmp->base;
    // TODO: need refrence.

    return 0;
}
