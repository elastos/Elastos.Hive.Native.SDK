#include <pthread.h>
#include <crystal.h>
#include <curl/curl.h>
#include <libgen.h>
#include <stdbool.h>
#include <sys/param.h>

#include "hive.h"
#include "hive_impl.h"
#include "onedrive.h"
#include "http_client.h"


static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static enum {
    HIVE_MODULE_STATE_UNINITIALIZED,
    HIVE_MODULE_STATE_INITIALIZED,
} state;

void hive_global_cleanup()
{
    pthread_mutex_lock(&lock);
    if (state != HIVE_MODULE_STATE_INITIALIZED) {
        pthread_mutex_unlock(&lock);
        return;
    }

    http_client_cleanup();

    state = HIVE_MODULE_STATE_UNINITIALIZED;
    pthread_mutex_unlock(&lock);
}

hive_err_t hive_global_init()
{
    pthread_mutex_lock(&lock);
    if (state != HIVE_MODULE_STATE_UNINITIALIZED) {
        pthread_mutex_unlock(&lock);
        return 0;
    }

    if (http_client_init()) {
        hive_global_cleanup();
        return -1;
    }

    state = HIVE_MODULE_STATE_INITIALIZED;
    pthread_mutex_unlock(&lock);

    return 0;
}

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

    pthread_mutex_lock(&lock);
    if (state != HIVE_MODULE_STATE_INITIALIZED) {
        pthread_mutex_unlock(&lock);
        return NULL;
    }
    pthread_mutex_unlock(&lock);

    return hive_new_recipes[opt->type](opt);
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

void hive_kill(hive_t *hive)
{
    if (!hive)
        return;

    deref(hive);
}

void hive_free_json(char *json);

int hive_stat(hive_t *hive, const char *path, char **result);

int hive_set_timestamp(hive_t *hive, const char *path, const struct timeval);

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
    rc = hive->mkdir(hive, path);
    deref(hive);

    return rc;
}

int hive_move(hive_t *hive, const char *old, const char *new);

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

