#ifndef __OAUTH_CLIENT_H__
#define __OAUTH_CLIENT_H__

#include <limits.h>

#include "http_client.h"

typedef struct oauth_option {
    const char *auth_url;
    const char *token_url;
    const char *scope;
    const char *cli_id;
    const char *cli_secret;
    const char *redirect_port;
    const char *redirect_path;
    const char *redirect_url;
    char profile_path[PATH_MAX];
    void (*open_url)(const char *url);
} oauth_opt_t;

typedef struct oauth_client oauth_client_t;

oauth_client_t *oauth_client_new(oauth_opt_t *opt);
void oauth_client_delete(oauth_client_t *client);
int oauth_client_authorize(oauth_client_t *client);
int oauth_client_get_access_token(oauth_client_t *client, char **token);
int oauth_client_refresh_access_token(oauth_client_t *client, char **token);

#endif // __OAUTH_CLIENT_H__
