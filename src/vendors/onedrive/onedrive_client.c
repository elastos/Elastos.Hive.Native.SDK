#include <stdlib.h>
#include <oauth_client.h>
#include <crystal.h>
#include <errno.h>
#include <crystal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "onedrive_client.h"
#include "onedrive_common.h"
#include "onedrive_drive.h"
#include "http_client.h"
#include "oauth_client.h"

typedef struct OneDriveClient {
    HiveClient base;
    oauth_client_t *oauth_client;
} OneDriveClient;

static void onedrive_client_destructor(void *obj)
{
    OneDriveClient *client = (OneDriveClient *)obj;

    if (client->oauth_client)
        oauth_client_delete(client->oauth_client);
}

static int onedrive_client_login(HiveClient *obj)
{
    OneDriveClient *client = (OneDriveClient *)obj;

    return oauth_client_login(client->oauth_client);
}

static int onedrive_client_logout(HiveClient *obj)
{
    OneDriveClient *client = (OneDriveClient *)obj;

    return oauth_client_logout(client->oauth_client);
}

static int onedrive_client_list_drives(HiveClient *obj, char **result)
{
#define RESP_BODY_MAX_SZ 2048
    OneDriveClient *client = (OneDriveClient *)obj;
    char url[MAXPATHLEN + 1];
    int rc;
    char resp_body[RESP_BODY_MAX_SZ];
    size_t resp_body_len = sizeof(resp_body);
    long resp_code;
    http_client_t *http_cli;
    char *access_token;

    http_cli = http_client_new();
    if (!http_cli)
        return -1;

    rc = snprintf(url, sizeof(url), "%s/drives", ONEDRV_ROOT);
    if (rc < 0 || rc >= sizeof(url)) {
        http_client_close(http_cli);
        return -1;
    }

    rc = oauth_client_get_access_token(client->oauth_client, &access_token);
    if (rc) {
        http_client_close(http_cli);
        return -1;
    }

retry:
    http_client_reset(http_cli);

    http_client_set_url(http_cli, url);
    http_client_set_method(http_cli, HTTP_METHOD_GET);
    http_client_set_header(http_cli, "Authorization", access_token);
    http_client_set_response_body_instant(http_cli, resp_body, resp_body_len);
    free(access_token);

    rc = http_client_request(http_cli);
    if (rc) {
        http_client_close(http_cli);
        return -1;
    }

    rc = http_client_get_response_code(http_cli, &resp_code);
    if (rc < 0 || (resp_code != 200 && resp_code != 401)) {
        http_client_close(http_cli);
        return -1;
    }

    if (resp_code == 401) {
        rc = oauth_client_refresh_access_token(client->oauth_client, &access_token);
        if (rc) {
            http_client_close(http_cli);
            return -1;
        }
        goto retry;
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

static int validate_drive_id(OneDriveClient *client, const char *drive_id)
{
#define RESP_BODY_MAX_SZ 2048
    char url[MAXPATHLEN + 1];
    int rc;
    long resp_code;
    http_client_t *http_cli;
    char *access_token;

    http_cli = http_client_new();
    if (!http_cli)
        return -1;

    rc = snprintf(url, sizeof(url), "%s/drives/%s", ONEDRV_ROOT, drive_id);
    if (rc < 0 || rc >= sizeof(url)) {
        http_client_close(http_cli);
        return -1;
    }

    rc = oauth_client_get_access_token(client->oauth_client, &access_token);
    if (rc) {
        http_client_close(http_cli);
        return -1;
    }

retry:
    http_client_reset(http_cli);

    http_client_set_url(http_cli, url);
    http_client_set_method(http_cli, HTTP_METHOD_GET);
    http_client_set_header(http_cli, "Authorization", access_token);
    free(access_token);

    rc = http_client_request(http_cli);
    if (rc) {
        http_client_close(http_cli);
        return -1;
    }

    rc = http_client_get_response_code(http_cli, &resp_code);
    if (rc < 0 || (resp_code != 200 && resp_code != 401)) {
        http_client_close(http_cli);
        return -1;
    }

    if (resp_code == 401) {
        rc = oauth_client_refresh_access_token(client->oauth_client, &access_token);
        if (rc) {
            http_client_close(http_cli);
            return -1;
        }
        goto retry;
    }

    http_client_close(http_cli);

    return 0;
#undef RESP_BODY_MAX_SZ
}

static HiveDrive *onedrive_client_drive_open(
    HiveClient *obj, const HiveDriveOptions *options)
{
    OneDriveClient *client = (OneDriveClient *)obj;
    const OneDriveDriveOptions *opt = (const OneDriveDriveOptions *)options;
    int rc;

    if (!*opt->drive_id || strlen(opt->drive_id) >= sizeof(opt->drive_id))
        return NULL;

    rc = validate_drive_id(client, opt->drive_id);
    if (rc)
        return NULL;

    return onedrive_drive_open(obj, opt->drive_id);
}

static void onedrive_client_close(HiveClient *obj)
{
    deref(obj);
}

static int onedrive_client_get_access_token(HiveClient *obj, char **access_token)
{
    OneDriveClient *client = (OneDriveClient *)obj;

    return oauth_client_get_access_token(client->oauth_client, access_token);
}

static int onedrive_client_refresh_access_token(HiveClient *obj, char **access_token)
{
    OneDriveClient *client = (OneDriveClient *)obj;

    return oauth_client_refresh_access_token(client->oauth_client, access_token);
}

HiveClient *onedrive_client_new(const HiveOptions *options)
{
    OneDriveOptions *opts = (OneDriveOptions *)options;
    http_client_t *http_client;
    OneDriveClient *onedrv_client;
    oauth_opt_t oauth_opt;
    char *redirect_scheme;
    char *redirect_host;
    char *redirect_url;
    char *redirect_port;
    char *redirect_path;
    int rc;

    if (!opts->client_id    || !*opts->client_id    ||
        !opts->redirect_url || !*opts->redirect_url ||
        !opts->scope        || !*opts->scope        ||
        !opts->grant_authorize)
        return NULL;

    if (!realpath(opts->base.persistent_location, oauth_opt.profile_path) &&
        errno != ENOENT)
        return NULL;

    http_client = http_client_new();
    if (!http_client)
        return NULL;

    rc = http_client_set_url(http_client, opts->redirect_url);
    if (rc) {
        http_client_close(http_client);
        return NULL;
    }

    rc = http_client_get_scheme(http_client, &redirect_scheme);
    if (rc) {
        http_client_close(http_client);
        return NULL;
    }

    rc = http_client_get_host(http_client, &redirect_host);
    if (rc) {
        http_client_memory_free(redirect_scheme);
        http_client_close(http_client);
        return NULL;
    }

    redirect_url = malloc(strlen(redirect_scheme) + strlen(redirect_host) + 4);
    if (!redirect_url) {
        http_client_memory_free(redirect_scheme);
        http_client_memory_free(redirect_host);
        http_client_close(http_client);
        return NULL;
    }
    sprintf(redirect_url, "%s://%s", redirect_scheme, redirect_host);
    http_client_memory_free(redirect_scheme);
    http_client_memory_free(redirect_host);

    rc = http_client_get_port(http_client, &redirect_port);
    if (rc) {
        free(redirect_url);
        http_client_close(http_client);
        return NULL;
    }

    rc = http_client_get_path(http_client, &redirect_path);
    http_client_close(http_client);
    if (rc) {
        http_client_memory_free(redirect_port);
        free(redirect_url);
        return NULL;
    }

    onedrv_client = (OneDriveClient *)rc_zalloc(sizeof(OneDriveClient), &onedrive_client_destructor);
    if (!onedrv_client) {
        http_client_memory_free(redirect_port);
        http_client_memory_free(redirect_path);
        free(redirect_url);
        return NULL;
    }

    oauth_opt.auth_url        = "https://login.microsoftonline.com/common/oauth2/v2.0/authorize";
    oauth_opt.token_url       = "https://login.microsoftonline.com/common/oauth2/v2.0/token";
    oauth_opt.scope           = opts->scope;
    oauth_opt.client_id       = opts->client_id;
    oauth_opt.redirect_url    = redirect_url;
    oauth_opt.redirect_port   = redirect_port;
    oauth_opt.redirect_path   = redirect_path;
    oauth_opt.grant_authorize = opts->grant_authorize;

    onedrv_client->oauth_client = oauth_client_new(&oauth_opt);
    http_client_memory_free(redirect_port);
    http_client_memory_free(redirect_path);
    free(redirect_url);
    if (!onedrv_client->oauth_client) {
        deref(onedrv_client);
        return NULL;
    }

    onedrv_client->base.login                = &onedrive_client_login;
    onedrv_client->base.logout               = &onedrive_client_logout;
    onedrv_client->base.list_drives          = &onedrive_client_list_drives;
    onedrv_client->base.drive_open           = &onedrive_client_drive_open;
    onedrv_client->base.destructor_func      = &onedrive_client_close;

    onedrv_client->base.get_access_token     = &onedrive_client_get_access_token;
    onedrv_client->base.refresh_access_token = &onedrive_client_refresh_access_token;

    return (HiveClient *)onedrv_client;
}
