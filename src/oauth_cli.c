#include <pthread.h>
#include <stdlib.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif
#include <crystal.h>
#if defined(HAVE_IO_H)
#include <io.h>
#endif
#include <sys/stat.h>
#include <assert.h>
#include <stdbool.h>
#include <errno.h>
#include <cjson/cJSON.h>

#include "http_cli.h"
#include "oauth_cli.h"
#include "sandbird.h"

typedef struct server_response {
    char *auth_code;
    char *token_type;
    struct timeval expires_at;
    char *scope;
    char *access_token;
    char *refresh_token;
} svr_resp_t;

typedef struct oauth_client {
    enum {
        OAUTH_CLI_STATE_UNAUTHORIZED,
        OAUTH_CLI_STATE_AUTHORIZING,
        OAUTH_CLI_STATE_AUTHORIZED,
        OAUTH_CLI_STATE_INSTANT_REFRESH,
        OAUTH_CLI_STATE_REFRESHING,
        OAUTH_CLI_STATE_STOPPED,
        OAUTH_CLI_STATE_FAILED,
        OAUTH_CLI_STATE_CLEANUP
    } state;
    oauth_opt_t     opt;
    svr_resp_t      svr_resp;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
} oauth_cli_t;

#define reset_svr_resp(resp) do { \
        if ((resp)->auth_code) free((resp)->auth_code); \
        if ((resp)->token_type) free((resp)->token_type); \
        if ((resp)->scope) free((resp)->scope); \
        if ((resp)->access_token) free((resp)->access_token); \
        if ((resp)->refresh_token) free((resp)->refresh_token); \
    } while (0)

static char *encode_profile(oauth_cli_t *cli)
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

    auth_code = cJSON_CreateStringReference(cli->svr_resp.auth_code);
    if (!auth_code)
        goto end;
    cJSON_AddItemToObject(json, "auth_code", auth_code);

    token_type = cJSON_CreateStringReference(cli->svr_resp.token_type);
    if (!token_type)
        goto end;
    cJSON_AddItemToObject(json, "token_type", token_type);

    expires_at = cJSON_CreateNumber(cli->svr_resp.expires_at.tv_sec);
    if (!expires_at)
        goto end;
    cJSON_AddItemToObject(json, "expires_at", expires_at);

    scope = cJSON_CreateStringReference(cli->svr_resp.scope);
    if (!scope)
        goto end;
    cJSON_AddItemToObject(json, "scope", scope);

    access_token = cJSON_CreateStringReference(cli->svr_resp.access_token);
    if (!access_token)
        goto end;
    cJSON_AddItemToObject(json, "access_token", access_token);

    refresh_token = cJSON_CreateStringReference(cli->svr_resp.refresh_token);
    if (!refresh_token)
        goto end;
    cJSON_AddItemToObject(json, "refresh_token", refresh_token);

    json_str = cJSON_Print(json);

end:
    if (json)
        cJSON_Delete(json);
    return json_str;
}

static void save_profile(oauth_cli_t *cli)
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

    fd = open(new_prof, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        free(json_str);
        return;
    }

    nleft = strlen(json_str) + 1;
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

static int perform_token_tsx(oauth_cli_t *cli, char *req_body, void *resp_body, size_t *resp_body_len)
{
    http_client_t *http_cli;
    long resp_code;
    int rc;

    http_cli = http_client_init();
    if (!http_cli)
        return -1;

    http_client_set_url(http_cli, cli->opt.token_url);
    http_client_set_method(http_cli, HTTP_METHOD_POST);
    http_client_set_request_body_instant(http_cli, req_body, strlen(req_body));
    http_client_set_response_body_instant(http_cli, resp_body, *resp_body_len);

    rc = http_client_request(http_cli);
    if (rc)
        return -1;

    resp_code = http_client_get_response_code(http_cli);
    if (resp_code != 200)
        return -1;

    *resp_body_len = http_client_get_response_body_length(http_cli);
    return 0;
}

static int decode_access_token_resp(oauth_cli_t *cli, const char *json_str, bool load_profile)
{
    cJSON *json;
    cJSON *auth_code;
    cJSON *token_type;
    cJSON *expires_in;
    cJSON *scope;
    cJSON *access_token;
    cJSON *refresh_token;
    cJSON *expires_at;
    int rc = 0;
    svr_resp_t resp;

    memset(&resp, 0, sizeof(resp));

    json = cJSON_Parse(json_str);
    if (!json)
        return -1;

    if (load_profile) {
        expires_at = cJSON_GetObjectItemCaseSensitive(json, "expires_at");
        if (!cJSON_IsNumber(expires_at))
            goto fail;
        resp.expires_at.tv_sec = expires_at->valuedouble;

        auth_code = cJSON_GetObjectItemCaseSensitive(json, "auth_code");
        if (!cJSON_IsString(auth_code) || !auth_code->valuestring || !*auth_code->valuestring)
            goto fail;
        resp.auth_code = auth_code->valuestring;
        auth_code->valuestring = NULL;
    } else {
        expires_in = cJSON_GetObjectItemCaseSensitive(json, "expires_in");
        if (!cJSON_IsNumber(expires_in) || (int)expires_in->valuedouble < 0)
            goto fail;

        struct timeval now;
        rc = gettimeofday(&now, NULL);
        if (rc)
            goto fail;
        resp.expires_at.tv_sec = now.tv_sec + expires_in->valuedouble;
    }

    token_type = cJSON_GetObjectItemCaseSensitive(json, "token_type");
    if (!cJSON_IsString(token_type) || !token_type->valuestring || !*token_type->valuestring)
        goto fail;
    resp.token_type = token_type->valuestring;
    token_type->valuestring = NULL;

    scope = cJSON_GetObjectItemCaseSensitive(json, "scope");
    if (!cJSON_IsString(scope) || !scope->valuestring || !*scope->valuestring)
        goto fail;
    resp.scope = scope->valuestring;
    scope->valuestring = NULL;

    access_token = cJSON_GetObjectItemCaseSensitive(json, "access_token");
    if (!cJSON_IsString(access_token) || !access_token->valuestring || !*access_token->valuestring)
        goto fail;
    resp.access_token = access_token->valuestring;
    access_token->valuestring = NULL;

    refresh_token = cJSON_GetObjectItemCaseSensitive(json, "refresh_token");
    if (!cJSON_IsString(refresh_token) || !refresh_token->valuestring || !*refresh_token->valuestring)
        goto fail;
    resp.refresh_token = refresh_token->valuestring;
    refresh_token->valuestring = NULL;

    if (load_profile)
        cli->svr_resp.auth_code = resp.auth_code;
    cli->svr_resp.access_token = resp.access_token;
    cli->svr_resp.refresh_token = resp.refresh_token;
    cli->svr_resp.scope = resp.scope;
    cli->svr_resp.expires_at = resp.expires_at;
    cli->svr_resp.token_type = resp.token_type;
    goto succeed;

fail:
    reset_svr_resp(&resp);
    rc = -1;
succeed:
    cJSON_Delete(json);
    return rc;
}

static int refresh_token(oauth_cli_t *cli)
{
#define REQ_BODY_MAX_SZ 2048
#define RESP_BODY_MAX_SZ 2048
    char req_body[REQ_BODY_MAX_SZ];
    char resp_body[RESP_BODY_MAX_SZ];
    size_t resp_body_len = sizeof(resp_body);
    int rc;
    char redirect_url_raw[128];
    http_client_t *http_cli;
    char *cli_id;
    char *cli_secret;
    char *refresh_token;
    char *redirect_url;

    rc = snprintf(redirect_url_raw, sizeof(redirect_url_raw), "%s:%s%s",
                  cli->opt.redirect_url, cli->opt.redirect_port, cli->opt.redirect_path);
    if (rc < 0 || rc >= sizeof(redirect_url_raw))
        return -1;

    http_cli = http_client_init();
    if (!http_cli)
        return -1;

    cli_id = http_client_escape(http_cli, cli->opt.cli_id, strlen(cli->opt.cli_id));
    if (!cli_id)
        return -1;

    redirect_url = http_client_escape(http_cli, redirect_url_raw, strlen(redirect_url_raw));
    if (!redirect_url) {
        http_client_memory_free(cli_id);
        return -1;
    }

    cli_secret = http_client_escape(http_cli, cli->opt.cli_secret, strlen(cli->opt.cli_secret));
    if (!cli_secret) {
        http_client_memory_free(cli_id);
        http_client_memory_free(redirect_url);
        return -1;
    }

    refresh_token = http_client_escape(http_cli, cli->svr_resp.refresh_token, strlen(cli->svr_resp.refresh_token));
    if (!refresh_token) {
        http_client_memory_free(cli_id);
        http_client_memory_free(redirect_url);
        http_client_memory_free(cli_secret);
        return -1;
    }

    rc = snprintf(req_body, sizeof(req_body),
        "client_id=%s&"
        "redirect_uri=%s&"
        "client_secret=%s&"
        "refresh_token=%s&"
        "grant_type=refresh_token",
        cli_id,
        redirect_url,
        cli_secret,
        refresh_token);
    http_client_memory_free(cli_id);
    http_client_memory_free(redirect_url);
    http_client_memory_free(cli_secret);
    http_client_memory_free(refresh_token);
    if (rc < 0 || rc >= sizeof(req_body))
        return -1;

    rc = perform_token_tsx(cli, req_body, resp_body, &resp_body_len);
    if (rc)
        return -1;

    if (resp_body_len == sizeof(resp_body))
        return -1;
    resp_body[resp_body_len] = '\0';

    rc = decode_access_token_resp(cli, resp_body, false);
    if (rc)
        return -1;

    return 0;
#undef REQ_BODY_MAX_SZ
#undef RESP_BODY_MAX_SZ
}

static void *refresh_token_routine(void *arg)
{
    oauth_cli_t *cli = (oauth_cli_t *)arg;
    int rc;
    struct timespec next_refresh;

    pthread_mutex_lock(&cli->lock);
    while (cli->state < OAUTH_CLI_STATE_AUTHORIZED)
        pthread_cond_wait(&cli->cond, &cli->lock);
    if (cli->state >= OAUTH_CLI_STATE_STOPPED)
        goto cleanup;

schedule_refresh:
    rc = 0;
    next_refresh.tv_sec = cli->svr_resp.expires_at.tv_sec;
    next_refresh.tv_nsec = cli->svr_resp.expires_at.tv_usec * 1000;

    while (cli->state == OAUTH_CLI_STATE_AUTHORIZED && rc != ETIMEDOUT)
        rc = pthread_cond_timedwait(&cli->cond, &cli->lock, &next_refresh);
    if (cli->state >= OAUTH_CLI_STATE_STOPPED)
        goto cleanup;
    assert(cli->state == OAUTH_CLI_STATE_AUTHORIZED || cli->state == OAUTH_CLI_STATE_INSTANT_REFRESH);

    cli->state = OAUTH_CLI_STATE_REFRESHING;
    pthread_mutex_unlock(&cli->lock);

    rc = refresh_token(cli);

    pthread_mutex_lock(&cli->lock);
    if (cli->state != OAUTH_CLI_STATE_REFRESHING) {
        assert(cli->state >= OAUTH_CLI_STATE_STOPPED);
        goto cleanup;
    } else if (rc) {
        cli->state = OAUTH_CLI_STATE_FAILED;
        goto cleanup;
    }
    cli->state = OAUTH_CLI_STATE_AUTHORIZED;
    pthread_cond_broadcast(&cli->cond);
    save_profile(cli);
    goto schedule_refresh;

cleanup:
    while (cli->state != OAUTH_CLI_STATE_CLEANUP)
        pthread_cond_wait(&cli->cond, &cli->lock);
    pthread_mutex_unlock(&cli->lock);
    reset_svr_resp(&cli->svr_resp);
    pthread_mutex_destroy(&cli->lock);
    pthread_cond_destroy(&cli->cond);
    return NULL;
}

static int load_profile(oauth_cli_t *cli)
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

    if (json_str[sbuf.st_size - 1]) {
        close(fd);
        free(json_str);
        return -1;
    }

    rc = decode_access_token_resp(cli, json_str, true);

    close(fd);
    free(json_str);
    return rc;
}

static int handle_auth_redirect(sb_Event *e)
{
    bool *finish = (bool *)(((void **)e->udata)[1]);
    oauth_cli_t *cli = (oauth_cli_t *)(((void **)e->udata)[0]);
    char code[128];
    int rc;

    if (e->type != SB_EV_REQUEST) {
        if (e->type == SB_EV_CLOSE)
            *finish = true;
        return SB_RES_OK;
    }

    if (strncmp(e->path, cli->opt.redirect_path, strlen(cli->opt.redirect_path)))
        return SB_RES_OK;

    rc = sb_get_var(e->stream, "code", code, sizeof(code));
    if (rc != SB_ESUCCESS)
        return SB_RES_OK;

    cli->svr_resp.auth_code = (char *)malloc(strlen(code) + 1);
    if (!cli->svr_resp.auth_code)
        return SB_RES_OK;

    strcpy(cli->svr_resp.auth_code, code);
    sb_send_status(e->stream, 200, "OK");
    return SB_RES_OK;
}

static void *open_auth_url(void *args)
{
    http_client_t *http_cli = (http_client_t *)(((void **)args)[0]);
    int *res = (int *)(((void **)args)[1]);
    oauth_cli_t *cli = (oauth_cli_t *)(((void **)args)[2]);
    char *url;
    int rc;

    rc = http_client_get_url(http_cli, &url);
    if (rc) {
        *res = -1;
        return NULL;
    }
    *res = 0;

    cli->opt.open_url(url);

    http_client_memory_free(url);
    return NULL;
}

static int get_auth_code(oauth_cli_t *cli)
{
    sb_Options opt;
    sb_Server *server;
    pthread_t tid;
    int rc, trc = 1;
    http_client_t *http_cli;
    char buf[512];
    bool finish = false;

    http_cli = http_client_init();
    if (!http_cli)
        return -1;

    http_client_set_url(http_cli, cli->opt.auth_url);

    rc = snprintf(buf, sizeof(buf), "%s:%s%s",
        cli->opt.redirect_url, cli->opt.redirect_port, cli->opt.redirect_path);
    assert(rc < sizeof(buf));

    http_client_set_query(http_cli, "client_id", cli->opt.cli_id);
    http_client_set_query(http_cli, "scope", cli->opt.scope);
    http_client_set_query(http_cli, "redirect_uri", buf);
    http_client_set_query(http_cli, "response_type", "code");

    void *args1[] = {cli, &finish};
    memset(&opt, 0, sizeof(opt));
    opt.port = cli->opt.redirect_port;
    opt.handler = handle_auth_redirect;
    opt.udata = args1;

    server = sb_new_server(&opt);
    if (!server)
        return -1;

    void *args2[] = {http_cli, &trc, cli};
    rc = pthread_create(&tid, NULL, open_auth_url, args2);
    if (rc) {
        sb_close_server(server);
        return -1;
    }
    pthread_detach(tid);

    while (trc == 1) ;
    if (trc) {
        sb_close_server(server);
        return -1;
    }

    while (!finish)
        sb_poll_server(server, 300000);
    sb_close_server(server);
    return cli->svr_resp.auth_code ? 0 : -1;
}

static int redeem_access_token(oauth_cli_t *cli)
{
#define REQ_BODY_MAX_SZ 512
#define RESP_BODY_MAX_SZ 2048
    char req_body[REQ_BODY_MAX_SZ];
    char resp_body[RESP_BODY_MAX_SZ];
    size_t resp_body_len = sizeof(resp_body);
    int rc;
    char redirect_url_raw[128];
    http_client_t *http_cli;
    char *cli_id;
    char *cli_secret;
    char *code;
    char *redirect_url;

    rc = snprintf(redirect_url_raw, sizeof(redirect_url_raw), "%s:%s%s",
        cli->opt.redirect_url, cli->opt.redirect_port, cli->opt.redirect_path);
    if (rc < 0 || rc >= sizeof(redirect_url_raw))
        return -1;

    http_cli = http_client_init();
    if (!http_cli)
        return -1;

    cli_id = http_client_escape(http_cli, cli->opt.cli_id, strlen(cli->opt.cli_id));
    if (!cli_id)
        return -1;

    redirect_url = http_client_escape(http_cli, redirect_url_raw, strlen(redirect_url_raw));
    if (!redirect_url) {
        http_client_memory_free(cli_id);
        return -1;
    }

    cli_secret = http_client_escape(http_cli, cli->opt.cli_secret, strlen(cli->opt.cli_secret));
    if (!cli_secret) {
        http_client_memory_free(cli_id);
        http_client_memory_free(redirect_url);
        return -1;
    }

    code = http_client_escape(http_cli, cli->svr_resp.auth_code, strlen(cli->svr_resp.auth_code));
    if (!code) {
        http_client_memory_free(cli_id);
        http_client_memory_free(redirect_url);
        http_client_memory_free(cli_secret);
        return -1;
    }

    rc = snprintf(req_body, sizeof(req_body),
        "client_id=%s&"
        "redirect_uri=%s&"
        "client_secret=%s&"
        "code=%s&"
        "grant_type=authorization_code",
        cli_id,
        redirect_url,
        cli_secret,
        code);
    http_client_memory_free(cli_id);
    http_client_memory_free(redirect_url);
    http_client_memory_free(cli_secret);
    http_client_memory_free(code);
    if (rc < 0 || rc >= sizeof(req_body))
        return -1;

    rc = perform_token_tsx(cli, req_body, resp_body, &resp_body_len);
    if (rc)
        return -1;

    if (resp_body_len == sizeof(resp_body))
        return -1;
    resp_body[resp_body_len] = '\0';

    rc = decode_access_token_resp(cli, resp_body, false);
    if (rc)
        return -1;

    return 0;
#undef REQ_BODY_MAX_SZ
#undef RESP_BODY_MAX_SZ
}

oauth_cli_t *oauth_cli_new(oauth_opt_t *opt)
{
    oauth_cli_t *cli;
    int rc;
    pthread_t worker_tid;

    cli = (oauth_cli_t *)calloc(1, sizeof(oauth_cli_t));
    if (!cli)
        return NULL;

    rc = pthread_mutex_init(&cli->lock, NULL);
    if (rc) {
        free(cli);
        return NULL;
    }

    rc = pthread_cond_init(&cli->cond, NULL);
    if (rc) {
        pthread_mutex_destroy(&cli->lock);
        free(cli);
        return NULL;
    }

    cli->opt = *opt;
    if (!access(opt->profile_path, F_OK)) {
        rc = load_profile(cli);
        if (rc) {
            pthread_mutex_destroy(&cli->lock);
            pthread_cond_destroy(&cli->cond);
            free(cli);
            return NULL;
        }
        cli->state = OAUTH_CLI_STATE_AUTHORIZED;
    }

    rc = pthread_create(&worker_tid, NULL, refresh_token_routine, cli);
    if (rc) {
        pthread_mutex_destroy(&cli->lock);
        pthread_cond_destroy(&cli->cond);
        free(cli);
        return NULL;
    }
    pthread_detach(worker_tid);

    return cli;
}

void oauth_cli_delete(oauth_cli_t *cli)
{
    pthread_mutex_lock(&cli->lock);
    if (cli->state == OAUTH_CLI_STATE_CLEANUP)
        return;
    cli->state = OAUTH_CLI_STATE_CLEANUP;
    pthread_mutex_unlock(&cli->lock);
    pthread_cond_signal(&cli->cond);
}

int oauth_cli_authorize(oauth_cli_t *cli)
{
    int rc;

    pthread_mutex_lock(&cli->lock);
    if (cli->state != OAUTH_CLI_STATE_UNAUTHORIZED) {
        pthread_mutex_unlock(&cli->lock);
        return -1;
    }
    cli->state = OAUTH_CLI_STATE_AUTHORIZING;
    pthread_mutex_unlock(&cli->lock);

    rc = get_auth_code(cli);
    pthread_mutex_lock(&cli->lock);
    if (cli->state != OAUTH_CLI_STATE_AUTHORIZING) {
        pthread_mutex_unlock(&cli->lock);
        return -1;
    } else if (rc) {
        cli->state = OAUTH_CLI_STATE_FAILED;
        pthread_mutex_unlock(&cli->lock);
        return -1;
    }
    pthread_mutex_unlock(&cli->lock);

    rc = redeem_access_token(cli);
    pthread_mutex_lock(&cli->lock);
    if (cli->state != OAUTH_CLI_STATE_AUTHORIZING) {
        pthread_mutex_unlock(&cli->lock);
        return -1;
    } else if (rc) {
        cli->state = OAUTH_CLI_STATE_FAILED;
        pthread_mutex_unlock(&cli->lock);
        return -1;
    }

    cli->state = OAUTH_CLI_STATE_AUTHORIZED;
    save_profile(cli);

    pthread_mutex_unlock(&cli->lock);
    pthread_cond_signal(&cli->cond);

    return 0;
}

http_client_t *oauth_cli_perform_tsx(oauth_cli_t *cli,
    void (*configure_req)(http_client_t *, void *), void *user_data)
{
    char auth_hdr[2048];
    int rc;
    long resp_code;
    http_client_t *http_cli;

    pthread_mutex_lock(&cli->lock);
    if (cli->state < OAUTH_CLI_STATE_AUTHORIZED || cli->state > OAUTH_CLI_STATE_REFRESHING) {
        pthread_mutex_unlock(&cli->lock);
        return NULL;
    }
retry:
    while (cli->state == OAUTH_CLI_STATE_INSTANT_REFRESH || cli->state == OAUTH_CLI_STATE_REFRESHING)
        pthread_cond_wait(&cli->cond, &cli->lock);
    if (cli->state >= OAUTH_CLI_STATE_STOPPED) {
        pthread_mutex_unlock(&cli->lock);
        return NULL;
    }
    assert(cli->state == OAUTH_CLI_STATE_AUTHORIZED);

    rc = snprintf(auth_hdr, sizeof(auth_hdr), "%s %s",
        cli->svr_resp.token_type, cli->svr_resp.access_token);
    if (rc >= sizeof(auth_hdr)) {
        pthread_mutex_unlock(&cli->lock);
        return NULL;
    }
    pthread_mutex_unlock(&cli->lock);

    http_cli = http_client_init();
    if (!http_cli)
        return NULL;

    configure_req(http_cli, user_data);
    http_client_set_header(http_cli, "Authorization", auth_hdr);

    rc = http_client_request(http_cli);
    if (rc)
        return NULL;

    resp_code = http_client_get_response_code(http_cli);
    if (resp_code < 0)
        return NULL;
    else if (resp_code == 401) {
        pthread_mutex_lock(&cli->lock);
        if (cli->state == OAUTH_CLI_STATE_AUTHORIZED) {
            cli->state = OAUTH_CLI_STATE_INSTANT_REFRESH;
            pthread_cond_broadcast(&cli->cond);
        } else if (cli->state >= OAUTH_CLI_STATE_STOPPED) {
            pthread_mutex_unlock(&cli->lock);
            return NULL;
        }
        assert(cli->state == OAUTH_CLI_STATE_INSTANT_REFRESH || cli->state == OAUTH_CLI_STATE_REFRESHING);
        goto retry;
    }

    return http_cli;
}
