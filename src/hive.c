#include <pthread.h>
#include <crystal.h>
#include <curl/curl.h>
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include <stdbool.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "hive.h"
#include "hive_impl.h"
#include "onedrive.h"
#include "http_client.h"

const char *hive_get_version(void)
{
    return NULL;
}

static hive_t *(*hive_new_recipes[])(const hive_opt_t *) = {
    hive_1drv_new
};

hive_t *hive_new(const hive_opt_t *opt)
{
    if (!opt || opt->type < 0 || opt->type >= HIVE_TYPE_NONEXIST)
        return NULL;

    return hive_new_recipes[opt->type](opt);
}

int hive_set_expired(hive_t *hive)
{
    int rc;

    if (!hive)
        return -1;

    ref(hive);
    rc = hive->set_expired(hive);
    deref(hive);

    return rc;
}

int hive_authorize(hive_t *hive)
{
    int rc;

    if (!hive)
        return -1;

    ref(hive);
    rc = hive->authorize(hive);
    deref(hive);

    return rc;
}

int hive_revoke(hive_t *hive)
{
    int rc;

    if (!hive)
        return -1;

    ref(hive);
    rc = hive->revoke(hive);
    deref(hive);

    return rc;
}

void hive_kill(hive_t *hive)
{
    if (!hive)
        return;

    deref(hive);
}

void hive_free_json(char *json);

int hive_stat(hive_t *hive, const char *path, char **result)
{
    int rc;

    if (!hive || !path || !result || path[0] != '/')
        return -1;

    ref(hive);
    rc = hive->stat(hive, path, result);
    deref(hive);

    return rc;
}

int hive_set_timestamp(hive_t *hive, const char *path, const struct timeval timestamp);

int hive_list(hive_t *hive, const char *path, char **result)
{
    int rc;

    if (!hive || !path || path[0] != '/' || !result)
       return -1;

    ref(hive);
    rc = hive->list(hive, path, result);
    deref(hive);

    return rc;
}

int hive_mkdir(hive_t *hive, const char *path)
{
    int rc;

    if (!hive || !path || path[0] != '/' || path[1] == '\0')
        return -1;

    ref(hive);
    rc = hive->makedir(hive, path);
    deref(hive);

    return rc;
}

int hive_move(hive_t *hive, const char *old, const char *new)
{
    int rc;

    if (!hive || !old || !new || old[0] != '/' ||
        new[0] == '\0' || strlen(old) > MAXPATHLEN ||
        strlen(new) > MAXPATHLEN || !strcmp(old, new))
        return -1;

    ref(hive);
    rc = hive->move(hive, old, new);
    deref(hive);

    return rc;
}

int hive_copy(hive_t *hive, const char *src_path, const char *dest_path)
{
    int rc;

    if (!hive || !src_path || !dest_path || src_path[0] != '/' ||
        dest_path[0] == '\0' || strlen(src_path) > MAXPATHLEN ||
        strlen(dest_path) > MAXPATHLEN || !strcmp(src_path, dest_path))
        return -1;

    ref(hive);
    rc = hive->copy(hive, src_path, dest_path);
    deref(hive);

    return rc;
}

int hive_delete(hive_t *hive, const char *path)
{
    int rc;

    if (!hive || !path || path[0] != '/' || path[1] == '\0')
        return -1;

    ref(hive);
    rc = hive->delete(hive, path);
    deref(hive);

    return rc;
}

int hive_async_stat(hive_t *hive, const char *path,
    hive_resp_cbs_t *callbacks, void *context);

int hive_async_set_timestamp(hive_t *hive, const char *path,
    const struct timeval, hive_resp_cbs_t *callbacks, void *context);

int hive_async_list(hive_t *hive, const char *path,
    hive_resp_cbs_t *callbacks, void *context);

int hive_async_mkdir(hive_t *hive, const char *path,
    hive_resp_cbs_t *callbacks, void *context);

int hive_async_move(hive_t *hive, const char *old, const char *new,
    hive_resp_cbs_t *callbacks, void *context);

int hive_async_copy(hive_t *hive, const char *src_path, const char *dest_path,
        hive_resp_cbs_t *callbacks, void *context);

int hive_async_delete(hive_t *hive, const char *path,
        hive_resp_cbs_t *callbacks, void *context);

