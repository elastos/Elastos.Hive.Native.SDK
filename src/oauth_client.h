#ifndef __OAUTH_CLIENT_H__
#define __OAUTH_CLIENT_H__

#include <limits.h>
#if defined(_WIN32) || defined(_WIN64)
#include <crystal.h>
#endif

#include "http_client.h"

typedef struct oauth_option {
    const char *auth_url;
    const char *token_url;
    const char *scope;
    const char *client_id;
    const char *redirect_port;
    const char *redirect_path;
    const char *redirect_url;
    char profile_path[PATH_MAX];
    int (*grant_authorize)(const char *url);
} oauth_opt_t;

typedef struct oauth_client oauth_client_t;

oauth_client_t *oauth_client_new(oauth_opt_t *opt);
void oauth_client_delete(oauth_client_t *client);
int oauth_client_login(oauth_client_t *client);
int oauth_client_logout(oauth_client_t *client);
int oauth_client_get_access_token(oauth_client_t *client, char **token);
int oauth_client_refresh_access_token(oauth_client_t *client, char **token);


char *token_get_access_token(oauth_client_t *client);

#endif // __OAUTH_CLIENT_H__
