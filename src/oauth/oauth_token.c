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

#define ARGV(args, index) ((void **)args)[index]

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
     * Lock;
     */
    pthread_mutex_t lock;

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

/*
static char *encode_profile(oauth_client_deprecated_t *cli)
{
    cJSON *json = NULL;
    cJSON *auth_code;
    cJSON *token_type;
    cJSON *expires_at;
    cJSON *scope;
    cJSON *access_token;
    cJSON *refresh_token;
    char *json_str = NULL;

    json = cJSON_CreateObject();
    if (!json)
        goto end;

    pthread_mutex_lock(&cli->lock);
    auth_code = cJSON_CreateStringReference(cli->svr_resp.auth_code);
    if (!cli->svr_resp.auth_code || !auth_code)
        goto end;
    cJSON_AddItemToObject(json, "auth_code", auth_code);

    token_type = cJSON_CreateStringReference(cli->svr_resp.token_type);
    if (!cli->svr_resp.token_type || !token_type)
        goto end;
    cJSON_AddItemToObject(json, "token_type", token_type);

    expires_at = cJSON_CreateNumber(cli->svr_resp.expires_at.tv_sec);
    if (!expires_at)
        goto end;
    cJSON_AddItemToObject(json, "expires_at", expires_at);

    scope = cJSON_CreateStringReference(cli->svr_resp.scope);
    if (!cli->svr_resp.scope || !scope)
        goto end;
    cJSON_AddItemToObject(json, "scope", scope);

    access_token = cJSON_CreateStringReference(cli->svr_resp.access_token);
    if (!cli->svr_resp.access_token || !access_token)
        goto end;
    cJSON_AddItemToObject(json, "access_token", access_token);

    refresh_token = cJSON_CreateStringReference(cli->svr_resp.refresh_token);
    if (!cli->svr_resp.refresh_token || !refresh_token)
        goto end;
    cJSON_AddItemToObject(json, "refresh_token", refresh_token);

    json_str = cJSON_PrintUnformatted(json);

end:
    pthread_mutex_unlock(&cli->lock);
    if (json)
        cJSON_Delete(json);
    return json_str;
}

static void save_profile(oauth_client_deprecated_t *cli)
{
    char *json_str = NULL;
    int fd;
    char tmp_profile_path_new[PATH_MAX];
    char tmp_profile_path_old[PATH_MAX];
    char *new_prof;
    int rc;
    size_t nleft;
    bool old_prof_exists = true;

    if (access(cli->opt.profile_path, F_OK)) {
        old_prof_exists = false;
        new_prof = cli->opt.profile_path;
    } else {
        strcpy(tmp_profile_path_new, cli->opt.profile_path);
        strcat(tmp_profile_path_new, ".new");
        new_prof = tmp_profile_path_new;
    }

    json_str = encode_profile(cli);
    if (!json_str)
        return;

    fd = open(new_prof, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        free(json_str);
        return;
    }

    nleft = strlen(json_str);
    while (nleft) {
        ssize_t nwr = write(fd, json_str, nleft);
        if (nwr < 0) {
            free(json_str);
            close(fd);
            remove(new_prof);
            return;
        }
        nleft -= nwr;
    }
    free(json_str);
    close(fd);

    if (old_prof_exists) {
        strcpy(tmp_profile_path_old, cli->opt.profile_path);
        strcat(tmp_profile_path_old, ".old");
        rc = rename(cli->opt.profile_path, tmp_profile_path_old);
        if (rc) {
            remove(new_prof);
            return;
        }

        rc = rename(new_prof, cli->opt.profile_path);
        if (rc) {
            rename(tmp_profile_path_old, cli->opt.profile_path);
            remove(new_prof);
            return;
        }

        remove(tmp_profile_path_old);
    }
}

static int load_profile(oauth_client_deprecated_t *cli)
{
    struct stat sbuf;
    int fd;
    char *json_str;
    off_t nleft;
    int rc;

    fd = open(cli->opt.profile_path, O_RDONLY);
    if (fd < 0)
        return -1;

    if (fstat(fd, &sbuf)) {
        close(fd);
        return -1;
    }

    if (!sbuf.st_size) {
        close(fd);
        return -1;
    }
    nleft = sbuf.st_size;

    json_str = (char *)malloc(sbuf.st_size);
    if (!json_str) {
        close(fd);
        return -1;
    }

    while (nleft) {
        ssize_t nrd;
        nrd = read(fd, json_str, sbuf.st_size);
        if (nrd < 0) {
            close(fd);
            free(json_str);
            return -1;
        }
        nleft -= nrd;
    }

    rc = decode_access_token_resp(cli, json_str, true);

    close(fd);
    free(json_str);
    return rc;
}

*/

static int load_stored_tokens(oauth_token_t *token, const cJSON *json)
{
    // TODO;
    return -1;
}

static int writeback_tokens(oauth_token_t *token)
{
    // TODO;
    return -1;
}

static void oauth_token_destrcutor(void *p)
{
    oauth_token_t *token = (oauth_token_t *)p;
    assert(token);

    if (token->access_token)
        free(token->access_token);

    if (token->refresh_token)
        free(token->refresh_token);
}

oauth_token_t *oauth_token_new(oauth_options_t *opts,
                               oauth_writeback_func_t cb, void *user_data)
{
    oauth_token_t *token;
    size_t extra_len = 0;
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

    extra_len += strlen(opts->authorize_url) + 1;
    extra_len += strlen(opts->token_url) + 1;
    extra_len += strlen(opts->client_id) + 1;
    extra_len += strlen(opts->scope) + 1;
    extra_len += strlen(opts->redirect_url) + 1;

    token =(oauth_token_t *)rc_zalloc(sizeof(oauth_token_t) + extra_len,
                                      oauth_token_destrcutor);
    if (!token)
        return NULL;

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

    rc = load_stored_tokens(token, opts->store);
    if (rc < 0) {
        deref(token);
        return NULL;
    }

    pthread_mutex_init(&token->lock, NULL);

    token->writeback_cb = cb;
    token->user_data = user_data;

    return token;
}

int oauth_token_delete(oauth_token_t *token)
{
    int rc;
    assert(token);

    rc = writeback_tokens(token);
    if (rc < 0)
        vlogW("Hive: Write back tokens error");

    pthread_mutex_destroy(&token->lock);

    deref(token);
    return  0;
}

int oauth_token_reset(oauth_token_t *token)
{
    // TODO;
    return -1;
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

    if (e->type == SB_EV_CLOSE)
        *stop = true;

    if (e->type != SB_EV_CLOSE)
        return SB_RES_OK;

    if (strcmp(e->path, path))
        return SB_RES_OK;

    rc = sb_get_var(e->stream, code, NULL, *sz); // TODO:
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

static char * get_authorize_code(oauth_token_t *token,
                                 oauth_request_func_t cb, void *user_data,
                                 char *code_buf, size_t bufsz)
{
    http_client_t *httpc;
    char *url;
    char *direct_path; // TODO;
    int rc;

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
    http_client_set_query(httpc, "redirect_url", token->redirect_url);
    http_client_set_query(httpc, "response_type", "code");

    rc = http_client_get_url_escape(httpc, &url);
    http_client_close(httpc);

    if (rc < 0)
        return NULL;

    /*
     * Create and start running a sandbird http server in order to
     * get authorize code.
     */
    {
        sb_Server *server;
        char code_buf[256] = {0};
        bool stopped = false;
        void *argv[] = {
            &stopped,
            direct_path,
            code_buf,
            &bufsz
        };

        sb_Options opts = {
            .port = 0,
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
            return NULL;
        }

        rc = pthread_create(&tid, NULL, request_auth_entry, thread_args);
        if (rc != 0) {
            sb_close_server(server);
            http_client_memory_free(url);
            return NULL;
        }

        while (!stopped) {
            sb_poll_server(server, 300*1000);
        }

        pthread_join(tid, NULL);
        sb_close_server(server);
    }

    /*
     * Free request url allocated before.
     */
    http_client_memory_free(url);

    return (*code_buf ? code_buf : NULL);
}

static int decode_access_token(oauth_token_t *token, const char *json_str)
{
    cJSON *json;
    cJSON *item;
    struct timeval now;
    struct timeval interval;

#define IS_STRING_NODE(item)    (cJSON_IsString(item) && \
                                 item->valuestring && *item->valuestring)

#define FREE(ptr)               do { \
        if ((ptr)) \
            free((ptr)); \
        (ptr) = NULL;  \
    } while(0)

    assert(token);
    assert(json_str);

    FREE(token->access_token);
    FREE(token->refresh_token);

    json = cJSON_Parse(json_str);
    if (!json)
        return -1;

    // Parse "token_type" item in http response.
    item = cJSON_GetObjectItemCaseSensitive(json, "token_type");
    if (!IS_STRING_NODE(item))
        goto error_exit;

    strcpy(token->token_type, item->valuestring);

    // Parse 'scope' item.
    item = cJSON_GetObjectItemCaseSensitive(json, "scope");
    if (!IS_STRING_NODE(item) || strcmp(item->valuestring, token->scope))
        goto error_exit;

    // Parse 'access_token' item
    item = cJSON_GetObjectItemCaseSensitive(json, "access_token");
    if (!IS_STRING_NODE(item))
        goto error_exit;

    token->access_token = strdup(item->valuestring);
    if (!token->access_token)
        goto error_exit;

    item = cJSON_GetObjectItemCaseSensitive(json, "refresh_token");
    if (!IS_STRING_NODE(item))
        goto error_exit;

    token->refresh_token = strdup(item->valuestring);
    if (!token->refresh_token)
        goto error_exit;

    // Parse 'expires_in' item.
    item = cJSON_GetObjectItemCaseSensitive(json, "expires_in");
    if (!cJSON_IsNumber(item) || item->valuedouble < 0)
        goto error_exit;

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
                  "client_id=%s&redirect_url=%s&code=%s&grant_type=authorization_code",
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
        goto error_exit;

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
                  "client_id=%s&redirect_url=%s&refresh_token=%sgrant_type=refresh_token",
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
        goto error_exit;

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
    // TODO;
    return NULL;
}
