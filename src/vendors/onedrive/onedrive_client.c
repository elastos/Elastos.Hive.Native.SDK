#include <stdlib.h>
#include <oauth_client.h>
#include <crystal.h>
#include <errno.h>

#include "client.h"
#include "hiveipfs.h"
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

static void onedrive_client_close(HiveClient *obj)
{
    deref(obj);
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

    onedrv_client->base.login           = &onedrive_client_login;
    onedrv_client->base.logout          = &onedrive_client_logout;
    onedrv_client->base.destructor_func = &onedrive_client_close;

    return (HiveClient *)onedrv_client;
}
