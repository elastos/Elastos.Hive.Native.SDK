#include <stdlib.h>
#include <crystal.h>
#include <errno.h>
#include <crystal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <crystal.h>

#include "oauth_client.h"
#include "onedrive_client.h"
#include "onedrive_constants.h"
#include "onedrive_drive.h"
#include "http_client.h"
#include "client.h"

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
        return rc;
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
    char *access_token;
    char *p = NULL;
    int rc;

    assert(client);
    assert(result);

    rc = oauth_client_get_access_token(client->oauth_client, &access_token);
    if (rc) {
        vlogD("Hive: Get access token to login for onedrive error");
        //return ELA_GENERAL_ERROR(-1);
        return rc;
    }

    httpc = http_client_new();
    if (!httpc) {
        free(access_token);
        // TODO: rc
        return rc;
    }

    http_client_set_url(httpc, URL_API);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_set_header(httpc, "Content-Type", "application/json");
    http_client_set_header(httpc, "Authorization", access_token);
    http_client_enable_response_body(httpc);
    free(access_token);

    rc = http_client_request(httpc);
    if (rc < 0) {
        // TODO: rc
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0) {
        // TODO: rc
        goto error_exit;
    }

    if (resp_code == 401) {
        oauth_client_set_expired(client->oauth_client);
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

    *result = p;
    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static int onedrive_client_list_drives(HiveClient *base, char **result)
{
    OneDriveClient *client = (OneDriveClient *)base;
    char url[ONEDRIVE_MAX_URL_LENGTH] = {0};
    http_client_t *httpc;
    char *access_token;
    long resp_code = 0;
    char *p;
    int rc;

    assert(client);
    assert(result);

    rc = oauth_client_get_access_token(client->oauth_client, &access_token);
    if (rc) {
        // TODO: rc
        return rc;
    }

    httpc = http_client_new();
    if (!httpc) {
        free(access_token);
        // TODO: rc;
        return rc;
    }

    snprintf(url, sizeof(url), "%s/drives", URL_API);
    http_client_set_url_escape(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_enable_response_body(httpc);
    http_client_set_header(httpc, "Authorization", access_token);
    http_client_enable_response_body(httpc);
    free(access_token);

    rc = http_client_request(httpc);
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
        oauth_client_set_expired(client->oauth_client);
        // TODO: rc;
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p) {
        // TODO: rc;
        return rc;
    }

    *result = p;
    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static int onedrive_client_drive_open(HiveClient *base, HiveDrive **drive)
{
    OneDriveClient *client = (OneDriveClient *)base;
    HiveDrive *tmp;
    int rc;

    assert(client);
    assert(client->oauth_client);

    rc = onedrive_drive_open(client->oauth_client, "default", &tmp);
    if (rc < 0) {
        // TODO: rc;
        return rc;
    }

    *drive = tmp;
    return 0;
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

int onedrive_client_new(const HiveOptions *options, HiveClient **client)
{
    OneDriveOptions *opts = (OneDriveOptions *)options;
    OneDriveClient *tmp = NULL;
    http_client_t *httpc = NULL;
    oauth_opt_t oauth_opt;
    char *redirect_url = NULL;
    char *scheme = NULL;
    char *host = NULL;
    char *port = NULL;
    char *path = NULL;
    int rc = -1;

    assert(client);
    assert(opts);
    assert(opts->base.drive_type == HiveDriveType_OneDrive);

    if (!opts->client_id    || !*opts->client_id    ||
        !opts->redirect_url || !*opts->redirect_url ||
        !opts->scope        || !*opts->scope        ||
        !opts->grant_authorize) {
        // TOOD;
        return rc;
    }

    path = realpath(options->persistent_location, oauth_opt.profile_path);
    if (!path && errno != ENOENT) {
        // TODO;
        return rc;
    }

    httpc = http_client_new();
    if (!httpc) {
        // TODO;
        return rc;
    }

    rc = http_client_set_url(httpc, opts->redirect_url);
    if (rc < 0) {
        // TODO;
        goto error_exit;
    }

    rc = http_client_get_scheme(httpc, &scheme);
    if (rc < 0) {
        // TODO:
        goto error_exit;
    }

    rc = http_client_get_host(httpc, &host);
    if (rc < 0) {
        // TODO;
        goto error_exit;
    }

    redirect_url = (char *)calloc(1, strlen(scheme) + strlen(host) + 4);
    if (!redirect_url) {
        // TODO;
        goto error_exit;
    }

    sprintf(redirect_url, "%s://%s", scheme, host);
    rc = http_client_get_port(httpc, &port);
    if (rc) {
        // TODO;
        goto error_exit;
    }

    rc = http_client_get_path(httpc, &path);
    if (rc <  0) {
        // TODO;
        goto error_exit;
    }

    tmp = (OneDriveClient *)rc_zalloc(sizeof(OneDriveClient), client_destroy);
    if (!client) {
        // TODO;
        goto error_exit;
    }

    oauth_opt.auth_url        = URL_OAUTH METHOD_AUTHORIZE;
    oauth_opt.token_url       = URL_OAUTH METHOD_TOKEN;
    oauth_opt.scope           = opts->scope;
    oauth_opt.client_id       = opts->client_id;
    oauth_opt.redirect_url    = redirect_url;
    oauth_opt.redirect_port   = port;
    oauth_opt.redirect_path   = path;
    oauth_opt.grant_authorize = opts->grant_authorize;

    tmp->oauth_client = oauth_client_new(&oauth_opt);
    if (!tmp->oauth_client) {
        // TODO;
        goto error_exit;
    }

    tmp->base.login        = &onedrive_client_login;
    tmp->base.logout       = &onedrive_client_logout;
    tmp->base.get_info     = &onedrive_client_get_info;
    tmp->base.list_drives  = &onedrive_client_list_drives;
    tmp->base.get_drive    = &onedrive_client_drive_open;
    tmp->base.finalize     = &onedrive_client_close;

    tmp->base.invalidate_credential = &onedrive_client_invalidate_credential;

    *client = (HiveClient *)tmp;
    return 0;

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

    return -1;
}
