#include <stdio.h>
#include <stddef.h>
#include <pthread.h>
#include <crystal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#include <http_client.h>
#include <stdlib.h>

#endif

#include "hiveipfs_client.h"
#include "hiveipfs_drive.h"

typedef enum state {
    NOT_LOGGED_IN,
    LOGGING_IN,
    LOGGED_IN,
    ERRORED
} state_t;

typedef struct IpfsClient {
    HiveClient base;

    pthread_mutex_t lock;
    pthread_cond_t cond;
    state_t state;
    char *uid;
    char *svr_addr;
} IpfsClient;

#define CLUSTER_API_PORT (9094)
#define NODE_API_PORT (9095)

static int ipfs_client_login(HiveClient *obj)
{
    IpfsClient *client = (IpfsClient *)obj;
    int rc;
    char url[MAXPATHLEN + 1];
    http_client_t *http_client = NULL;
    long resp_code;
    state_t state_to_set = ERRORED;

    pthread_mutex_lock(&client->lock);
    while (client->state == LOGGING_IN)
        pthread_cond_wait(&client->cond, &client->lock);
    if (client->state >= LOGGED_IN) {
        rc = client->state == LOGGED_IN ? 0 : -1;
        pthread_mutex_unlock(&client->lock);
        return rc;
    }
    client->state = LOGGING_IN;
    pthread_mutex_unlock(&client->lock);

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/uid/login",
        client->svr_addr, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url))
        goto end;

    http_client = http_client_new();
    if (!http_client)
        goto end;

    http_client_set_url(http_client, url);
    http_client_set_query(http_client, "uid", client->uid);

    rc = http_client_request(http_client);
    if (rc)
        goto end;

    rc = http_client_get_response_code(http_client, &resp_code);
    if (rc || resp_code != 200)
        goto end;

    state_to_set = LOGGED_IN;

end:
    if (http_client)
        http_client_close(http_client);
    pthread_mutex_lock(&client->lock);
    if (client->state == LOGGING_IN) {
        client->state = state_to_set;
        pthread_cond_broadcast(&client->cond);
    }
    rc = client->state == LOGGED_IN ? 0 : -1;
    pthread_mutex_unlock(&client->lock);
    return rc;
}

static int ipfs_client_logout(HiveClient *obj)
{
    IpfsClient *client = (IpfsClient *)obj;
    int rc = -1;

    pthread_mutex_lock(&client->lock);
    if (client->state != ERRORED) {
        client->state = NOT_LOGGED_IN;
        rc = 0;
    }
    pthread_mutex_unlock(&client->lock);

    return rc;
}

static int ipfs_client_get_info(HiveClient *obj, char **result)
{
#define RESP_BODY_MAX_SZ 256
    IpfsClient *client = (IpfsClient *)obj;
    int rc;
    http_client_t *http_client;
    char url[MAXPATHLEN + 1];
    char resp_body[RESP_BODY_MAX_SZ];
    size_t resp_body_len = sizeof(resp_body);
    long resp_code;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/uid/info",
        client->svr_addr, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    http_client = http_client_new();
    if (!http_client)
        return -1;

    http_client_set_url(http_client, url);
    http_client_set_query(http_client, "uid", client->uid);
    http_client_set_response_body_instant(http_client, resp_body, resp_body_len);

    rc = http_client_request(http_client);
    if (rc) {
        http_client_close(http_client);
        return -1;
    }

    rc = http_client_get_response_code(http_client, &resp_code);
    if (rc || resp_code != 200) {
        http_client_close(http_client);
        return -1;
    }

    resp_body_len = http_client_get_response_body_length(http_client);
    http_client_close(http_client);
    if (result) {
        *result = (char *)malloc(resp_body_len + 1);
        if (!*result)
            return -1;
        memcpy(*result, resp_body, resp_body_len);
        (*result)[resp_body_len] = '\0';
    }
    return 0;
#undef RESP_BODY_MAX_SZ
}

static int ipfs_client_list_drives(HiveClient *obj, char **result)
{
#define RESP_BODY_MAX_SZ 512
    IpfsClient *client = (IpfsClient *)obj;
    int rc;
    http_client_t *http_client;
    char url[MAXPATHLEN + 1];
    char resp_body[RESP_BODY_MAX_SZ];
    size_t resp_body_len = sizeof(resp_body);
    long resp_code;

    pthread_mutex_lock(&client->lock);
    while (client->state == LOGGING_IN)
        pthread_cond_wait(&client->cond, &client->lock);
    if (client->state != LOGGED_IN) {
        pthread_mutex_unlock(&client->lock);
        return -1;
    }
    pthread_mutex_unlock(&client->lock);

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/files/stat",
        client->svr_addr, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    http_client = http_client_new();
    if (!http_client)
        return -1;

    http_client_set_url(http_client, url);
    http_client_set_query(http_client, "uid", client->uid);
    http_client_set_query(http_client, "path", "/");
    http_client_set_response_body_instant(http_client, resp_body, resp_body_len);

    rc = http_client_request(http_client);
    if (rc) {
        http_client_close(http_client);
        return -1;
    }

    rc = http_client_get_response_code(http_client, &resp_code);
    if (rc || resp_code != 200) {
        http_client_close(http_client);
        return -1;
    }

    resp_body_len = http_client_get_response_body_length(http_client);
    http_client_close(http_client);
    *result = (char *)malloc(resp_body_len + 1);
    if (!*result)
        return -1;

    memcpy(*result, resp_body, resp_body_len);
    (*result)[resp_body_len] = '\0';
    return 0;
#undef RESP_BODY_MAX_SZ
}

static HiveDrive *ipfs_client_drive_open(HiveClient *obj, const HiveDriveOptions *options)
{
    IpfsClient *client = (IpfsClient *)obj;

    (void)options;

    pthread_mutex_lock(&client->lock);
    while (client->state == LOGGING_IN)
        pthread_cond_wait(&client->cond, &client->lock);
    if (client->state != LOGGED_IN) {
        pthread_mutex_unlock(&client->lock);
        return NULL;
    }
    pthread_mutex_unlock(&client->lock);

    return ipfs_drive_open((HiveClient *)client);
}

static void ipfs_client_close(HiveClient *obj)
{
    deref(obj);
}

static int ipfs_client_perform_tsx(HiveClient *obj, client_tsx_t *base)
{
    IpfsClient *client = (IpfsClient *)obj;
    ipfs_tsx_t *tsx = (ipfs_tsx_t *)base;
    int rc;
    http_client_t *http_client;
    char url[MAXPATHLEN + 1];

    pthread_mutex_lock(&client->lock);
    while (client->state == LOGGING_IN)
        pthread_cond_wait(&client->cond, &client->lock);
    if (client->state != LOGGED_IN) {
        pthread_mutex_unlock(&client->lock);
        return -1;
    }
    pthread_mutex_unlock(&client->lock);

    rc = snprintf(url, sizeof(url), "http://%s:%d", client->svr_addr, NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    http_client = http_client_new();
    if (!http_client)
        return -1;

    http_client_set_url(http_client, url);
    http_client_set_query(http_client, "uid", client->uid);
    tsx->setup_req(http_client, tsx->user_data);

    rc = http_client_request(http_client);
    if (rc) {
        http_client_close(http_client);
        return -1;
    }

    tsx->resp = http_client;
    return 0;
}

static int ipfs_client_invalidate_credential(HiveClient *obj)
{
    (void)obj;
    return 0;
}

static int test_reachable(const char *ip)
{
    http_client_t *http_client;
    char url[MAXPATHLEN + 1];
    int rc;
    long resp_code;

    rc = snprintf(url, sizeof(url), "http://%s:%d/version",
        ip, CLUSTER_API_PORT);
    if (rc < 0 || rc >= sizeof(url))
        return -1;

    http_client = http_client_new();
    if (!http_client)
        return -1;

    http_client_set_url(http_client, url);

    rc = http_client_request(http_client);
    if (rc) {
        http_client_close(http_client);
        return -1;
    }

    rc = http_client_get_response_code(http_client, &resp_code);
    http_client_close(http_client);
    if (rc || resp_code != 200)
        return -1;
    return 0;
}

static void hiveipfs_destructor(void *obj)
{
    IpfsClient *client = (IpfsClient *)obj;

    if (client->uid)
        free(client->uid);

    if (client->svr_addr)
        free(client->svr_addr);

    pthread_mutex_destroy(&client->lock);
    pthread_cond_destroy(&client->cond);
}

HiveClient *hiveipfs_client_new(const HiveOptions * options)
{
    HiveIpfsOptions *opt = (HiveIpfsOptions *)options;
    int i;
    int rc;
    IpfsClient *client;

    if (!opt->uid || !*opt->uid || !opt->bootstraps_size)
        return NULL;

    for (i = 0; i < opt->bootstraps_size; ++i) {
        rc = test_reachable(opt->bootstraps_ip[i]);
        if (!rc)
            break;
    }
    if (i == opt->bootstraps_size)
        return NULL;

    client = (IpfsClient *)rc_zalloc(sizeof(IpfsClient), &hiveipfs_destructor);
    if (!client)
        return NULL;

    client->uid = strdup(opt->uid);
    if (!client->uid) {
        deref(client);
        return NULL;
    }

    client->svr_addr = strdup(opt->bootstraps_ip[i]);
    if (!client->svr_addr) {
        deref(client);
        return NULL;
    }

    pthread_mutex_init(&client->lock, NULL);
    pthread_cond_init(&client->cond, NULL);

    client->base.login                 = &ipfs_client_login;
    client->base.logout                = &ipfs_client_logout;
    client->base.get_info              = &ipfs_client_get_info;
    client->base.list_drives           = &ipfs_client_list_drives;
    client->base.drive_open            = &ipfs_client_drive_open;
    client->base.destructor_func       = &ipfs_client_close;

    client->base.perform_tsx           = &ipfs_client_perform_tsx;
    client->base.invalidate_credential = &ipfs_client_invalidate_credential;

    rc = ipfs_client_get_info((HiveClient *)client, NULL);
    if (rc) {
        deref(client);
        return NULL;
    }

    return (HiveClient *)client;
}
