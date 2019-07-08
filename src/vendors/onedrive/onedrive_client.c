#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#endif

#include <crystal.h>
#include <cjson/cJSON.h>

#include "hive_error.h"
#include "onedrive_misc.h"
#include "onedrive_constants.h"
#include "http_client.h"

typedef struct OneDriveClient {
    HiveClient base;
    oauth_token_t *token;
    char keystore_path[PATH_MAX];
    char tmp_template[PATH_MAX];
} OneDriveClient;

static int onedrive_client_login(HiveClient *base,
                                 HiveRequestAuthenticationCallback *callback,
                                 void *user_data)
{
    OneDriveClient *client = (OneDriveClient *)base;
    int rc;

    if (!callback) {
        vlogE("OneDriveClient: no open URL callback configured.");
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);
    }

    assert(client);
    assert(client->token);

    rc = oauth_token_request(client->token,
                             (oauth_request_func_t *)callback, user_data);
    if (rc < 0) {
        vlogE("OneDriveClient: Try to login onto onedrive error (%d)", rc);
        return rc;
    }

    return 0;
}

static int onedrive_client_logout(HiveClient *base)
{
    OneDriveClient *client = (OneDriveClient *)base;

    assert(client);
    assert(client->token);

    oauth_token_reset(client->token);
    return 0;
}

#define DECODE_INFO_FIELD(json, name, field) do { \
        int rc; \
        rc = decode_info_field(json, name, field, sizeof(field)); \
        if (rc < 0) { \
            vlogE("OneDriveClient: missing %s json object.", name); \
            cJSON_Delete(json); \
            return rc; \
        } \
    } while(0)

static
int onedrive_decode_client_info(const char *info_str, HiveClientInfo *info)
{
    cJSON *json;

    assert(info_str);
    assert(info);

    json = cJSON_Parse(info_str);
    if (!json) {
        vlogE("OneDriveClient: bad json format.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    DECODE_INFO_FIELD(json, "id", info->user_id);
    DECODE_INFO_FIELD(json, "displayName", info->display_name);
    DECODE_INFO_FIELD(json, "mail", info->email);
    DECODE_INFO_FIELD(json, "mobilePhone", info->phone_number);
    DECODE_INFO_FIELD(json, "officeLocation", info->region);

    cJSON_Delete(json);
    return 0;
}

static int onedrive_client_get_info(HiveClient *base, HiveClientInfo *info)
{
    OneDriveClient *client = (OneDriveClient *)base;
    http_client_t *httpc;
    long resp_code = 0;
    char *p = NULL;
    int rc;

    assert(client);
    assert(client->token);
    assert(info);

    rc = oauth_token_check_expire(client->token);
    if (rc < 0) {
        vlogE("OneDriveClient: checking access token expired error.");
        return rc;
    }

    httpc = http_client_new();
    if (!httpc) {
        vlogE("OneDriveClient: failed to create http client instance.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    http_client_set_url(httpc, URL_API);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_set_header(httpc, "Authorization", get_bearer_token(client->token));
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc) {
        rc = HIVE_HTTPC_ERROR(rc);
        vlogE("OneDriveClient: failed to perform http request.");
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc) {
        rc = HIVE_HTTPC_ERROR(rc);
        vlogE("OneDriveClient: failed to get http response code.");
        goto error_exit;
    }

    if (resp_code == HttpStatus_Unauthorized) {
        vlogE("OneDriveClient: current node is not reachable.");
        oauth_token_set_expired(client->token);
        rc = HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
        goto error_exit;
    }

    if (resp_code != HttpStatus_OK) {
        vlogE("OneDriveClient: error from http response (%d).", resp_code);
        rc = HIVE_HTTP_STATUS_ERROR(resp_code);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p) {
        vlogE("OneDriveClient: failed to get response body.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    rc = onedrive_decode_client_info(p, info);
    free(p);

    return rc;

error_exit:
    http_client_close(httpc);
    return rc;
}

static int onedrive_client_drive_open(HiveClient *base, HiveDrive **drive)
{
    OneDriveClient *client = (OneDriveClient *)base;
    int rc;

    assert(client);
    assert(client->token);
    assert(drive);

    rc = onedrive_drive_open(client->token, "default", client->tmp_template, drive);
    if (rc < 0) {
        vlogE("OneDriveClient: Opening onedrive drive handle error");
        return rc;
    }

    return 0;
}

static int onedrive_client_close(HiveClient *base)
{
    assert(base);

    deref(base);
    return 0;
}

static cJSON *load_keystore_in_json(const char *path)
{
    struct stat st;
    char buf[4096] = {0};
    cJSON *json;
    int rc;
    int fd;

    assert(path);

    rc = stat(path, &st);
    if (rc < 0) {
        vlogE("OneDriveClient: cache does not exist.");
        return NULL;
    }

    if (!st.st_size || st.st_size > sizeof(buf)) {
        vlogE("OneDriveClient: cache size does not meet requirement.");
        errno = ERANGE;
        return NULL;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        vlogE("OneDriveClient: failed to open cache.");
        return NULL;
    }

    rc = (int)read(fd, buf, st.st_size);
    close(fd);

    if (rc < 0 || rc != st.st_size) {
        vlogE("OneDriveClient: read size error.");
        return NULL;
    }

    json = cJSON_Parse(buf);
    if (!json) {
        vlogE("OneDriveClient: bad json format for cache.");
        errno = ENOMEM;
        return NULL;
    }

    return json;
}

static int oauth_writeback(const cJSON *json, void *user_data)
{
    OneDriveClient *client = (OneDriveClient *)user_data;
    char *json_str;
    int json_str_len;
    int fd;
    int bytes;

    json_str = cJSON_PrintUnformatted(json);
    if (!json_str || !*json_str) {
        vlogE("OneDriveClient: bad json format.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    fd = open(client->keystore_path, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        vlogE("OneDriveClient: open cache failure (%d).", errno);
        free(json_str);
        return HIVE_SYS_ERROR(errno);
    }

    json_str_len = strlen(json_str);
    bytes = (int)write(fd, json_str, json_str_len + 1);
    free(json_str);
    close(fd);

    if (bytes != json_str_len + 1) {
        vlogE("OneDriveClient: write size error.");
        return HIVE_SYS_ERROR(errno);
    }

    return 0;
}

static void onedrive_client_destructor(void *obj)
{
    OneDriveClient *client = (OneDriveClient *)obj;

    if (client->token)
        oauth_token_delete(client->token);
}

HiveClient *onedrive_client_new(const HiveOptions *options)
{
    OneDriveOptions *opts = (OneDriveOptions *)options;
    oauth_options_t oauth_opts;
    OneDriveClient *client;
    cJSON *keystore = NULL;
    char path_tmp[PATH_MAX];
    int rc;

    assert(opts);

    if (!opts->client_id    || !*opts->client_id    ||
        !opts->redirect_url || !*opts->redirect_url ||
        !opts->scope        || !*opts->scope        ||
        opts->base.drive_type != HiveDriveType_OneDrive) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return NULL;
    }

    client = (OneDriveClient *)rc_zalloc(sizeof(OneDriveClient), onedrive_client_destructor);
    if (!client) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        return NULL;
    }

    rc = snprintf(client->keystore_path, sizeof(client->keystore_path),
                  "%s/.data/onedrive.json", options->persistent_location);
    if (rc < 0 || rc >= (int)sizeof(client->keystore_path)) {
        vlogE("OneDriveClient: keystore path too long.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        deref(client);
        return NULL;
    }
    strcpy(path_tmp, client->keystore_path);
    mkdir(dirname(path_tmp), S_IRUSR | S_IWUSR | S_IXUSR);

    rc = snprintf(client->tmp_template, sizeof(client->tmp_template),
                  "%s/.tmp/onedrive/XXXXXX", options->persistent_location);
    if (rc < 0 || rc >= (int)sizeof(client->tmp_template)) {
        vlogE("OneDriveClient: template too long.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        deref(client);
        return NULL;
    }
    strcpy(path_tmp, client->tmp_template);
    mkdir(dirname(dirname(path_tmp)), S_IRUSR | S_IWUSR | S_IXUSR);
    strcpy(path_tmp, client->tmp_template);
    mkdir(dirname(path_tmp), S_IRUSR | S_IWUSR | S_IXUSR);

    if (!access(client->keystore_path, F_OK)) {
        keystore = load_keystore_in_json(client->keystore_path);
        if (!keystore) {
            vlogE("OneDriveClient: load cache failure.");
            hive_set_error(HIVE_SYS_ERROR(errno));
            deref(client);
            return NULL;
        }
    }

    oauth_opts.store         = keystore;
    oauth_opts.authorize_url = URL_OAUTH METHOD_AUTHORIZE;
    oauth_opts.token_url     = URL_OAUTH METHOD_TOKEN;
    oauth_opts.client_id     = opts->client_id;
    oauth_opts.scope         = opts->scope;
    oauth_opts.redirect_url  = opts->redirect_url;

    client->token = oauth_token_new(&oauth_opts, &oauth_writeback, client);
    if (keystore)
        cJSON_Delete(keystore);

    if (!client->token) {
        vlogE("OneDriveClient: Failed to create onedrive client instance: failed to create oauth token instance.");
        deref(client);
        return NULL;
    }

    client->base.login       = onedrive_client_login;
    client->base.logout      = onedrive_client_logout;
    client->base.get_info    = onedrive_client_get_info;
    client->base.get_drive   = onedrive_client_drive_open;
    client->base.close       = onedrive_client_close;

    return &client->base;
}
