#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
#include <pthread.h>

#include <crystal.h>
#include <cjson/cJSON.h>

#include "http_client.h"
#include "oauth_token.h"
#include "sandbird.h"

#define ARGV(args, index) (((void **)(args))[index])

typedef struct stored_token {
    const char *token_type;
    const char *access_token;
    const char *refresh_token;
    double expires_at;
} stored_token_t;

struct oauth_token {
    /*
     * main url part to get authorize code.
     */
    char *authorize_url;

    /*
     * main url part to redeem or refresh access token.
     */
    char *token_url;
    /*
     * Application registeration info.
     */
    char *client_id;
    char *scope;
    char *redirect_url;

    /*
     * tokens.
     */
    char *token_type;
    char *access_token;
    char *refresh_token;
    struct timeval expires_at;

    /*
     * Token should be wroteback as long as they have been
     * updated.
     */
    oauth_writeback_func_t *writeback_cb;
    void *user_data;

    /*
     *  padding buffer.
     */
    char padding[4];
};

static int writeback_tokens(oauth_token_t *token)
{
    cJSON *json;
    int rc;

    json = cJSON_CreateObject();
    if (!json)
        return -1;

    if (!cJSON_AddStringToObject(json, "authorize_url", token->authorize_url)) {
        cJSON_Delete(json);
        return -1;
    }

    if (!cJSON_AddStringToObject(json, "token_url", token->token_url)) {
        cJSON_Delete(json);
        return -1;
    }

    if (!cJSON_AddStringToObject(json, "client_id", token->client_id)) {
        cJSON_Delete(json);
        return -1;
    }

    if (!cJSON_AddStringToObject(json, "scope", token->scope)) {
        cJSON_Delete(json);
        return -1;
    }

    if (!cJSON_AddStringToObject(json, "redirect_url", token->redirect_url)) {
        cJSON_Delete(json);
        return -1;
    }

    if (token->token_type) {
        if (!cJSON_AddStringToObject(json, "token_type", token->token_type)) {
            cJSON_Delete(json);
            return -1;
        }

        if (!cJSON_AddStringToObject(json, "access_token", token->access_token)) {
            cJSON_Delete(json);
            return -1;
        }

        if (!cJSON_AddStringToObject(json, "refresh_token", token->refresh_token)) {
            cJSON_Delete(json);
            return -1;
        }

        if (!cJSON_AddNumberToObject(json, "expires_at", token->expires_at.tv_sec)) {
            cJSON_Delete(json);
            return -1;
        }
    }

    rc = token->writeback_cb(json, token->user_data);
    cJSON_Delete(json);
    return rc;
}

static void oauth_token_destrcutor(void *p)
{
    oauth_token_t *token = (oauth_token_t *)p;
    assert(token);

    if (token->token_type)
        free(token->token_type);

    if (token->access_token)
        free(token->access_token);

    if (token->refresh_token)
        free(token->refresh_token);
}

static bool load_stored_token(const oauth_options_t *opts, stored_token_t *token)
{
    cJSON *authorize_url;
    cJSON *token_url;
    cJSON *client_id;
    cJSON *scope;
    cJSON *redirect_url;
    cJSON *token_type;
    cJSON *access_token;
    cJSON *refresh_token;
    cJSON *expires_at;

    authorize_url = cJSON_GetObjectItemCaseSensitive(opts->store, "authorize_url");
    if (!authorize_url || !cJSON_IsString(authorize_url) || !authorize_url->valuestring ||
        strcmp(opts->authorize_url, authorize_url->valuestring))
        return false;

    token_url = cJSON_GetObjectItemCaseSensitive(opts->store, "token_url");
    if (!token_url || !cJSON_IsString(token_url) || !token_url->valuestring ||
        strcmp(opts->token_url, token_url->valuestring))
        return false;

    client_id = cJSON_GetObjectItemCaseSensitive(opts->store, "client_id");
    if (!client_id || !cJSON_IsString(client_id) || !client_id->valuestring ||
        strcmp(opts->client_id, client_id->valuestring))
        return false;

    scope = cJSON_GetObjectItemCaseSensitive(opts->store, "scope");
    if (!scope || !cJSON_IsString(scope) || !scope->valuestring ||
        strcmp(opts->scope, scope->valuestring))
        return false;

    redirect_url = cJSON_GetObjectItemCaseSensitive(opts->store, "redirect_url");
    if (!redirect_url || !cJSON_IsString(redirect_url) ||
        !redirect_url->valuestring || strcmp(opts->redirect_url,
                                             redirect_url->valuestring))
        return false;

    token_type    = cJSON_GetObjectItemCaseSensitive(opts->store, "token_type");
    access_token  = cJSON_GetObjectItemCaseSensitive(opts->store, "access_token");
    refresh_token = cJSON_GetObjectItemCaseSensitive(opts->store, "refresh_token");
    expires_at    = cJSON_GetObjectItemCaseSensitive(opts->store, "expires_at");

    if (!token_type && !access_token && !refresh_token && !expires_at) {
        memset(token, 0, sizeof(*token));
        return true;
    } else if (token_type && access_token && refresh_token && expires_at) {
        if (!cJSON_IsString(token_type) || !token_type->valuestring ||
            !*token_type->valuestring)
            return false;

        if (!cJSON_IsString(access_token) || !access_token->valuestring ||
            !*access_token->valuestring)
            return false;

        if (!cJSON_IsString(refresh_token) || !refresh_token->valuestring ||
            !*refresh_token->valuestring)
            return false;

        if (!cJSON_IsNumber(expires_at))
            return false;

        token->token_type    = token_type->valuestring;
        token->access_token  = access_token->valuestring;
        token->refresh_token = refresh_token->valuestring;
        token->expires_at    = expires_at->valuedouble;

        return true;
    } else
        return false;
}

oauth_token_t *oauth_token_new(const oauth_options_t *opts, oauth_writeback_func_t cb,
                               void *user_data)
{
    oauth_token_t *token;
    size_t extra_len = 0;
    stored_token_t stored_token;
    char *p;
    int rc;

    assert(opts);
    assert(opts->scope);
    assert(opts->scope[0]);
    assert(opts->client_id);
    assert(opts->client_id[0]);
    assert(opts->redirect_url);
    assert(opts->redirect_url[0]);
    assert(cb);

    if (opts->store && !load_stored_token(opts, &stored_token))
        return NULL;

    extra_len += strlen(opts->authorize_url) + 1;
    extra_len += strlen(opts->token_url) + 1;
    extra_len += strlen(opts->client_id) + 1;
    extra_len += strlen(opts->scope) + 1;
    extra_len += strlen(opts->redirect_url) + 1;

    token =(oauth_token_t *)rc_zalloc(sizeof(oauth_token_t) + extra_len,
                                      oauth_token_destrcutor);
    if (!token)
        return NULL;

    if (opts->store && stored_token.token_type) {
        token->token_type        = strdup(stored_token.token_type);
        token->access_token      = strdup(stored_token.access_token);
        token->refresh_token     = strdup(stored_token.refresh_token);
        token->expires_at.tv_sec = stored_token.expires_at;
        if (!token->token_type || !token->access_token || !token->refresh_token) {
            deref(token);
            return NULL;
        }
    }

    p = token->padding;
    token->authorize_url = p;
    strcpy(p, opts->authorize_url);

    p += strlen(p) + 1;
    token->token_url = p;
    strcpy(p, opts->token_url);

    p += strlen(p) + 1;
    token->client_id = p;
    strcpy(p, opts->client_id);

    p += strlen(p) + 1;
    token->scope = p;
    strcpy(p, opts->scope);

    p += strlen(p) + 1;
    token->redirect_url = p;
    strcpy(p, opts->redirect_url);

    token->writeback_cb = cb;
    token->user_data = user_data;

    if (!opts->store)
        writeback_tokens(token);

    return token;
}

int oauth_token_delete(oauth_token_t *token)
{
    int rc;
    assert(token);

    deref(token);
    return 0;
}

int oauth_token_reset(oauth_token_t *token)
{
    int rc;

    assert(token->token_type);
    assert(token->access_token);
    assert(token->refresh_token);

    free(token->token_type);
    token->token_type = NULL;

    free(token->access_token);
    token->access_token = NULL;

    free(token->refresh_token);
    token->refresh_token = NULL;

    return writeback_tokens(token);
}

static int handle_auth_redirect(sb_Event *e)
{
    bool *stop = (bool *)ARGV(e->udata, 0);
    char *path = (char *)ARGV(e->udata, 1);
    char *code = (char *)ARGV(e->udata, 2);
    size_t *sz = (size_t *)ARGV(e->udata, 3);
    int rc;

    assert(stop);
    assert(path);
    assert(code);
    assert(sz);

    if (e->type != SB_EV_REQUEST) {
        if (e->type == SB_EV_CLOSE)
            *stop = true;
        return SB_RES_OK;
    }

    if (strcmp(e->path, path))
        return SB_RES_OK;

    rc = sb_get_var(e->stream, "code", code, *sz);
    if (rc != SB_ESUCCESS)
        return SB_RES_OK;

    sb_send_status(e->stream, 200, "OK");
    return SB_RES_OK;
}

static void *request_auth_entry(void *args)
{
    oauth_request_func_t *cb = (oauth_request_func_t *)ARGV(args, 0);
    void *user_data = (void *)ARGV(args, 1);
    char *url = (char *)ARGV(args, 2);

    assert(url);
    assert(cb);

    cb(url, user_data);
    return NULL;
}

static char *get_authorize_code(oauth_token_t *token,
                                oauth_request_func_t cb, void *user_data,
                                char *code_buf, size_t bufsz)
{
    http_client_t *httpc;
    char *url;
    int rc;
    char *redirect_path;
    char *redirect_host;
    char *redirect_port;

    assert(token);
    assert(code_buf);

    /*
     * Generate the full url to make the authentication request.
     */
    httpc = http_client_new();
    if (!httpc)
        return NULL;

    http_client_set_url(httpc, token->authorize_url);
    http_client_set_query(httpc, "client_id", token->client_id);
    http_client_set_query(httpc, "scope", token->scope);
    http_client_set_query(httpc, "redirect_uri", token->redirect_url);
    http_client_set_query(httpc, "response_type", "code");

    rc = http_client_get_url_escape(httpc, &url);
    if (rc < 0) {
        http_client_close(httpc);
        return NULL;
    }

    http_client_reset(httpc);
    http_client_set_url(httpc, token->redirect_url);

    rc = http_client_get_host(httpc, &redirect_host);
    if (rc < 0) {
        http_client_close(httpc);
        http_client_memory_free(url);
        return NULL;
    }

    rc = http_client_get_port(httpc, &redirect_port);
    if (rc < 0) {
        http_client_close(httpc);
        http_client_memory_free(url);
        http_client_memory_free(redirect_host);
        return NULL;
    }

    rc = http_client_get_path(httpc, &redirect_path);
    http_client_close(httpc);
    if (rc < 0) {
        http_client_memory_free(url);
        http_client_memory_free(redirect_host);
        http_client_memory_free(redirect_port);
        return NULL;
    }

    /*
     * Create and start running a sandbird http server in order to
     * get authorize code.
     */
    {
        sb_Server *server;
        bool stopped = false;
        void *argv[] = {
            &stopped,
            redirect_path,
            code_buf,
            &bufsz
        };

        sb_Options opts = {
            .host = redirect_host,
            .port = redirect_port,
            .udata = argv,
            .handler = handle_auth_redirect,
        };

        void *thread_args[] = {
            &cb,
            user_data,
            url
        };
        pthread_t tid;

        memset(code_buf, 0, bufsz);
        server = sb_new_server(&opts);
        if (!server) {
            http_client_memory_free(url);
            http_client_memory_free(redirect_path);
            http_client_memory_free(redirect_host);
            http_client_memory_free(redirect_port);
            return NULL;
        }

        rc = pthread_create(&tid, NULL, request_auth_entry, thread_args);
        if (rc != 0) {
            sb_close_server(server);
            http_client_memory_free(url);
            http_client_memory_free(redirect_path);
            http_client_memory_free(redirect_host);
            http_client_memory_free(redirect_port);
            return NULL;
        }

        while (!stopped) {
            sb_poll_server(server, 300*1000);
        }

        pthread_join(tid, NULL);
        sb_close_server(server);
        http_client_memory_free(redirect_host);
        http_client_memory_free(redirect_port);
    }

    /*
     * Free request url allocated before.
     */
    http_client_memory_free(url);
    http_client_memory_free(redirect_path);

    return (*code_buf ? code_buf : NULL);
}

static int decode_access_token(oauth_token_t *token, const char *json_str)
{
    cJSON *json;
    cJSON *item;
    struct timeval now;
    struct timeval interval;
    char *token_type;
    char *access_token;
    char *refresh_token;

#define IS_STRING_NODE(item)    (cJSON_IsString(item) && \
                                 (item)->valuestring && *(item)->valuestring)

#define FREE(ptr)               do { \
        if ((ptr)) \
            free((ptr)); \
        (ptr) = NULL;  \
    } while(0)

    assert(token);
    assert(json_str);

    json = cJSON_Parse(json_str);
    if (!json)
        return -1;

    // Parse "token_type" item in http response.
    item = cJSON_GetObjectItemCaseSensitive(json, "token_type");
    if (!IS_STRING_NODE(item))
        goto error_exit;

    token_type = strdup(item->valuestring);
    if (!token_type)
        goto error_exit;

    // Parse 'scope' item.
    item = cJSON_GetObjectItemCaseSensitive(json, "scope");
    if (!IS_STRING_NODE(item) || strcmp(item->valuestring, token->scope)) {
        free(token_type);
        goto error_exit;
    }

    // Parse 'access_token' item
    item = cJSON_GetObjectItemCaseSensitive(json, "access_token");
    if (!IS_STRING_NODE(item)) {
        free(token_type);
        goto error_exit;
    }

    access_token = strdup(item->valuestring);
    if (!access_token) {
        free(token_type);
        goto error_exit;
    }

    // Parse 'refresh_token' item
    item = cJSON_GetObjectItemCaseSensitive(json, "refresh_token");
    if (!IS_STRING_NODE(item)) {
        free(token_type);
        free(access_token);
        goto error_exit;
    }

    refresh_token = strdup(item->valuestring);
    if (!refresh_token) {
        free(token_type);
        free(access_token);
        goto error_exit;
    }

    // Parse 'expires_in' item.
    item = cJSON_GetObjectItemCaseSensitive(json, "expires_in");
    if (!cJSON_IsNumber(item) || item->valuedouble < 0) {
        free(token_type);
        free(access_token);
        free(refresh_token);
        goto error_exit;
    }

    FREE(token->token_type);
    FREE(token->access_token);
    FREE(token->refresh_token);

    token->token_type    = token_type;
    token->access_token  = access_token;
    token->refresh_token = refresh_token;

    gettimeofday(&now, NULL);
    interval.tv_sec = (time_t)item->valuedouble;
    interval.tv_usec = 0;
    timeradd(&now, &interval, &token->expires_at);

    cJSON_Delete(json);
    return 0;

error_exit:
    cJSON_Delete(json);
    return -1;
}

static int redeem_access_token(oauth_token_t *token, char *code)
{
    http_client_t *httpc;
    char buf[512] = {0};
    char *body;
    long resp_code = 0;
    int rc;

    assert(token);
    assert(code);

    rc = snprintf(buf, sizeof(buf),
                  "client_id=%s&redirect_uri=%s&code=%s&grant_type=authorization_code",
                  token->client_id,
                  token->redirect_url, code);
    if (rc < 0 || rc >= (int)sizeof(buf))
        return -1;

    httpc = http_client_new();
    if (!httpc)
        return -1;

    body = http_client_escape(httpc, buf, strlen(buf));
    http_client_reset(httpc);

    if (!body)
        goto error_exit;

    http_client_set_url(httpc, token->token_url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_request_body_instant(httpc, body, strlen(body));
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    http_client_memory_free(body);

    if (rc < 0)
        goto error_exit;

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0)
        goto error_exit;

    if (resp_code != 200)
        goto error_exit;

    body = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!body)
        return -1;

    rc = decode_access_token(token, body);
    http_client_memory_free(body);

    return rc;

error_exit:
    http_client_close(httpc);
    return 0;
}

int oauth_token_request(oauth_token_t *token,
                        oauth_request_func_t cb, void *user_data)
{
    char authorize_code[512] = {0};
    char *code;
    int rc;

    assert(token);
    assert(cb);

    code = get_authorize_code(token, cb, user_data,
                              authorize_code, sizeof(authorize_code));
    if (!code)
        return -1;

    rc = redeem_access_token(token, code);
    if (rc < 0)
        return -1;

    return 0;
}

static int refresh_access_token(oauth_token_t *token)
{
    http_client_t *httpc;
    char buf[512] = {0};
    char *body;
    long resp_code = 0;
    int rc;

    rc = snprintf(buf, sizeof(buf),
                  "client_id=%s&redirect_uri=%s&refresh_token=%sgrant_type=refresh_token",
                  token->client_id,
                  token->redirect_url,
                  token->refresh_token);
    if (rc < 0 || rc >= sizeof(buf))
        return -1;

    httpc = http_client_new();
    if (!httpc)
        return -1;

    body = http_client_escape(httpc, buf, strlen(buf));
    http_client_reset(httpc);

    if (!body)
        goto error_exit;

    http_client_set_url(httpc, token->token_url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_request_body_instant(httpc, body, strlen(body));
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    http_client_memory_free(body);

    if (rc < 0)
        goto error_exit;

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0)
        goto error_exit;

    if (resp_code != 200)
        goto error_exit;

    body = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!body)
        return -1;

    rc = decode_access_token(token, body);
    http_client_memory_free(body);

    return  rc;

error_exit:
    http_client_close(httpc);
    return 0;
}

void oauth_token_set_expired(oauth_token_t *token)
{
    assert(token);
    memset(&token->expires_at, 0, sizeof(struct timeval));
}

bool oauth_token_is_expired(oauth_token_t *token)
{
    struct timeval now;
    assert(token);

    gettimeofday(&now, NULL);
    return timercmp(&now, &token->expires_at, >);
}

int oauth_token_check_expire(oauth_token_t *token)
{
    if (!oauth_token_is_expired(token))
        return 0;

    return refresh_access_token(token);
}

char *get_bearer_token(oauth_token_t *token)
{
    return token->access_token;
}
