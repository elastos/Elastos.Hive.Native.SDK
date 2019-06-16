#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include <crystal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <arpa/inet.h>
#include <stdbool.h>
#include <cjson/cJSON.h>
#include <http_client.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#if defined(_WIN32) || defined(_WIN64)
#include <winnt.h>
#endif

#include "ela_hive.h"
#include "client.h"
#include "ipfs_client.h"
#include "ipfs_drive.h"
#include "ipfs_token.h"
#include "ipfs_common_ops.h"
#include "ipfs_constants.h"
#include "http_client.h"
#include "error.h"

typedef struct IPFSClient {
    HiveClient base;
    HiveDrive *drive;
    char token_cookie[MAXPATHLEN + 1];
    ipfs_token_t *token;
} IPFSClient;

static inline void *_test_and_swap_ptr(void **ptr, void *oldval, void *newval)
{
#if defined(_WIN32) || defined(_WIN64)
    return InterlockedCompareExchangePointer(ptr, newval, oldval);
#else
    void *tmp = oldval;
    __atomic_compare_exchange(ptr, &tmp, &newval, false,
                              __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return tmp;
#endif
}

static inline void *_exchange_ptr(void **ptr, void *val)
{
#if defined(_WIN32) || defined(_WIN64)
    return _InterlockedExchangePointer(ptr, val);
#else
    void *old;
    __atomic_exchange(ptr, &val, &old, __ATOMIC_SEQ_CST);
    return old;
#endif
}

static int ipfs_client_login(HiveClient *base,
                             HiveRequestAuthenticationCallback *cb,
                             void *user_data)
{
    IPFSClient *client = (IPFSClient *)base;
    int rc;

    assert(client);

    (void)cb;
    (void)user_data;

    rc = ipfs_synchronize(client->token);
    if (rc < 0) {
        // recover back to 'RAW' state.
        hive_set_error(-1);
        return -1;
    }

    vlogI("Hive: This client logined onto Hive IPFS in success");
    return 0;
}

static int ipfs_client_logout(HiveClient *base)
{
    IPFSClient *client = (IPFSClient *)base;
    HiveDrive *drive;
    int rc;

    drive = _exchange_ptr((void **)&client->drive, NULL);
    if (drive)
        deref(drive);

    ipfs_token_reset(client->token);
    return 0;
}

static int ipfs_client_get_info(HiveClient *base, HiveClientInfo *result)
{
    IPFSClient *client = (IPFSClient *)base;
    int rc;

    assert(client);
    assert(result);

    rc = snprintf(result->user_id, sizeof(result->user_id), "%s",
                  ipfs_token_get_uid(client->token));
    if (rc < 0 || rc >= sizeof(result->user_id))
        return -1;

    rc = snprintf(result->display_name, sizeof(result->display_name), "");
    if (rc < 0 || rc >= sizeof(result->display_name))
        return -1;

    rc = snprintf(result->email, sizeof(result->email), "");
    if (rc < 0 || rc >= sizeof(result->email))
        return -1;

    rc = snprintf(result->phone_number, sizeof(result->phone_number), "");
    if (rc < 0 || rc >= sizeof(result->phone_number))
        return -1;

    rc = snprintf(result->region, sizeof(result->region), "");
    if (rc < 0 || rc >= sizeof(result->region))
        return -1;

    return 0;
}

static int ipfs_client_list_drives(HiveClient *base, char **result)
{
    IPFSClient *client = (IPFSClient *)base;
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    char *p;
    int rc;

    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/files/stat",
                  ipfs_token_get_node_in_use(client->token), NODE_API_PORT);
    if (rc < 0 || rc >= sizeof(url)) {
        hive_set_error(-1);
        return -1;
    }

    httpc = http_client_new();
    if (!httpc) {
        hive_set_error(-1);
        return -1;
    }

    rc = ipfs_synchronize(client->token);
    if (rc)
        return -1;

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", ipfs_token_get_uid(client->token));
    http_client_set_query(httpc, "path", "/");
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0) {
        hive_set_error(-1);
        goto error_exit;
    }

    if (resp_code != 200) {
        hive_set_error(-1);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p) {
        hive_set_error(-1);
        return -1;
    }

    *result = p;
    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

static int ipfs_client_drive_open(HiveClient *base, HiveDrive **drive)
{
    IPFSClient *client = (IPFSClient *)base;
    HiveDrive *tmp;

    assert(base);

    if (client->drive)
        return -1;

    tmp = ipfs_drive_open(client->token);
    if (!tmp) {
        // TODO
        return -1;
    }

    if (_test_and_swap_ptr((void **)&client->drive, NULL, tmp)) {
        deref(tmp);
        return -1;
    }

    *drive = tmp;

    return 0;
}

static int ipfs_client_close(HiveClient *base)
{
    assert(base);

    deref(base);
    return 0;
}

static void ipfs_client_destructor(void *p)
{
    IPFSClient *client = (IPFSClient *)p;

    if (client->drive)
        deref(client->drive);

    if (client->token)
        ipfs_token_close(client->token);
}

static inline bool is_valid_ip(const char *ip)
{
    return inet_pton(AF_INET, ip, NULL) > 0 ? true : false;
}

static inline bool is_valid_ipv6(const char *ip)
{
    return inet_pton(AF_INET6, ip, NULL) > 0 ? true : false;
}

static cJSON *load_ipfs_token_cookie(const char *token_cookie)
{
    struct stat st;
    size_t n2read;
    ssize_t nread;
    cJSON *json;
    char *buf;
    char *cur;
    int rc;
    int fd;

    assert(token_cookie);

    rc = stat(token_cookie, &st);
    if (rc < 0)
        return NULL;

    if (!st.st_size)
        return NULL;

    fd = open(token_cookie, O_RDONLY);
    if (fd < 0)
        return NULL;

    buf = malloc(st.st_size);
    if (!buf) {
        close(fd);
        return NULL;
    }

    for (n2read = st.st_size, cur = buf; n2read; n2read -= nread, cur += nread) {
        nread = read(fd, cur, n2read);
        if (!nread || (nread < 0 && errno != EINTR)) {
            close(fd);
            free(buf);
            return NULL;
        }
        if (nread < 0)
            nread = 0;
    }
    close(fd);

    json = cJSON_Parse(buf);
    free(buf);

    return json;
}

static int writeback_token(const cJSON *json, void *user_data)
{
    IPFSClient *client = (IPFSClient *)user_data;
    int fd;
    size_t n2write;
    ssize_t nwrite;
    char *json_str;
    char *tmp;

    json_str = cJSON_PrintUnformatted(json);
    if (!json_str || !*json_str)
        return -1;

    fd = open(client->token_cookie, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        free(json_str);
        return -1;
    }

    for (n2write = strlen(json_str), tmp = json_str;
         n2write; n2write -= nwrite, tmp += nwrite) {
        nwrite = write(fd, tmp, n2write);
        if (!nwrite || (nwrite < 0 && errno != EINTR)) {
            free(json_str);
            close(fd);
            return -1;
        }
        if (nwrite < 0)
            nwrite = 0;
    }
    free(json_str);
    close(fd);

    return 0;
}

HiveClient *ipfs_client_new(const HiveOptions *options)
{
    IPFSOptions *opts = (IPFSOptions *)options;
    ipfs_token_options_t *token_options;
    size_t token_options_sz;
    char token_cookie[MAXPATHLEN + 1];
    cJSON *token_cookie_json = NULL;
    IPFSClient *client;
    int rc;
    size_t i;

    assert(options);

    token_options_sz = sizeof(*token_options) +
                       sizeof(rpc_node_t) * opts->rpc_node_count;
    token_options = alloca(token_options_sz);
    memset(token_options, 0, token_options_sz);

    // check uid configuration
    if (opts->uid && (!*opts->uid || strlen(opts->uid) >= sizeof(token_options->uid)))
        return NULL;

    if (opts->uid)
        strcpy(token_options->uid, opts->uid);

    // check bootstraps configuration
    token_options->rpc_nodes_count = opts->rpc_node_count;
    for (i = 0; i < opts->rpc_node_count; ++i) {
        HiveRpcNode *node = &opts->rpcNodes[i];
        rpc_node_t *node_options = &token_options->rpc_nodes[i];
        size_t ipv4_len;
        size_t ipv6_len;
        char *endptr;
        long port;

        ipv4_len = node->ipv4 ? strlen(node->ipv4) : 0;
        ipv6_len = node->ipv6 ? strlen(node->ipv6) : 0;

        if (!ipv4_len && !ipv6_len)
            return NULL;

        if (ipv4_len) {
            if (ipv4_len >= sizeof(node_options->ipv4))
                return NULL;

            if (!is_valid_ip(node->ipv4))
                return NULL;

            strcpy(node_options->ipv4, node->ipv4);
        }

        if (ipv6_len) {
            if (ipv6_len >= sizeof(node_options->ipv6))
                return NULL;

            if (!is_valid_ipv6(node->ipv6))
                return NULL;

            strcpy(node_options->ipv6, node->ipv6);
        }

        if (!node->port || !*node->port)
            return NULL;

        port = strtol(node->port, &endptr, 10);
        if (*endptr)
            return NULL;

        if (port < 1 || port > 65535)
            return NULL;

        node_options->port = (uint16_t)port;
    }

    // check token cookie
    rc = snprintf(token_cookie, sizeof(token_cookie), "%s/ipfs.json",
                  opts->base.persistent_location);
    if (rc < 0 || rc >= sizeof(token_cookie))
        return NULL;

    if (!access(token_cookie, F_OK)) {
        token_cookie_json = load_ipfs_token_cookie(token_cookie);
        if (!token_cookie_json)
            return NULL;
    }

    token_options->store = token_cookie_json;

    if (!token_options->store && (!*token_options->uid ||
                                  !token_options->rpc_nodes_count))
        return NULL;

    client = (IPFSClient *)rc_zalloc(sizeof(IPFSClient), &ipfs_client_destructor);
    if (!client) {
        if (token_options->store)
            cJSON_Delete(token_options->store);
        return NULL;
    }

    client->token = ipfs_token_new(token_options, &writeback_token, client);
    if (token_options->store)
        cJSON_Delete(token_options->store);
    if (!client->token) {
        deref(client);
        return NULL;
    }

    strcpy(client->token_cookie, token_cookie);

    client->base.login       = &ipfs_client_login;
    client->base.logout      = &ipfs_client_logout;
    client->base.get_info    = &ipfs_client_get_info;
    client->base.get_drive   = &ipfs_client_drive_open;
    client->base.close       = &ipfs_client_close;

    return &client->base;
}
