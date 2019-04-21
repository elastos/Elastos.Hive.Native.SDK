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

typedef struct oauth_client oauth_cli_t;

oauth_cli_t *oauth_cli_new(oauth_opt_t *opt);
void oauth_cli_delete(oauth_cli_t *cli);
int oauth_cli_authorize(oauth_cli_t *cli);
int oauth_cli_perform_tsx(oauth_cli_t *cli, http_client_t *http_client);

#endif // __OAUTH_CLIENT_H__
