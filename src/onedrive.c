#include <limits.h>
#include <errno.h>
#include <crystal.h>
#include <stdio.h>
#include <libgen.h>
#include <cjson/cJSON.h>
#include <sys/param.h>
#include <string.h>

#include "oauth_cli.h"
#include "http_client.h"
#include "hive_impl.h"
#include "onedrive.h"

struct hive_onedrive {
    hive_t base;
    oauth_cli_t *oauth;
};
#define authorize base.authorize
#define mkdir     base.mkdir
#define list      base.list
#define copy      base.copy

#define ONEDRV_ROOT "https://graph.microsoft.com/v1.0/me"

static oauth_opt_t onedrv_oauth_opt = {
    .auth_url      = "https://login.microsoftonline.com/common/oauth2/v2.0/authorize",
    .token_url     = "https://login.microsoftonline.com/common/oauth2/v2.0/token",
    .scope         = "Files.ReadWrite.All offline_access",
    .cli_id        = "32f221de-1a46-48da-98ad-c4508dab6f32",
    .cli_secret    = "masmsCUE9741#)mrOMBM4|}",
    .redirect_port = "33822",
    .redirect_path = "/auth-redirect",
    .redirect_url  = "http://localhost"
};

static void hive_1drv_destroy(void *hive)
{
    hive_1drv_t *onedrv = (hive_1drv_t *)hive;

    if (!onedrv->oauth)
        return;

    oauth_cli_delete(onedrv->oauth);
}

static int hive_1drv_authorize(hive_t *hive)
{
    hive_1drv_t *onedrv = (hive_1drv_t *)hive;

    return oauth_cli_authorize(onedrv->oauth);
}

static int hive_1drv_copy(hive_t *hive, const char *src_path, const char *dest_path)
{
#define is_dir(str) ((str)[strlen(str) - 1] == '/' || (str)[strlen(str) - 1] == '.')
    hive_1drv_t *onedrv = (hive_1drv_t *) hive;
    char url[MAXPATHLEN + 1];
    int rc;
    http_client_t *http_cli;
    char *src_path_esc;
    cJSON *req_body, *parent_ref;
    char src_dir[MAXPATHLEN + 1];
    char src_base[MAXPATHLEN + 1];
    const char *dst_dir;
    const char *dst_base;
    bool dir_eq = false, base_eq = false;
    char *req_body_str;
    long resp_code;

    strcpy(src_dir, dirname((char *)src_path));
    strcpy(src_base, basename((char *)src_path));
    dst_dir = is_dir(dest_path) ? dest_path : dirname((char *)dest_path);
    dst_base = is_dir(dest_path) ? src_base : basename((char *)dest_path);

    if (!strcmp(src_dir, dst_dir) || !strcmp(dst_dir, ".") || !strcmp(dst_dir, "./"))
        dir_eq = true;
    if (!strcmp(src_base, dst_base))
        base_eq = true;
    if (dir_eq && base_eq)
        return -1;

    if (!strcmp(src_path, "/"))
        rc = snprintf(url, sizeof(url), "%s/drive/root/copy", ONEDRV_ROOT);
    else {
        http_cli = http_client_new();
        if (!http_cli)
            return -1;
        src_path_esc = http_client_escape(http_cli, src_path, strlen(src_path));
        if (!src_path_esc)
            return -1;
        rc = snprintf(url, sizeof(url), "%s/drive/root:%s:/copy", ONEDRV_ROOT, src_path_esc);
        http_client_memory_free(src_path_esc);
    }
    if (rc >= sizeof(url))
        return -1;

    req_body = cJSON_CreateObject();
    if (!req_body)
        return -1;

    if (!dir_eq) {
        char parent_dir[MAXPATHLEN + 1];

        if (dest_path[0] == '/')
            rc = snprintf(parent_dir, sizeof(parent_dir), "/drive/root:%s", dst_dir);
        else
            rc = snprintf(parent_dir, sizeof(parent_dir), "/drive/root:%s/%s",
                !strcmp(src_dir, "/") ? "" : src_dir, dst_dir);
        if (rc >= sizeof(parent_dir)) {
            cJSON_Delete(req_body);
            return -1;
        }
        if (parent_dir[strlen(parent_dir) - 1] == '/')
            parent_dir[strlen(parent_dir) - 1] = '\0';
        parent_ref = cJSON_AddObjectToObject(req_body, "parentReference");
        if (!parent_ref) {
            cJSON_Delete(req_body);
            return -1;
        }
        if (!cJSON_AddStringToObject(parent_ref, "path", parent_dir)) {
            cJSON_Delete(req_body);
            return -1;
        }
    }

    if (!base_eq) {
        if (!cJSON_AddStringToObject(req_body, "name", dst_base)) {
            cJSON_Delete(req_body);
            return -1;
        }
    }

    req_body_str = cJSON_Print(req_body);
    cJSON_Delete(req_body);
    if (!req_body_str)
        return -1;

    http_cli = http_client_new();
    if (!http_cli)
        return -1;

    http_client_set_url_escape(http_cli, url);
    http_client_set_method(http_cli, HTTP_METHOD_POST);
    http_client_set_header(http_cli, "Content-Type", "application/json");
    http_client_set_request_body_instant(http_cli, req_body_str, strlen(req_body_str));

    rc = oauth_cli_perform_tsx(onedrv->oauth, http_cli);
    free(req_body_str);
    if (rc)
        return -1;

    rc = http_client_get_response_code(http_cli, &resp_code);
    if (rc < 0 || resp_code != 202)
        return -1;

    return 0;
#undef is_dir
}

static int hive_1drv_list(hive_t *hive, const char *path, char **result)
{
#define RESP_BODY_MAX_SZ 16384
    hive_1drv_t *onedrv = (hive_1drv_t *)hive;
    char url[MAXPATHLEN + 1];
    int rc, val_sz;
    char *path_esc;
    char resp_body[RESP_BODY_MAX_SZ];
    size_t resp_body_len = sizeof(resp_body);
    long resp_code;
    cJSON *resp_part, *next_link, *resp, *val_part, *elem, *val;
    http_client_t *http_cli;

    if (!strcmp(path, "/"))
        rc = snprintf(url, sizeof(url), "%s/drive/root/children", ONEDRV_ROOT);
    else {
        http_cli = http_client_new();
        if (!http_cli)
            return -1;
        path_esc = http_client_escape(http_cli, path, strlen(path));
        if (!path_esc)
            return -1;
        rc = snprintf(url, sizeof(url), "%s/drive/root:%s:/children", ONEDRV_ROOT, path_esc);
        http_client_memory_free(path_esc);
    }
    if (rc >= sizeof(url))
        return -1;

    resp = cJSON_CreateObject();
    if (!resp)
        return -1;

    val = cJSON_AddArrayToObject(resp, "value");
    if (!val) {
        cJSON_Delete(resp);
        return -1;
    }

    while (true) {
        http_cli = http_client_new();
        if (!http_cli) {
            cJSON_Delete(resp);
            return -1;
        }

        http_client_set_url_escape(http_cli, url);
        http_client_set_method(http_cli, HTTP_METHOD_GET);
        http_client_set_response_body_instant(http_cli, resp_body, resp_body_len);

        rc = oauth_cli_perform_tsx(onedrv->oauth, http_cli);
        if (rc) {
            cJSON_Delete(resp);
            return -1;
        }

        rc = http_client_get_response_code(http_cli, &resp_code);
        if (rc < 0 || resp_code != 200) {
            cJSON_Delete(resp);
            return -1;
        }

        resp_body_len = http_client_get_response_body_length(http_cli);
        if (resp_body_len == sizeof(resp_body)) {
            cJSON_Delete(resp);
            return -1;
        }
        resp_body[resp_body_len] = '\0';

        resp_part = cJSON_Parse(resp_body);
        if (!resp_part) {
            cJSON_Delete(resp);
            return -1;
        }

        val_part = cJSON_GetObjectItemCaseSensitive(resp_part, "value");
        if (!val_part || !cJSON_IsArray(val_part) || !(val_sz = cJSON_GetArraySize(val_part))) {
            cJSON_Delete(resp);
            cJSON_Delete(resp_part);
            return -1;
        }

        for (int i = 0; i < val_sz; ++i) {
            elem = cJSON_DetachItemFromArray(val_part, 0);
            cJSON_AddItemToArray(val, elem);
        }

        next_link = cJSON_GetObjectItemCaseSensitive(resp_part, "@odata.nextLink");
        if (next_link && (!cJSON_IsString(next_link) || !*next_link->valuestring)) {
            cJSON_Delete(resp);
            cJSON_Delete(resp_part);
            return -1;
        }

        if (next_link) {
            rc = snprintf(url, sizeof(url), "%s", next_link->valuestring);
            if (rc >= sizeof(url)) {
                cJSON_Delete(resp);
                cJSON_Delete(resp_part);
                return -1;
            }
            resp_body_len = sizeof(resp_body);
            cJSON_Delete(resp_part);
            continue;
        }

        *result = cJSON_Print(resp);
        cJSON_Delete(resp);
        cJSON_Delete(resp_part);
        return *result ? 0 : 1;
    }
#undef RESP_BODY_MAX_SZ
}

static int hive_1drv_mkdir(hive_t *hive, const char *path)
{
#define RESP_BODY_MAX_SZ 2048
    hive_1drv_t *onedrv = (hive_1drv_t *)hive;
    char url[MAXPATHLEN + 1];
    cJSON *req_body;
    int rc;
    char resp_body[RESP_BODY_MAX_SZ];
    size_t resp_body_len = sizeof(resp_body);
    long resp_code;
    char *req_body_str;
    char *dir;
    char *dir_esc;
    http_client_t *http_cli;

    dir = dirname((char *)path);
    if (!strcmp(dir, "/"))
        rc = snprintf(url, sizeof(url), "%s/drive/root/children", ONEDRV_ROOT);
    else {
        http_cli = http_client_new();
        if (!http_cli)
            return -1;
        dir_esc = http_client_escape(http_cli, dir, strlen(dir));
        if (!dir_esc)
            return -1;
        rc = snprintf(url, sizeof(url), "%s/drive/root:%s:/children", ONEDRV_ROOT, dir_esc);
        http_client_memory_free(dir_esc);
    }
    if (rc >= sizeof(url))
        return -1;

    req_body = cJSON_CreateObject();
    if (!req_body)
        return -1;

    if (!cJSON_AddStringToObject(req_body, "name", basename((char *)path))) {
        cJSON_Delete(req_body);
        return -1;
    }

    if (!cJSON_AddObjectToObject(req_body, "folder")) {
        cJSON_Delete(req_body);
        return -1;
    }

    if (!cJSON_AddStringToObject(req_body, "@microsoft.graph.conflictBehavior", "rename")) {
        cJSON_Delete(req_body);
        return -1;
    }

    req_body_str = cJSON_Print(req_body);
    cJSON_Delete(req_body);
    if (!req_body_str)
        return -1;

    http_cli = http_client_new();
    if (!http_cli)
        return -1;

    http_client_set_url_escape(http_cli, url);
    http_client_set_method(http_cli, HTTP_METHOD_POST);
    http_client_set_header(http_cli, "Content-Type", "application/json");
    http_client_set_request_body_instant(http_cli, req_body_str, strlen(req_body_str));
    http_client_set_response_body_instant(http_cli, resp_body, resp_body_len);

    rc = oauth_cli_perform_tsx(onedrv->oauth, http_cli);
    free(req_body_str);
    if (rc)
        return -1;

    rc = http_client_get_response_code(http_cli, &resp_code);
    if (rc < 0 || resp_code != 201)
        return -1;

    resp_body_len = http_client_get_response_body_length(http_cli);
    if (resp_body_len == sizeof(resp_body))
        return -1;
    resp_body[resp_body_len] = '\0';

    return 0;
#undef RESP_BODY_MAX_SZ
}

hive_t *hive_1drv_new(const hive_opt_t *base_opt)
{
    hive_1drv_t *onedrv;
    hive_1drv_opt_t *opt = (hive_1drv_opt_t *)base_opt;
    oauth_opt_t oauth_opt;

    if (!*opt->profile_path || !opt->open_oauth_url)
        return NULL;

    oauth_opt = onedrv_oauth_opt;

    if (!realpath(opt->profile_path, oauth_opt.profile_path) && errno != ENOENT)
        return NULL;

    oauth_opt.open_url = opt->open_oauth_url;

    onedrv = rc_zalloc(sizeof(hive_1drv_t), hive_1drv_destroy);
    if (!onedrv)
        return NULL;

    onedrv->oauth = oauth_cli_new(&oauth_opt);
    if (!onedrv->oauth) {
        deref(onedrv);
        return NULL;
    }

    onedrv->authorize = hive_1drv_authorize;
    onedrv->mkdir     = hive_1drv_mkdir;
    onedrv->list      = hive_1drv_list;
    onedrv->copy      = hive_1drv_copy;

    return (hive_t *)onedrv;
}
