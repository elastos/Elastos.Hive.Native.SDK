#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <crystal.h>

#include "ipfs_file.h"
#include "ipfs_token.h"
#include "ipfs_utils.h"
#include "hive_client.h"
#include "http_client.h"
#include "http_status.h"

typedef struct IPFSFile {
    HiveFile base;
    ipfs_token_t *token;
    size_t lpos;
} IPFSFile;

static int get_file_stat(ipfs_token_t *token, const char *path, size_t *fsz)
{
    char buf[MAX_URL_LEN] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    cJSON *json;
    cJSON *item;
    char *p;
    int rc;

    assert(token);
    assert(path);
    assert(fsz);

    rc = ipfs_token_check_reachable(token);
    if (rc < 0) {
        vlogE("IpfsFile: failed to check node connectivity.");
        return rc;
    }

    sprintf(buf, "http://%s:%d/api/v0/files/stat",
            ipfs_token_get_current_node(token), NODE_API_PORT);

    httpc = http_client_new();
    if (!httpc) {
        vlogE("IpfsFile: failed to create http client instance.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    http_client_set_url(httpc, buf);
    http_client_set_query(httpc, "uid", ipfs_token_get_uid(token));
    http_client_set_query(httpc, "path", path);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_request_body_instant(httpc, NULL, 0);
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc) {
        rc = HIVE_HTTPC_ERROR(rc);
        if (RC_NODE_UNREACHABLE(rc)) {
            vlogE("IpfsFile: current node is not reachable.");
            rc = HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
            ipfs_token_mark_node_unreachable(token);
        } else
            vlogE("IpfsFile: failed to perform http request.");
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc) {
        vlogE("IpfsFile: failed to get http response code.");
        rc = HIVE_HTTPC_ERROR(rc);
        goto error_exit;
    }

    if (resp_code != HttpStatus_OK) {
        vlogE("IpfsFile: error from http response (%d).", resp_code);
        rc = HIVE_HTTP_STATUS_ERROR(resp_code);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p) {
        vlogE("IpfsFile: failed to get http response body.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    json = cJSON_Parse(p);
    free(p);

    if (!json) {
        vlogE("IpfsFile: bad json format for response body.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    item = cJSON_GetObjectItem(json, "Size");
    if (!cJSON_IsNumber(item)) {
        vlogE("IpfsFile: missing Size json object in response body.");
        cJSON_Delete(json);
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
    }

    *fsz = (size_t)item->valuedouble;
    cJSON_Delete(json);

    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static ssize_t ipfs_file_lseek(HiveFile *base, ssize_t offset, int whence)
{
    IPFSFile *file = (IPFSFile *)base;
    int rc;
    size_t fsz;

    switch (whence) {
    case HiveSeek_Cur:
        file->lpos = offset + file->lpos < 0 ? 0 : offset + file->lpos;
        return file->lpos;
    case HiveSeek_Set:
        file->lpos = offset > 0 ? offset : 0;
        return file->lpos;
    case HiveSeek_End:
        rc = get_file_stat(file->token, file->base.path, &fsz);
        if (rc < 0) {
            vlogE("IpfsFile: failed to get file status.");
            return rc;
        }

        file->lpos = offset + fsz < 0 ? 0 : offset + fsz;
        return 0;
    default:
        assert(0);
        vlogE("IpfsFile: unrecognizable whence parameter.");
        return HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED);
    }
}

static size_t read_response_body_cb(char *buffer,
                                    size_t size, size_t nitems, void *userdata)
{
    char *buf = (char *)(((void **)userdata)[0]);
    size_t bufsz = *((size_t *)(((void **)userdata)[1]));
    size_t *nrd = (size_t *)(((void **)userdata)[2]);
    size_t total_sz = size * nitems;

    if (*nrd + total_sz > bufsz)
        return 0;

    memcpy(buf + *nrd, buffer, total_sz);
    *nrd += total_sz;

    return total_sz;
}

static ssize_t ipfs_file_read(HiveFile *base, char *buffer, size_t bufsz)
{
    IPFSFile *file = (IPFSFile *)base;
    char buf[MAX_URL_LEN] = {0};
    char header[128];
    http_client_t *httpc;
    long resp_code = 0;
    size_t nrd = 0;
    void *user_data[] = {buffer, &bufsz, &nrd};
    int rc;

    rc = ipfs_token_check_reachable(file->token);
    if (rc < 0) {
        vlogE("IpfsFile: failed to check node connectivity.");
        return rc;
    }

    sprintf(buf, "http://%s:%d/api/v0/files/read",
            ipfs_token_get_current_node(file->token), NODE_API_PORT);

    httpc = http_client_new();
    if (!httpc) {
        vlogE("IpfsFile: failed to create http client instance.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    http_client_set_url(httpc, buf);
    http_client_set_query(httpc, "uid", ipfs_token_get_uid(file->token));
    http_client_set_query(httpc, "path", file->base.path);
    sprintf(header, "%zu", file->lpos);
    http_client_set_query(httpc, "offset", header);
    sprintf(header, "%zu", bufsz);
    http_client_set_query(httpc, "count", header);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_request_body_instant(httpc, NULL, 0);
    http_client_set_response_body(httpc, read_response_body_cb, user_data);

    rc = http_client_request(httpc);
    if (rc) {
        rc = HIVE_HTTPC_ERROR(rc);
        if (RC_NODE_UNREACHABLE(rc)) {
            vlogE("IpfsFile: current node is not reachable.");
            rc = HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
            ipfs_token_mark_node_unreachable(file->token);
        } else
            vlogE("IpfsFile: failed to perform http request.");
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    if (rc) {
        vlogE("IpfsFile: failed to get http response code.");
        return HIVE_HTTPC_ERROR(rc);
    }

    if (resp_code != HttpStatus_OK) {
        vlogE("IpfsFile: error from http response (%d).", resp_code);
        return HIVE_HTTP_STATUS_ERROR(resp_code);
    }

    file->lpos += nrd;
    return nrd;

error_exit:
    http_client_close(httpc);
    return rc;
}

static ssize_t ipfs_file_write(HiveFile *base, const char *buffer, size_t bufsz)
{
    IPFSFile *file = (IPFSFile *)base;
    char buf[MAX_URL_LEN] = {0};
    char header[128];
    http_client_t *httpc;
    long resp_code = 0;
    int rc;

    rc = ipfs_token_check_reachable(file->token);
    if (rc < 0) {
        vlogE("IpfsFile: failed to check node connectivity.");
        return rc;
    }

    sprintf(buf, "http://%s:%d/api/v0/files/write",
            ipfs_token_get_current_node(file->token), NODE_API_PORT);

    httpc = http_client_new();
    if (!httpc) {
        vlogE("IpfsFile: failed to create http client instance.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    http_client_set_url(httpc, buf);
    http_client_set_query(httpc, "uid", ipfs_token_get_uid(file->token));
    http_client_set_query(httpc, "path", file->base.path);
    sprintf(header, "%zu", file->lpos);
    http_client_set_query(httpc, "offset", header);
    sprintf(header, "%zu", bufsz);
    http_client_set_query(httpc, "count", header);
    if (HIVE_F_IS_SET(file->base.flags, HIVE_F_CREAT))
        http_client_set_query(httpc, "create", "true");
    if (HIVE_F_IS_SET(file->base.flags, HIVE_F_TRUNC))
        http_client_set_query(httpc, "truncate", "true");
    http_client_set_mime_instant(httpc, "file", NULL, NULL, buffer, bufsz);
    http_client_set_method(httpc, HTTP_METHOD_POST);

    rc = http_client_request(httpc);
    if (rc) {
        rc = HIVE_HTTPC_ERROR(rc);
        if (RC_NODE_UNREACHABLE(rc)) {
            vlogE("IpfsFile: current node is not reachable.");
            rc = HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
            ipfs_token_mark_node_unreachable(file->token);
        } else
            vlogE("IpfsFile: failed to perform http request.");
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    if (rc) {
        vlogE("IpfsFile: failed to get http response code.");
        return HIVE_HTTPC_ERROR(rc);
    }

    if (resp_code != HttpStatus_OK) {
        vlogE("IpfsFile: error from http response (%d).", resp_code);
        return HIVE_HTTP_STATUS_ERROR(resp_code);
    }

    rc = publish_root_hash(file->token, buf, sizeof(buf));
    if (rc < 0) {
        if (RC_NODE_UNREACHABLE(rc)) {
            vlogE("IpfsFile: current node is not reachable.");
            rc = HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
            ipfs_token_mark_node_unreachable(file->token);
        } else
            vlogE("IpfsFile: failed to publish root hash.");
        return rc;
    }

    HIVE_F_UNSET(file->base.flags, HIVE_F_CREAT | HIVE_F_TRUNC);

    file->lpos += bufsz;
    return bufsz;

error_exit:
    http_client_close(httpc);
    return rc;
}

static int ipfs_file_close(HiveFile *base)
{
    deref(base);
    return 0;
}

static void ipfs_file_destructor(void *obj)
{
    IPFSFile *file = (IPFSFile *)obj;

    if (file->token)
        ipfs_token_close(file->token);
}

int ipfs_file_open(ipfs_token_t *token, const char *path, int flags, HiveFile **file)
{
    IPFSFile *tmp;
    int rc;
    size_t fsz;
    bool file_exists;

    rc = get_file_stat(token, path, &fsz);
    if (rc < 0 && rc != HIVE_HTTP_STATUS_ERROR(HttpStatus_InternalServerError)) {
        vlogE("IpfsFile: failed to get file status.");
        return rc;
    }

    file_exists = !rc ? true : false;

    if (HIVE_F_IS_EQ(flags, HIVE_F_RDONLY)) {
        if (!file_exists) {
            vlogE("IpfsFile: readonly but file does not exist.");
            return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);
        }
    } else if (HIVE_F_IS_SET(flags, HIVE_F_CREAT | HIVE_F_EXCL) && file_exists) {
        vlogE("IpfsFile:  file exists while EXCL flag is set.");
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);
    } else if (!HIVE_F_IS_SET(flags, HIVE_F_CREAT) && !file_exists) {
        vlogE("IpfsFile:  CREAT flag is not set while file does not exist.");
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);
    }

    tmp = rc_zalloc(sizeof(IPFSFile), ipfs_file_destructor);
    if (!tmp)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    strcpy(tmp->base.path, path);
    tmp->base.flags   = flags;
    tmp->base.lseek   = ipfs_file_lseek;
    tmp->base.read    = ipfs_file_read;
    tmp->base.write   = ipfs_file_write;
    tmp->base.close   = ipfs_file_close;

    tmp->token        = ref(token);
    if (file_exists && HIVE_F_IS_SET(flags, HIVE_F_APPEND))
        tmp->lpos = fsz;

    *file = &tmp->base;
    return 0;
}
