#include <stddef.h>
#include <assert.h>
#include <crystal.h>
#include <stdlib.h>
#include <crystal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "ipfs_drive.h"
#include "ipfs_token.h"
#include "ipfs_constants.h"
#include "ipfs_common_ops.h"
#include "http_client.h"
#include "error.h"

typedef struct ipfs_drive {
    HiveDrive base;
    ipfs_token_t *token;
} ipfs_drive_t;

static int ipfs_drive_get_info(HiveDrive *base, HiveDriveInfo *info)
{
    int rc;

    assert(base);
    assert(info);

    (void)base;

    // TODO:
    rc = snprintf(info->driveid, sizeof(info->driveid), "");
    if (rc < 0 || rc >= sizeof(info->driveid))
        return -1;

    return 0;
}

static int ipfs_drive_stat_file(HiveDrive *base, const char *file_path,
                                HiveFileInfo *info)
{
    char **result;
    ipfs_drive_t *drive = (ipfs_drive_t *)base;
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    cJSON *response;
    cJSON *hash;
    long resp_code;
    char *p;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/files/stat",
                  ipfs_token_get_node_in_use(drive->token), NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", ipfs_token_get_uid(drive->token));
    http_client_set_query(httpc, "path", file_path);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (resp_code != 200) {
        hive_set_error(-1);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p) {
        hive_set_error(-1);
        return -1;
    }

    response = cJSON_Parse(p);
    free(p);
    if (!response)
        return -1;

    hash = cJSON_GetObjectItemCaseSensitive(response, "Hash");
    if (!hash || !cJSON_IsString(hash) || !hash->valuestring ||
        !*hash->valuestring || strlen(hash->valuestring) >= sizeof(info->fileid)) {
        cJSON_Delete(response);
        return -1;
    }

    strcpy(info->fileid, hash->valuestring);
    cJSON_Delete(response);
    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static cJSON *parse_list_files_response(const char *response)
{
    cJSON *json;
    cJSON *entries;
    cJSON *entry;

    assert(response);

    json = cJSON_Parse(response);
    if (!json)
        return NULL;

    entries = cJSON_GetObjectItemCaseSensitive(json, "Entries");
    if (!entries || !cJSON_IsArray(entries)) {
        cJSON_Delete(json);
        return NULL;
    }

    cJSON_ArrayForEach(entry, entries) {
        cJSON *name;

        if (!cJSON_IsObject(entry)) {
            cJSON_Delete(json);
            return NULL;
        }

        name = cJSON_GetObjectItemCaseSensitive(entry, "name");
        if (!name || !cJSON_IsString(name) || !name->valuestring ||
            !*name->valuestring) {
            cJSON_Delete(json);
            return NULL;
        }
    }

    return json;
}

static void notify_file_entries(cJSON *entries,
                                HiveFilesIterateCallback *callback,
                                void *context)
{
    cJSON *entry;

    assert(entries);
    assert(callback);

    cJSON_ArrayForEach(entry, entries) {
        cJSON *name;
        KeyValue properties[1];
        bool resume;

        name = cJSON_GetObjectItemCaseSensitive(entry, "name");

        properties[0].key   = "name";
        properties[0].value = name->valuestring;

        resume = callback(properties, sizeof(properties) / sizeof(properties[0]),
                          context);
        if (!resume)
            return;
    }
    callback(NULL, 0, context);
}

static int ipfs_drive_list_files(HiveDrive *base, const char *path,
                                 HiveFilesIterateCallback *callback, void *context)
{
    ipfs_drive_t *drive = (ipfs_drive_t *)base;
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code;
    cJSON *response;
    char *p;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/files/ls",
                  ipfs_token_get_node_in_use(drive->token), NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", ipfs_token_get_uid(drive->token));
    http_client_set_query(httpc, "path", path);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (resp_code != 200) {
        hive_set_error(-1);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p) {
        hive_set_error(-1);
        return -1;
    }

    response = parse_list_files_response(p);
    free(p);
    if (!response)
        return -1;

    notify_file_entries(cJSON_GetObjectItemCaseSensitive(response, "Entries"),
                        callback, context);
    cJSON_Delete(response);
    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static int ipfs_drive_make_dir(HiveDrive *base, const char *path)
{
    ipfs_drive_t *drive = (ipfs_drive_t *)base;
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/files/mkdir",
                  ipfs_token_get_node_in_use(drive->token), NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    rc = ipfs_synchronize(drive->token);
    if (rc)
        goto error_exit;

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", ipfs_token_get_uid(drive->token));
    http_client_set_query(httpc, "path", path);
    http_client_set_query(httpc, "parents", "true");
    http_client_set_method(httpc, HTTP_METHOD_POST);

    rc = http_client_request(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    if (rc < 0)
        return -1;

    if (resp_code != 200)
        return -1;

    rc = ipfs_publish(drive->token, "/");
    if (rc < 0)
        return -1;

    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static int ipfs_drive_move_file(HiveDrive *base, const char *old, const char *new)
{
    ipfs_drive_t *drive = (ipfs_drive_t *)base;
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/files/mv",
                  ipfs_token_get_node_in_use(drive->token), NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", ipfs_token_get_uid(drive->token));
    http_client_set_query(httpc, "source", old);
    http_client_set_query(httpc, "dest", new);
    http_client_set_method(httpc, HTTP_METHOD_POST);

    rc = http_client_request(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    if (rc < 0)
        return -1;

    if (resp_code != 200)
        return -1;

    rc = ipfs_publish(drive->token, "/");
    if (rc < 0)
        return -1;

    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static int ipfs_drive_copy_file(HiveDrive *base, const char *src_path, const char *dest_path)
{
    ipfs_drive_t *drive = (ipfs_drive_t *)base;
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/files/cp",
                  ipfs_token_get_node_in_use(drive->token), NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", ipfs_token_get_uid(drive->token));
    http_client_set_query(httpc, "source", src_path);
    http_client_set_query(httpc, "dest", dest_path);
    http_client_set_method(httpc, HTTP_METHOD_POST);

    rc = http_client_request(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    if (rc < 0)
        return -1;

    if (resp_code != 200)
        return -1;

    rc = ipfs_publish(drive->token, "/");
    if (rc < 0)
        return -1;

    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static int ipfs_drive_delete_file(HiveDrive *base, const char *path)
{
    ipfs_drive_t *drive = (ipfs_drive_t *)base;
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/files/rm",
                  ipfs_token_get_node_in_use(drive->token), NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    rc = ipfs_synchronize(drive->token);
    if (rc)
        goto error_exit;

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", ipfs_token_get_uid(drive->token));
    http_client_set_query(httpc, "path", path);
    http_client_set_query(httpc, "recursive", "true");
    http_client_set_method(httpc, HTTP_METHOD_POST);

    rc = http_client_request(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    if (rc < 0)
        return -1;

    if (resp_code != 200)
        return -1;

    rc = ipfs_publish(drive->token, "/");
    if (rc < 0)
        return -1;

    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static void ipfs_drive_close(HiveDrive *obj)
{
    (void)obj;
}

HiveDrive *ipfs_drive_open(ipfs_token_t *token)
{
    ipfs_drive_t *drive;

    assert(token);

    drive = (ipfs_drive_t *)rc_zalloc(sizeof(ipfs_drive_t), NULL);
    if (!drive)
        return NULL;

    drive->token            = token;
    drive->base.get_info    = &ipfs_drive_get_info;
    drive->base.stat_file   = &ipfs_drive_stat_file;
    drive->base.list_files  = &ipfs_drive_list_files;
    drive->base.make_dir    = &ipfs_drive_make_dir;
    drive->base.move_file   = &ipfs_drive_move_file;
    drive->base.copy_file   = &ipfs_drive_copy_file;
    drive->base.delete_file = &ipfs_drive_delete_file;
    drive->base.close       = &ipfs_drive_close;

    return (HiveDrive *)drive;
}
