#include <stdlib.h>
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
#include "oauth_client_intl.h"

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

static int onedrive_client_perform_tsx(HiveClient *obj, client_tsx_t *base)
{
    OneDriveClient *client = (OneDriveClient *)obj;
    onedrv_tsx_t *tsx = (onedrv_tsx_t *)base;
    int rc;
    long resp_code;
    char *access_token;
    http_client_t *http_client;

    http_client = http_client_new();
    if (!http_client)
        return -1;

    rc = oauth_client_get_access_token(client->oauth_client, &access_token);
    if (rc) {
        http_client_close(http_client);
        return -1;
    }

    while (true) {
        tsx->setup_req(http_client, tsx->user_data);
        http_client_set_header(http_client, "Authorization", access_token);
        free(access_token);

        rc = http_client_request(http_client);
        if (rc) {
            http_client_close(http_client);
            return -1;
        }

        rc = http_client_get_response_code(http_client, &resp_code);
        if (rc) {
            http_client_close(http_client);
            return -1;
        }

        if (resp_code == 401) {
            rc = oauth_client_refresh_access_token(client->oauth_client, &access_token);
            if (rc) {
                http_client_close(http_client);
                return -1;
            }
            http_client_reset(http_client);
            continue;
        }

        tsx->resp = http_client;
        return 0;
    }
}

static void get_info_setup_req(http_client_t *req, void *args)
{
    (void)args;

    http_client_set_url(req, ONEDRV_ME);
    http_client_set_method(req, HTTP_METHOD_GET);
    http_client_set_header(req, "Content-Type", "application/json");
    http_client_enable_response_body(req);
}

static int onedrive_client_get_info(HiveClient *obj, char **result)
{
    int rc;
    long resp_code;
    onedrv_tsx_t tsx = {
        .setup_req = &get_info_setup_req,
    };

    rc = onedrive_client_perform_tsx(obj, &tsx);
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

static void list_drives_setup_req(http_client_t *req, void *args)
{
    const char *url = (const char *)args;

    http_client_set_url_escape(req, url);
    http_client_set_method(req, HTTP_METHOD_GET);
    http_client_enable_response_body(req);
}

static int onedrive_client_list_drives(HiveClient *obj, char **result)
{
    char url[MAXPATHLEN + 1];
    int rc;
    long resp_code;

    rc = snprintf(url, sizeof(url), "%s/drives", ONEDRV_ME);
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    onedrv_tsx_t tsx = {
        .setup_req = &list_drives_setup_req,
        .user_data = url
    };

    rc = onedrive_client_perform_tsx(obj, &tsx);
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

static void validate_drive_setup_req(http_client_t *req, void *args)
{
    const char *url = (const char *)args;

    http_client_set_url_escape(req, url);
    http_client_set_method(req, HTTP_METHOD_GET);
}

static int validate_drive_id(OneDriveClient *client, const char *drive_id)
{
#define RESP_BODY_MAX_SZ 2048
    char url[MAXPATHLEN + 1];
    int rc;
    long resp_code;

    rc = snprintf(url, sizeof(url), "%s/drives/%s", ONEDRV_ME, drive_id);
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    onedrv_tsx_t tsx = {
        .setup_req = &validate_drive_setup_req,
        .user_data = url
    };

    rc = onedrive_client_perform_tsx((HiveClient *)client, &tsx);
    if (rc)
        return -1;

    rc = http_client_get_response_code(tsx.resp, &resp_code);
    http_client_close(tsx.resp);
    if (rc < 0 || resp_code != 200)
        return -1;
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

    if (strcmp(opt->drive_id, "default")) {
        rc = validate_drive_id(client, opt->drive_id);
        if (rc)
            return NULL;
    }

    return onedrive_drive_open(obj, opt->drive_id);
}

static int onedrive_client_close(HiveClient *obj)
{
    deref(obj);
    return 0;
}

static int onedrive_client_invalidate_credential(HiveClient *obj)
{
    OneDriveClient *client = (OneDriveClient *)obj;

    return oauth_client_set_expired(client->oauth_client);
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

    onedrv_client = (OneDriveClient *)rc_zalloc(sizeof(OneDriveClient),
        &onedrive_client_destructor);
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

    onedrv_client->base.login                 = &onedrive_client_login;
    onedrv_client->base.logout                = &onedrive_client_logout;
    onedrv_client->base.get_info              = &onedrive_client_get_info;
    onedrv_client->base.list_drives           = &onedrive_client_list_drives;
    onedrv_client->base.drive_open            = &onedrive_client_drive_open;
    onedrv_client->base.finalize              = &onedrive_client_close;

    onedrv_client->base.perform_tsx           = &onedrive_client_perform_tsx;
    onedrv_client->base.invalidate_credential = &onedrive_client_invalidate_credential;

    return (HiveClient *)onedrv_client;
}
