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
    char uid_cookie[MAXPATHLEN];
    ipfs_token_t *token;
} IPFSClient;

static int ipfs_client_login(HiveClient *base)
{
    IPFSClient *client = (IPFSClient *)base;
    int rc;

    assert(client);

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
    int rc;

    rc = remove(client->uid_cookie);
    if (rc < 0)
        return -1;

    return 0;
}

static int ipfs_client_get_info(HiveClient *base, HiveClientInfo *result)
{
    IPFSClient *client = (IPFSClient *)base;
    char uid[HIVE_MAX_USER_ID_LEN + 1];
    int rc;

    assert(client);
    assert(result);

    ipfs_token_get_uid(client->token, uid, sizeof(uid));

    rc = snprintf(result->user_id, sizeof(result->user_id), "%s", uid);
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
    char node_ip[HIVE_MAX_IP_STRING_LEN + 1];
    char uid[HIVE_MAX_USER_ID_LEN + 1];
    char url[MAXPATHLEN + 1] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    char *p;
    int rc;

    ipfs_token_get_node_in_use(client->token, node_ip, sizeof(node_ip));
    rc = snprintf(url, sizeof(url), "http://%s:%d/api/v0/files/stat",
                  node_ip, NODE_API_PORT);
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

    ipfs_token_get_uid(client->token, uid, sizeof(uid));

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "uid", uid);
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

    tmp = ipfs_drive_open(client->token);
    if (!tmp) {
        // TODO
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

static int ipfs_client_invalidate_credential(HiveClient *obj)
{
    (void)obj;
    return 0;
}

static void ipfs_client_destructor(void *p)
{
    IPFSClient *client = (IPFSClient *)p;

    if (client->token)
        ipfs_token_close(client->token);
}

static bool is_valid_ip(const char *ip)
{
    int rc;

    rc = inet_pton(AF_INET, ip, NULL);
    if (rc < 0)
        return false;

    return true;
}

static int load_uid_cookie(const char *uid_cookie, char *uid, size_t len)
{
    struct stat st;
    size_t n2read;
    ssize_t nread;
    int rc;
    int fd;

    rc = stat(uid_cookie, &st);
    if (rc < 0)
        return -1;

    if (!st.st_size || st.st_size > len)
        return -1;

    fd = open(uid_cookie, O_RDONLY);
    if (fd < 0)
        return -1;

    for (n2read = st.st_size; n2read; n2read -= nread, uid += nread) {
        nread = read(fd, uid, n2read);
        if (!nread || (nread < 0 && errno != EINTR)) {
            close(fd);
            return -1;
        }
        if (nread < 0)
            nread = 0;
    }
    close(fd);

    if (*(uid - 1))
        return -1;

    return 0;
}

static int save_uid_cookie(const char *uid_cookie, const char *uid)
{
    int fd;
    size_t n2write;
    ssize_t nwrite;

    fd = open(uid_cookie, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0)
        return -1;

    for (n2write = strlen(uid) + 1; n2write; n2write -= nwrite, uid += nwrite) {
        nwrite = write(fd, uid, n2write);
        if (!nwrite || (nwrite < 0 && errno != EINTR)) {
            close(fd);
            return -1;
        }
        if (nwrite < 0)
            nwrite = 0;
    }
    close(fd);

    return 0;
}

int ipfs_client_new(const HiveOptions *options, HiveClient **client)
{
    IPFSOptions *opts = (IPFSOptions *)options;
    IPFSTokenOptions *token_opts = &opts->token_options;
    size_t uid_max_len = HIVE_MAX_USER_ID_LEN + 1;
    char uid_cookie[MAXPATHLEN + 1];
    bool uid_cookie_exists;
    ipfs_token_t *token;
    IPFSClient *tmp;
    int rc;
    char *uid;
    size_t i;

    assert(client);

    // check uid configuration
    if (strlen(token_opts->uid) >= sizeof(token_opts->uid))
        return -1;

    rc = snprintf(uid_cookie, sizeof(uid_cookie), "%s/uid",
                  opts->base.persistent_location);
    if (rc < 0 || rc >= sizeof(uid_cookie))
        return -1;

    uid_cookie_exists = !access(uid_cookie, F_OK) ? true : false;
    if (!token_opts->uid[0] && uid_cookie_exists) {
        uid = alloca(uid_max_len);
        rc = load_uid_cookie(uid_cookie, uid, uid_max_len);
        if (rc < 0)
            return -1;
    } else
        uid = token_opts->uid;

    // check bootstraps configuration
    if (!token_opts->bootstraps_size)
        return -1;

    for (i = 0; i < token_opts->bootstraps_size; ++i) {
        size_t ip_len = strlen(token_opts->bootstraps_ip[i]);

        if (!ip_len || ip_len >= sizeof(token_opts->bootstraps_ip[i]))
            return -1;

        if (!is_valid_ip(token_opts->bootstraps_ip[i]))
            return -1;
    }

    rc = ipfs_token_new(uid, token_opts->bootstraps_size,
                        token_opts->bootstraps_ip, &token);
    if (rc < 0)
        return -1;

    uid = alloca(uid_max_len);
    ipfs_token_get_uid(token, uid, uid_max_len);
    rc = save_uid_cookie(uid_cookie, uid);
    if (rc < 0) {
        ipfs_token_close(token);
        return -1;
    }

    tmp = (IPFSClient *)rc_zalloc(sizeof(IPFSClient), ipfs_client_destructor);
    if (!tmp)
        return -1;

    tmp->token = token;
    strcpy(tmp->uid_cookie, uid_cookie);

    tmp->base.login       = &ipfs_client_login;
    tmp->base.logout      = &ipfs_client_logout;
    tmp->base.get_info    = &ipfs_client_get_info;
    tmp->base.list_drives = &ipfs_client_list_drives;
    tmp->base.get_drive   = &ipfs_client_drive_open;
    tmp->base.finalize    = &ipfs_client_close;

    tmp->base.invalidate_credential = &ipfs_client_invalidate_credential;

    *client = (HiveClient *)tmp;
    return 0;
}
