#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif

#include <crystal.h>
#include <cjson/cJSON.h>

#include "ela_hive.h"
#include "hive_error.h"
#include "http_client.h"
#include "oauth_token.h"
#include "sandbird.h"
#include "http_status.h"

#define ARGV(args, index) (((void **)(args))[index])

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
};

static void writeback_tokens(oauth_token_t *token)
{
    cJSON *json;
    int rc;

    assert(token);

    json = cJSON_CreateObject();
    if (!json) {
        vlogE("OauthToken: Failed to create cjson object");
        return;
    }

    cJSON_AddStringToObject(json, "client_id", token->client_id);
    if (token->token_type) {
        assert(token->access_token);
        assert(token->refresh_token);
        cJSON_AddStringToObject(json, "token_type", token->token_type);
        cJSON_AddStringToObject(json, "access_token", token->access_token);
        cJSON_AddStringToObject(json, "refresh_token", token->refresh_token);
        cJSON_AddNumberToObject(json, "expires_at", token->expires_at.tv_sec);
    }

    rc = token->writeback_cb(json, token->user_data);
    if (rc < 0)
        vlogE("OauthToken: Write access token to persistent storage error");

    cJSON_Delete(json);
}

static void oauth_token_destrcutor(void *p)
{
    oauth_token_t *token = (oauth_token_t *)p;
    assert(token);

    if (token->token_type)
        free(token->token_type);
}

static int restore_access_token(const cJSON *json, oauth_token_t *token)
{
    cJSON *client_id;
    cJSON *token_type;
    cJSON *access_token;
    cJSON *refresh_token;
    cJSON *expires_at;
    size_t mem_len = 0;
    char *p;

    assert(token);
    assert(token->client_id);

    if (!json) {
        vlogI("OauthToken: No cache is found");
        return 0;
    }

    client_id = cJSON_GetObjectItem(json, "client_id");
    if (!cJSON_IsString(client_id) ||
        strcmp(token->client_id, client_id->valuestring) != 0) {
        vlogE("OauthToken: Missing client_id json object.");
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_PERSISTENT_DATA);
    }

    token_type    = cJSON_GetObjectItem(json, "token_type");
    access_token  = cJSON_GetObjectItem(json, "access_token");
    refresh_token = cJSON_GetObjectItem(json, "refresh_token");
    expires_at    = cJSON_GetObjectItem(json, "expires_at");

    if (!cJSON_IsString(token_type) ||
        !cJSON_IsString(access_token) ||
        !cJSON_IsString(refresh_token) ||
        !cJSON_IsNumber(expires_at)) {
        vlogI("OauthToken: Cache does not contain access token.");
        return 0;
    }

    mem_len += strlen(token_type->valuestring) + 1;
    mem_len += strlen(access_token->valuestring) + 1;
    mem_len += strlen(refresh_token->valuestring) + 1;

    token->expires_at.tv_sec = (long)expires_at->valuedouble;
    p = (char *)calloc(1, mem_len);
    if (!p)
        return HIVE_SYS_ERROR(errno);

    token->token_type = p;
    strcpy(p, token_type->valuestring);
    p += strlen(p) + 1;

    token->access_token = p;
    strcpy(p, access_token->valuestring);
    p += strlen(p) + 1;

    token->refresh_token = p;
    strcpy(p, refresh_token->valuestring);

    vlogI("OauthToken: Successfully restore cached access token");

    return 0;
}

oauth_token_t *oauth_token_new(const oauth_options_t *opts, oauth_writeback_func_t *cb,
                               void *user_data)
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
    assert(opts->authorize_url);
    assert(opts->authorize_url[0]);
    assert(opts->token_url);
    assert(opts->token_url[0]);
    assert(cb);

    extra_len += strlen(opts->authorize_url) + 1;
    extra_len += strlen(opts->token_url) + 1;
    extra_len += strlen(opts->client_id) + 1;
    extra_len += strlen(opts->scope) + 1;
    extra_len += strlen(opts->redirect_url) + 1;

    token =(oauth_token_t *)rc_zalloc(sizeof(oauth_token_t) + extra_len,
                                      oauth_token_destrcutor);
    if (!token) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        return NULL;
    }

    /*
     * set value for essential fields from options.
     */
    p = (char *)(token + 1);

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

    /*
     * try restore access/refresh token from parsed json object.
     */
    rc = restore_access_token(opts->store, token);
    if (rc < 0)
        vlogW("OauthToken: Could not load access token from json object");

    return token;
}

int oauth_token_delete(oauth_token_t *token)
{
    assert(token);
    deref(token);
    return 0;
}

int oauth_token_reset(oauth_token_t *token)
{
    assert(token->token_type);

    free(token->token_type);
    token->token_type = NULL;
    token->access_token = NULL;
    token->refresh_token = NULL;

    writeback_tokens(token);

    return 0;
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

    if (strcmp(e->path, path)){
        vlogE("OauthToken: Wrong redirect URL path.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_BAD_ADDRESS));
        return SB_RES_OK;
    }

    rc = sb_get_var(e->stream, "code", code, *sz);
    if (rc != SB_ESUCCESS) {
        vlogE("OauthToken: Missing authorization code.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_CREDENTIAL));
        return SB_RES_OK;
    }

    sb_send_status(e->stream, 200, "OK");
    return SB_RES_OK;
}

static void *request_auth_entry(void *args)
{
    oauth_request_func_t *cb = (oauth_request_func_t *)ARGV(args, 0);
    void *user_data = ARGV(args, 1);
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
    if (!httpc) {
        vlogE("OauthToken: Failed to create http client.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        return NULL;
    }

    http_client_set_url(httpc, token->authorize_url);
    http_client_set_query(httpc, "client_id", token->client_id);
    http_client_set_query(httpc, "scope", token->scope);
    http_client_set_query(httpc, "redirect_uri", token->redirect_url);
    http_client_set_query(httpc, "response_type", "code");

    rc = http_client_get_url_escape(httpc, &url);
    if (rc) {
        vlogE("OauthToken: Wrong redirect URL path.");
        hive_set_error(HIVE_CURLU_ERROR(rc));
        http_client_close(httpc);
        return NULL;
    }

    http_client_reset(httpc);
    http_client_set_url(httpc, token->redirect_url);

    rc = http_client_get_host(httpc, &redirect_host);
    if (rc) {
        vlogE("OauthToken: Failed to get redirect URL host.");
        hive_set_error(HIVE_CURLU_ERROR(rc));
        http_client_close(httpc);
        http_client_memory_free(url);
        return NULL;
    }

    rc = http_client_get_port(httpc, &redirect_port);
    if (rc) {
        vlogE("OauthToken: Failed to get redirect URL port.");
        hive_set_error(HIVE_CURLU_ERROR(rc));
        http_client_close(httpc);
        http_client_memory_free(url);
        http_client_memory_free(redirect_host);
        return NULL;
    }

    rc = http_client_get_path(httpc, &redirect_path);
    http_client_close(httpc);
    if (rc) {
        vlogE("OauthToken: Failed to get redirect URL path.");
        hive_set_error(HIVE_CURLU_ERROR(rc));
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
            cb,
            user_data,
            url
        };
        pthread_t tid;

        memset(code_buf, 0, bufsz);
        server = sb_new_server(&opts);
        if (!server) {
            vlogE("OauthToken: Failed to create server.");
            http_client_memory_free(url);
            http_client_memory_free(redirect_path);
            http_client_memory_free(redirect_host);
            http_client_memory_free(redirect_port);
            return NULL;
        }

        rc = pthread_create(&tid, NULL, request_auth_entry, thread_args);
        if (rc != 0) {
            vlogE("OauthToken: Failed to create thread.");
            hive_set_error(HIVE_SYS_ERROR(errno));
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
    int rc;

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
    if (!json) {
        vlogE("OauthToken: Failed to parse json string.");
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
    }

    // Parse "token_type" item in http response.
    item = cJSON_GetObjectItemCaseSensitive(json, "token_type");
    if (!IS_STRING_NODE(item)) {
        vlogE("OauthToken: Json object named token_type doesn't exist.");
        rc = HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
        goto error_exit;
    }

    token_type = strdup(item->valuestring);
    if (!token_type) {
        vlogE("OauthToken: Failed to duplicate token type.");
        rc = HIVE_SYS_ERROR(errno);
        goto error_exit;
    }

    // Parse 'scope' item.
    item = cJSON_GetObjectItemCaseSensitive(json, "scope");
    if (!IS_STRING_NODE(item)) {
        vlogE("OauthToken: Json object named scope doesn't exist.");
        rc = HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
        free(token_type);
        goto error_exit;
    }

    // Parse 'access_token' item
    item = cJSON_GetObjectItemCaseSensitive(json, "access_token");
    if (!IS_STRING_NODE(item)) {
        vlogE("OauthToken: Json object named access_token doesn't exist.");
        rc = HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
        free(token_type);
        goto error_exit;
    }

    access_token = strdup(item->valuestring);
    if (!access_token) {
        vlogE("OauthToken: Failed to duplicate access token.");
        rc = HIVE_SYS_ERROR(errno);
        free(token_type);
        goto error_exit;
    }

    // Parse 'refresh_token' item
    item = cJSON_GetObjectItemCaseSensitive(json, "refresh_token");
    if (!IS_STRING_NODE(item)) {
        vlogE("OauthToken: Json object named refresh_token doesn't exist.");
        rc = HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
        free(token_type);
        free(access_token);
        goto error_exit;
    }

    refresh_token = strdup(item->valuestring);
    if (!refresh_token) {
        vlogE("OauthToken: Failed to duplicate refresh token.");
        rc = HIVE_SYS_ERROR(errno);
        free(token_type);
        free(access_token);
        goto error_exit;
    }

    // Parse 'expires_in' item.
    item = cJSON_GetObjectItemCaseSensitive(json, "expires_in");
    if (!cJSON_IsNumber(item) || item->valuedouble < 0) {
        vlogE("OauthToken: Json object named expires_in doesn't exist.");
        rc = HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
        free(token_type);
        free(access_token);
        free(refresh_token);
        goto error_exit;
    }

    FREE(token->token_type);

    token->token_type    = token_type;
    token->access_token  = access_token;
    token->refresh_token = refresh_token;

    gettimeofday(&now, NULL);
    interval.tv_sec = (long)item->valuedouble;
    interval.tv_usec = 0;
    timeradd(&now, &interval, &token->expires_at);

    cJSON_Delete(json);
    return 0;

error_exit:
    cJSON_Delete(json);
    return rc;
}

static int redeem_access_token(oauth_token_t *token, char *code)
{
    http_client_t *httpc;
    char buf[512] = {0};
    char *body;
    long resp_code = 0;
    char *client_id;
    char *redirect_uri;
    char *code_escaped;
    int rc;

    assert(token);
    assert(code);

    httpc = http_client_new();
    if (!httpc) {
        vlogE("OauthToken: Failed to create http client instance.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    client_id = http_client_escape(httpc, token->client_id, strlen(token->client_id));
    if (!client_id) {
        vlogE("OauthToken: Failed to escape client id.");
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    redirect_uri = http_client_escape(httpc, token->redirect_url,
                                      strlen(token->redirect_url));
    if (!redirect_uri) {
        vlogE("OauthToken: Failed to escape redirect uri.");
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        http_client_memory_free(client_id);
        goto error_exit;
    }

    code_escaped = http_client_escape(httpc, code, strlen(code));
    if (!code_escaped) {
        vlogE("OauthToken: Failed to escape authorization code.");
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        http_client_memory_free(client_id);
        http_client_memory_free(redirect_uri);
        goto error_exit;
    }

    rc = snprintf(buf, sizeof(buf),
                  "client_id=%s&redirect_uri=%s&code=%s&grant_type=authorization_code",
                  client_id, redirect_uri, code_escaped);
    http_client_memory_free(client_id);
    http_client_memory_free(redirect_uri);
    http_client_memory_free(code_escaped);
    if (rc < 0 || rc >= (int)sizeof(buf)) {
        vlogE("OauthToken: Failed to print redeem token request body.");
        rc = HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);
        goto error_exit;
    }

    http_client_set_url(httpc, token->token_url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_request_body_instant(httpc, buf, strlen(buf));
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc) {
        vlogE("OauthToken: Failed to perform http request.");
        rc = HIVE_CURL_ERROR(rc);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0) {
        vlogE("OauthToken: Failed to get response code.");
        rc = HIVE_CURL_ERROR(rc);
        goto error_exit;
    }

    if (resp_code != HttpStatus_OK) {
        vlogE("OauthToken: Error from http response (%d).", resp_code);
        rc = HIVE_HTTP_STATUS_ERROR(resp_code);
        goto error_exit;
    }

    body = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!body) {
        vlogE("OauthToken: Failed to get response body.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    rc = decode_access_token(token, body);
    free(body);

    return rc;

error_exit:
    http_client_close(httpc);
    return rc;
}

int oauth_token_request(oauth_token_t *token,
                        oauth_request_func_t *cb, void *user_data)
{
    char authorize_code[512] = {0};
    char *code;
    int rc;

    assert(token);
    assert(cb);

    if (token->access_token) {
        vlogI("OauthToken: Access token already exists.");
        return 0;
    }

    code = get_authorize_code(token, cb, user_data,
                              authorize_code, sizeof(authorize_code));
    if (!code)
        return hive_get_error();

    vlogI("OauthToken: Successfully get authorization code.");

    rc = redeem_access_token(token, code);
    if (rc < 0)
        return rc;

    vlogI("OauthToken: Successfully redeem access token.");

    writeback_tokens(token);

    return 0;
}

static int refresh_access_token(oauth_token_t *token)
{
    http_client_t *httpc;
    char buf[4096] = {0};
    char *body;
    long resp_code = 0;
    char *client_id;
    char *redirect_uri;
    char *refresh_token;
    int rc;

    httpc = http_client_new();
    if (!httpc) {
        vlogE("OauthToken: Failed to create http client instance.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    client_id = http_client_escape(httpc, token->client_id, strlen(token->client_id));
    if (!client_id) {
        vlogE("OauthToken: Failed to escape client id.");
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    redirect_uri = http_client_escape(httpc, token->redirect_url,
                                      strlen(token->redirect_url));
    if (!redirect_uri) {
        vlogE("OauthToken: Failed to escape redirect uri.");
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        http_client_memory_free(client_id);
        goto error_exit;
    }

    refresh_token = http_client_escape(httpc, token->refresh_token,
                                       strlen(token->refresh_token));
    if (!refresh_token) {
        vlogE("OauthToken: Failed to escape refresh token.");
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        http_client_memory_free(client_id);
        http_client_memory_free(redirect_uri);
        goto error_exit;
    }

    rc = snprintf(buf, sizeof(buf),
                  "client_id=%s&redirect_uri=%s&refresh_token=%s&grant_type=refresh_token",
                  client_id, redirect_uri, refresh_token);
    http_client_memory_free(client_id);
    http_client_memory_free(redirect_uri);
    http_client_memory_free(refresh_token);
    if (rc < 0 || rc >= sizeof(buf)) {
        vlogE("OauthToken: Failed to print refresh access token request body.");
        rc = HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);
        goto error_exit;
    }

    http_client_set_url(httpc, token->token_url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_request_body_instant(httpc, buf, strlen(buf));
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc) {
        vlogE("OauthToken: Failed to perform refresh access token request.");
        rc = HIVE_CURL_ERROR(rc);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc) {
        vlogE("OauthToken: Failed to get refresh access token response code.");
        rc = HIVE_CURL_ERROR(rc);
        goto error_exit;
    }

    if (resp_code != HttpStatus_OK) {
        vlogE("OauthToken: Failed to refresh access token.");
        rc = HIVE_HTTP_STATUS_ERROR(resp_code);
        goto error_exit;
    }

    body = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!body) {
        vlogE("OauthToken: Failed to get refresh access token response body.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    rc = decode_access_token(token, body);
    free(body);
    if (rc < 0)
        return rc;

    writeback_tokens(token);

    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
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
    int rc;

    if (!oauth_token_is_expired(token))
        return 0;

    vlogI("OauthToken: Access token expired.");

    rc = refresh_access_token(token);
    if (rc < 0) {
        vlogE("OauthToken: Failed to refresh access token.");
        return rc;
    }

    vlogI("OauthToken: Successfully refreshed access token.");
    return 0;
}

char *get_bearer_token(oauth_token_t *token)
{
    return token->access_token;
}
