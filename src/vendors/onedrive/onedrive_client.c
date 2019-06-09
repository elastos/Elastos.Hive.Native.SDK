#include <stdlib.h>
#include <crystal.h>
#include <errno.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
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
} OneDriveClient;

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

    assert(client);
    assert(client->token);

    oauth_token_reset(client->token);
    return 0;
}

static int onedrive_client_decode_client_info(const char *info_str,
                                              HiveClientInfo *result)
{
    cJSON *display_name;
    cJSON *phone_number;
    cJSON *region;
    cJSON *json;
    cJSON *mail;
    cJSON *id;
    int rc;

    assert(info_str);
    assert(result);

    json = cJSON_Parse(info_str);
    if (!json)
        return -1;

    id = cJSON_GetObjectItemCaseSensitive(json, "id");
    if (!cJSON_IsString(id) || !id->valuestring || !*id->valuestring) {
        cJSON_Delete(json);
        return -1;
    }

    display_name = cJSON_GetObjectItemCaseSensitive(json, "displayName");
    if (!cJSON_IsString(display_name) || !display_name->valuestring ||
        !*display_name->valuestring) {
        cJSON_Delete(json);
        return -1;
    }

    mail = cJSON_GetObjectItemCaseSensitive(json, "mail");
    if (!cJSON_IsString(mail) || !mail->valuestring) {
        cJSON_Delete(json);
        return -1;
    }

    phone_number = cJSON_GetObjectItemCaseSensitive(json, "mobilePhone");
    if (!cJSON_IsString(phone_number) || !phone_number->valuestring) {
        cJSON_Delete(json);
        return -1;
    }

    region = cJSON_GetObjectItemCaseSensitive(json, "officeLocation");
    if (!cJSON_IsString(region) || !region->valuestring) {
        cJSON_Delete(json);
        return -1;
    }

    rc = snprintf(result->user_id, sizeof(result->user_id), "%s", id->valuestring);
    if (rc < 0 || rc >= sizeof(result->user_id)) {
        cJSON_Delete(json);
        return -1;
    }

    rc = snprintf(result->display_name, sizeof(result->display_name),
                  "%s", display_name->valuestring);
    if (rc < 0 || rc >= sizeof(result->display_name)) {
        cJSON_Delete(json);
        return -1;
    }

    rc = snprintf(result->email, sizeof(result->email), "%s", mail->valuestring);
    if (rc < 0 || rc >= sizeof(result->email)) {
        cJSON_Delete(json);
        return -1;
    }

    rc = snprintf(result->phone_number, sizeof(result->phone_number),
                  "%s", phone_number->valuestring);
    if (rc < 0 || rc >= sizeof(result->phone_number)) {
        cJSON_Delete(json);
        return -1;
    }

    rc = snprintf(result->phone_number, sizeof(result->phone_number),
                  "%s", phone_number->valuestring);
    if (rc < 0 || rc >= sizeof(result->phone_number)) {
        cJSON_Delete(json);
        return -1;
    }

    rc = snprintf(result->region, sizeof(result->region),
                  "%s", region->valuestring);
    if (rc < 0 || rc >= sizeof(result->region)) {
        cJSON_Delete(json);
        return -1;
    }

    cJSON_Delete(json);
    return 0;
}

static int onedrive_client_get_info(HiveClient *base, HiveClientInfo *result)
{
    OneDriveClient *client = (OneDriveClient *)base;
    http_client_t *httpc;
    long resp_code = 0;
    char *p = NULL;
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

    rc = onedrive_client_decode_client_info(p, result);
    free(p);
    if (rc)
        return -1;

    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

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

static int onedrive_client_drive_open(HiveClient *base, HiveDrive **drive)
{
    OneDriveClient *client = (OneDriveClient *)base;

    assert(client);
    assert(client->token);
    assert(drive);

    if (!client->drive) {
        int rc;

        rc = onedrive_drive_open(client->token, "default", &client->drive);
        if (rc < 0)
            return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    *drive  = client->drive;
    ref(*drive);

    return 0;
}

static int onedrive_client_close(HiveClient *base)
{
    OneDriveClient *client = (OneDriveClient *)base;

    assert(client);
    assert(client->drive);

    if (client->drive) {
        client->drive->close(client->drive);
        deref(client->drive);
        client->drive = NULL;
    }

    if (client->token) {
        oauth_token_delete(client->token);
        deref(client->token);
        client->token = NULL;
    }

    deref(client);
    return 0;
}

static void client_destroy(void *p)
{
    OneDriveClient *client = (OneDriveClient *)p;

    // Double checking.
    if (client->drive) {
        client->drive->close(client->drive);
        deref(client->drive);
    }

    // Double checking.
    if (client->token) {
        oauth_token_delete(client->token);
        deref(client->token);
    }
}

HiveClient *onedrive_client_new(const HiveOptions *options)
{
    OneDriveOptions *opts = (OneDriveOptions *)options;
    OneDriveClient *client = NULL;
    oauth_options_t oauth_opts;

    assert(opts);
    assert(opts->base.drive_type == HiveDriveType_OneDrive);

    if (!opts->client_id    || !*opts->client_id    ||
        !opts->redirect_url || !*opts->redirect_url ||
        !opts->scope        || !*opts->scope) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return NULL;
    }

    client = (OneDriveClient *)rc_zalloc(sizeof(OneDriveClient), client_destroy);
    if (!client) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        return NULL;
    }

    // oauth_opts.vendor_name = "onedrive";
    oauth_opts.client_id = opts->client_id;
    oauth_opts.scope = opts->scope;
    oauth_opts.redirect_url = opts->redirect_url;

    //TOOD:

    client->token = oauth_token_new(&oauth_opts, NULL, NULL);
    if (!client->token) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        deref(client);
        return NULL;
    }

    client->base.login        = &onedrive_client_login;
    client->base.logout       = &onedrive_client_logout;
    client->base.get_info     = &onedrive_client_get_info;
    client->base.list_drives  = &onedrive_client_list_drives;
    client->base.get_drive    = &onedrive_client_drive_open;
    client->base.finalize     = &onedrive_client_close;

    return &client->base;
}
