#include <stdlib.h>
#include <crystal.h>
#include <errno.h>
#include <crystal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <crystal.h>
#include <oauth_client.h>

#include "onedrive_client.h"
#include "onedrive_common.h"
#include "onedrive_drive.h"
#include "http_client.h"

typedef struct OneDriveClient {
    HiveClient base;
    oauth_client_t *oauth_client;
} OneDriveClient;

static int onedrive_client_login(HiveClient *base)
{
    OneDriveClient *client = (OneDriveClient *)base;
    int rc;

    assert(client);
    assert(client->oauth_client);

    rc = oauth_client_login(client->oauth_client);
    if (rc < 0) {
        vlogE("Hive: Client Logining onto oneDrive error");
        hive_set_error(rc);
        return -1;
    }

    vlogD("Hive: Client logined onto oneDrive service");
    return 0;
}

static int onedrive_client_logout(HiveClient *base)
{
    OneDriveClient *client = (OneDriveClient *)base;

    assert(client);
    assert(client->oauth_client);

    (void)oauth_client_logout(client->oauth_client);

    vlogD("Hive: Logout from oneDrive service");
    return 0;
}

static int onedrive_client_get_info(HiveClient *base, char **result)
{
    OneDriveClient *client = (OneDriveClient *)base;
    http_client_t *httpc;
    long resp_code = 0;
    char *p = NULL;
    char *access_token;
    int rc;

    assert(client);
    assert(result);

    rc = oauth_client_get_access_token(client->oauth_client, &access_token);
    if (rc) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        free(access_token);
        hive_set_error(-1);
        return -1;
    }

    http_client_set_url(httpc, ONEDRV_ME);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_header(httpc, "Authorization", access_token);
    http_client_enable_response_body(httpc);
    free(access_token);

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
        if (resp_code == 401)
            oauth_client_set_expired(client->oauth_client);
        hive_set_error(-1);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p) {
        hive_set_error(-1);
        return -1;
    }

    *result = p;
    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static int onedrive_client_list_drives(HiveClient *base, char **result)
{
    OneDriveClient *client = (OneDriveClient *)base;
    char url[ONEDRIVE_MAX_URL_LENGTH] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    char *p;
    int rc;
    char *access_token;

    assert(client);
    assert(result);

    snprintf(url, sizeof(url), "%s/drives", ONEDRV_ME);

    rc = oauth_client_get_access_token(client->oauth_client, &access_token);
    if (rc) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        free(access_token);
        hive_set_error(-1);
        return -1;
    }

    http_client_set_url_escape(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_enable_response_body(httpc);
    http_client_set_header(httpc, "Authorization", access_token);
    http_client_enable_response_body(httpc);
    free(access_token);

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
        if (resp_code == 401)
            oauth_client_set_expired(client->oauth_client);
        hive_set_error(-1);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p) {
        hive_set_error(-1);
        return -1;
    }

    *result = p;
    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static HiveDrive *onedrive_client_drive_open(HiveClient *base, const void *opts)
{
    OneDriveClient *client = (OneDriveClient *)base;

    assert(client);
    assert(client->oauth_client);
    (void)opts;

    return onedrive_drive_open(client->oauth_client, "default");
}

static int onedrive_client_close(HiveClient *base)
{
    assert(base);

    deref(base);
    return 0;
}

static int onedrive_client_invalidate_credential(HiveClient *base)
{
    OneDriveClient *client = (OneDriveClient *)base;

    assert(client);
    assert(client->oauth_client);

    return oauth_client_set_expired(client->oauth_client);
}


static void client_destroy(void *p)
{
    OneDriveClient *client = (OneDriveClient *)p;

    if (client->oauth_client) {
        oauth_client_delete(client->oauth_client);
        client->oauth_client = NULL;
    }
}

HiveClient *onedrive_client_new(const HiveOptions *options)
{
    OneDriveOptions *opts = (OneDriveOptions *)options;
    OneDriveClient *client = NULL;
    http_client_t *httpc = NULL;
    oauth_opt_t oauth_opt;
    char *redirect_url = NULL;
    char *scheme = NULL;
    char *host = NULL;
    char *port = NULL;
    char *path = NULL;
    int rc;

    assert(opts);
    assert(opts->base.drive_type == HiveDriveType_OneDrive);

    if (!opts->client_id    || !*opts->client_id    ||
        !opts->redirect_url || !*opts->redirect_url ||
        !opts->scope        || !*opts->scope        ||
        !opts->grant_authorize) {
        hive_set_error(-1);
        return NULL;
    }

    path = realpath(options->persistent_location, oauth_opt.profile_path);
    if (!path && errno != ENOENT) {
        hive_set_error(-1);
        return NULL;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return NULL;
    }

    rc = http_client_set_url(httpc, opts->redirect_url);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_scheme(httpc, &scheme);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_host(httpc, &host);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    redirect_url = (char *)calloc(1, strlen(scheme) + strlen(host) + 4);
    if (!redirect_url) {
        hive_set_error(-1);
        goto error_exit;
    }

    sprintf(redirect_url, "%s://%s", scheme, host);
    rc = http_client_get_port(httpc, &port);
    if (rc) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_path(httpc, &path);
    if (rc <  0) {
        hive_set_error(-1);
        goto error_exit;
    }

    client = (OneDriveClient *)rc_zalloc(sizeof(OneDriveClient), client_destroy);
    if (!client) {
        hive_set_error(-1);
        goto error_exit;
    }

    oauth_opt.auth_url        = "https://login.microsoftonline.com/common/oauth2/v2.0/authorize";
    oauth_opt.token_url       = "https://login.microsoftonline.com/common/oauth2/v2.0/token";
    oauth_opt.scope           = opts->scope;
    oauth_opt.client_id       = opts->client_id;
    oauth_opt.redirect_url    = redirect_url;
    oauth_opt.redirect_port   = port;
    oauth_opt.redirect_path   = path;
    oauth_opt.grant_authorize = opts->grant_authorize;

    client->oauth_client = oauth_client_new(&oauth_opt);
    if (!client->oauth_client) {
        hive_set_error(-1);
        goto error_exit;
    }

    client->base.login                 = &onedrive_client_login;
    client->base.logout                = &onedrive_client_logout;
    client->base.get_info              = &onedrive_client_get_info;
    client->base.list_drives           = &onedrive_client_list_drives;
    client->base.drive_open            = &onedrive_client_drive_open;
    client->base.finalize              = &onedrive_client_close;

    client->base.invalidate_credential = &onedrive_client_invalidate_credential;

    return &client->base;

error_exit:
    if (host)
        http_client_memory_free(host);
    if (path)
        http_client_memory_free(path);
    if (port)
        http_client_memory_free(port);
    if (scheme)
        http_client_memory_free(scheme);
    if (redirect_url)
        free(redirect_url);
    if (httpc)
        http_client_close(httpc);
    if (client)
        deref(client);

    return NULL;
}
