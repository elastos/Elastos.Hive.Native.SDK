#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if defined(_WIN32) || defined(_WIN64)
#include <winnt.h>
#endif
#include <crystal.h>
#include <cjson/cJSON.h>

#include "onedrive_client.h"
#include "onedrive_drive.h"
#include "onedrive_constants.h"
#include "http_client.h"
#include "oauth_token.h"
#include "client.h"

typedef struct OneDriveClient {
    HiveClient base;
    HiveDrive *drive;
    oauth_token_t *token;
    char oauth_cookie[MAXPATHLEN + 1];
} OneDriveClient;

static inline void *_test_and_swap_ptr(void **ptr, void *oldval, void *newval)
{
#if defined(_WIN32) || defined(_WIN64)
    return InterlockedCompareExchangePointer(ptr, newval, oldval);
#else
    void *tmp = oldval;
    __atomic_compare_exchange(ptr, &tmp, &newval, false,
                              __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return tmp;
#endif
}

static inline void *_exchange_ptr(void **ptr, void *val)
{
#if defined(_WIN32) || defined(_WIN64)
    return _InterlockedExchangePointer(ptr, val);
#else
    void *old;
    __atomic_exchange(ptr, &val, &old, __ATOMIC_SEQ_CST);
    return old;
#endif
}

static int onedrive_client_login(HiveClient *base,
                                 HiveRequestAuthenticationCallback *callback,
                                 void *user_data)
{
    OneDriveClient *client = (OneDriveClient *)base;
    oauth_token_t *token;
    int rc = -1;

    assert(client);
    assert(client->token);
    assert(callback);

    rc = oauth_token_request(client->token,
                             (oauth_request_func_t *)callback, user_data);
    if (rc < 0) {
        vlogE("Hive: Try to login onto onedrive error (%d)", rc);
        return rc;
    }

    return 0;
}

static int onedrive_client_logout(HiveClient *base)
{
    OneDriveClient *client = (OneDriveClient *)base;
    HiveDrive *drive;

    assert(client);
    assert(client->token);

    drive = _exchange_ptr((void **)&client->drive, NULL);
    if (drive)
        deref(drive);

    oauth_token_reset(client->token);
    return 0;
}

#define DECODE_INFO_FIELD(json, name, field) do { \
        cJSON *item = cJSON_GetObjectItemCaseSensitive(json, name); \
        if (!cJSON_IsString(item) || \
            !item->valuestring || !*item->valuestring) { \
            cJSON_Delete(json); \
            return -1; \
        } \
        if (strlen(item->valuestring) >= sizeof(field)) { \
            cJSON_Delete(json); \
            return -2; \
        } \
        strcpy(field, item->valuestring); \
    } while(0)

static
int onedrive_decode_client_info(const char *info_str, HiveClientInfo *info)
{
    cJSON *json;

    assert(info_str);
    assert(info);

    json = cJSON_Parse(info_str);
    if (!json)
        return -1;

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
        vlogE("Hive: Checking access token expired error.");
        return rc;
    }

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    http_client_set_url_escape(httpc, URL_API);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_header(httpc, "Authorization", get_bearer_token(client->token));
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc < 0)
        goto error_exit;

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0)
        goto error_exit;

    if (resp_code == 401) {
        oauth_token_set_expired(client->token);
        goto error_exit;
    }

    if (resp_code != 200) {
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    rc = onedrive_decode_client_info(p, info);
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

#if 0
static int onedrive_client_list_drives(HiveClient *base, char **result)
{
    OneDriveClient *client = (OneDriveClient *)base;
    http_client_t *httpc;
    char url[MAX_URL_LENGTH] = {0};
    long resp_code = 0;
    char *p;
    int rc;

    assert(client);
    assert(client->token);
    assert(result);

    rc = oauth_token_check_expire(client->token);
    if (rc < 0) {
        vlogE("Hive: Checking access token expired error.");
        return rc;
    }

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    snprintf(url, sizeof(url), "%s/drives", URL_API);

    http_client_set_url_escape(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_header(httpc, "Authorization", get_bearer_token(client->token));
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc < 0)
        goto error_exit;

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0)
        goto error_exit;

    if (resp_code == 401) {
        oauth_token_set_expired(client->token);
        goto error_exit;
    }

    if (resp_code != 200)
        goto error_exit;

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    *result = p;
    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}
#endif

static int onedrive_client_drive_open(HiveClient *base, HiveDrive **drive)
{
    OneDriveClient *client = (OneDriveClient *)base;
    int rc;

    assert(client);
    assert(client->token);
    assert(drive);

    if (client->drive)
        return HIVE_GENERAL_ERROR(HIVEERR_ALREADY_EXIST);

    rc = onedrive_drive_open(client->token, "default", drive);
    if (rc < 0)
        return -1;

    return 0;
}

static int onedrive_client_close(HiveClient *base)
{
    assert(base);

    deref(base);
    return 0;
}

static void client_destroy(void *p)
{
    OneDriveClient *client = (OneDriveClient *)p;

    if (client->drive)
        deref(client->drive);

    if (client->token)
        oauth_token_delete(client->token);
}

static cJSON *load_oauth_cache(const char *oauth_cookie)
{
    struct stat st;
    size_t n2read;
    ssize_t nread;
    cJSON *json;
    char *buf;
    char *cur;
    int rc;
    int fd;

    assert(oauth_cookie);

    rc = stat(oauth_cookie, &st);
    if (rc < 0)
        return NULL;

    if (!st.st_size)
        return NULL;

    fd = open(oauth_cookie, O_RDONLY);
    if (fd < 0)
        return NULL;

    buf = malloc(st.st_size);
    if (!buf) {
        close(fd);
        return NULL;
    }

    for (n2read = st.st_size, cur = buf; n2read; n2read -= nread, cur += nread) {
        nread = read(fd, cur, n2read);
        if (!nread || (nread < 0 && errno != EINTR)) {
            close(fd);
            free(buf);
            return NULL;
        }
        if (nread < 0)
            nread = 0;
    }
    close(fd);

    json = cJSON_Parse(buf);
    free(buf);

    return json;
}

static int oauth_writeback(const cJSON *json, void *user_data)
{
    OneDriveClient *client = (OneDriveClient *)user_data;
    int fd;
    size_t n2write;
    ssize_t nwrite;
    char *json_str;
    char *tmp;

    json_str = cJSON_PrintUnformatted(json);
    if (!json_str || !*json_str)
        return -1;

    fd = open(client->oauth_cookie, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        free(json_str);
        return -1;
    }

    for (n2write = strlen(json_str), tmp = json_str;
         n2write; n2write -= nwrite, tmp += nwrite) {
        nwrite = write(fd, tmp, n2write);
        if (!nwrite || (nwrite < 0 && errno != EINTR)) {
            free(json_str);
            close(fd);
            return -1;
        }
        if (nwrite < 0)
            nwrite = 0;
    }
    free(json_str);
    close(fd);

    return 0;
}

HiveClient *onedrive_client_new(const HiveOptions *options)
{
    OneDriveOptions *opts = (OneDriveOptions *)options;
    char oauth_cookie[MAXPATHLEN + 1];
    oauth_options_t oauth_opts;
    OneDriveClient *client;
    oauth_token_t *token;
    cJSON *oauth_cookie_json = NULL;
    int rc;

    assert(opts);
    assert(opts->base.drive_type == HiveDriveType_OneDrive);

    if (!opts->client_id    || !*opts->client_id    ||
        !opts->redirect_url || !*opts->redirect_url ||
        !opts->scope        || !*opts->scope) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return NULL;
    }

    rc = snprintf(oauth_cookie, sizeof(oauth_cookie), "%s/onedrive.json",
                  options->persistent_location);
    if (rc < 0 || rc >= sizeof(oauth_cookie))
        return NULL;

    if (!access(oauth_cookie, F_OK)) {
        oauth_cookie_json = load_oauth_cache(oauth_cookie);
        if (!oauth_cookie_json)
            return NULL;
    }

    client = (OneDriveClient *)rc_zalloc(sizeof(OneDriveClient), client_destroy);
    if (!client) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        return NULL;
    }

    oauth_opts.store         = oauth_cookie_json;
    oauth_opts.authorize_url = URL_OAUTH METHOD_AUTHORIZE;
    oauth_opts.token_url     = URL_OAUTH METHOD_TOKEN;
    oauth_opts.client_id     = opts->client_id;
    oauth_opts.scope         = opts->scope;
    oauth_opts.redirect_url  = opts->redirect_url;

    client->token = oauth_token_new(&oauth_opts, &oauth_writeback, client);
    if (!client->token) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        deref(client);
        return NULL;
    }

    memcpy(client->oauth_cookie, oauth_cookie, strlen(oauth_cookie) + 1);
    client->base.login       = &onedrive_client_login;
    client->base.logout      = &onedrive_client_logout;
    client->base.get_info    = &onedrive_client_get_info;
    client->base.get_drive   = &onedrive_client_drive_open;
    client->base.close       = &onedrive_client_close;

    return &client->base;
}
