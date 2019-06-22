#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <crystal.h>
#include <cjson/cJSON.h>

#include "ela_hive.h"
#include "hive_client.h"
#include "oauth_token.h"
#include "onedrive_constants.h"
#include "http_client.h"

typedef struct OneDriveFile {
    HiveFile base;
    bool dirty;
    int fd;
    char tmp_path[PATH_MAX];
    char ctag[0];
} OneDriveFile;

static ssize_t onedrive_file_lseek(HiveFile *base, uint64_t offset, int whence)
{
    OneDriveFile *file = (OneDriveFile *)base;

    return lseek(file->fd, offset, whence);
}

static ssize_t onedrive_file_read(HiveFile *base, char *buf, size_t bufsz)
{
    OneDriveFile *file = (OneDriveFile *)base;

    return read(file->fd, buf, bufsz);
}

static ssize_t onedrive_file_write(HiveFile *base, const char *buf, size_t bufsz)
{
    OneDriveFile *file = (OneDriveFile *)base;
    ssize_t nwr;

    nwr = write(file->fd, buf, bufsz);
    if (nwr < 0)
        return -1;

    file->dirty = true;
    return 0;
}

static int create_upload_session(OneDriveFile *file,
                                 http_client_t *httpc,
                                 char *upload_url, size_t upload_url_len)
{
    oauth_token_t *token = (oauth_token_t *)file->base.token;
    char url[MAX_URL_LEN] = {0};
    long resp_code = 0;
    int rc;
    char *p;
    cJSON *session;
    cJSON *upload_url_json;

    sprintf(url, "%s/root:%s:/createUploadSession", MY_DRIVE, file->base.path);

    http_client_set_url(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_header(httpc, "Authorization", get_bearer_token(token));
    if (file->ctag[0])
        http_client_set_header(httpc, "if-match", file->ctag);
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc < 0)
        return rc;

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0)
        return rc;

    if (resp_code == 401) {
        oauth_token_set_expired(token);
        return HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
    }

    if (resp_code != 200)
        return HIVE_HTTP_STATUS_ERROR(resp_code);

    p = http_client_move_response_body(httpc, NULL);

    if (!p)
        return rc;

    session = cJSON_Parse(p);
    free(p);

    if (!session)
        return -1;

    upload_url_json = cJSON_GetObjectItemCaseSensitive(session, "uploadUrl");
    if (!upload_url_json || !cJSON_IsString(upload_url_json) ||
        !upload_url_json->valuestring || !*upload_url_json->valuestring) {
        cJSON_Delete(session);
        return -1;
    }

    rc = snprintf(upload_url, upload_url_len, "%s", upload_url_json->valuestring);
    cJSON_Delete(session);
    if (rc < 0 || rc >= upload_url_len)
        return -1;

    return 0;
}

#define MIN(a,b) ((a) <= (b) ? (a) : (b))
static size_t upload_to_session_request_body_cb(char *buffer,
                                                size_t size, size_t nitems,
                                                void *userdata)
{
    size_t *ul_sz = (size_t *)(((void **)userdata)[0]);
    size_t *uled_sz = (size_t *)(((void **)userdata)[1]);
    int fd = *((int *)(((void **)userdata)[2]));
    size_t sz2ul;
    ssize_t nrd;

    if (*ul_sz == *uled_sz)
        return 0;

    sz2ul = MIN(*ul_sz - *uled_sz, size * nitems);
    nrd = read(fd, buffer, sz2ul);
    if (nrd < 0)
        return -1;

    *uled_sz += nrd;
    return nrd;
}

#define HTTP_PUT_MAX_CHUNK_SIZE (60U * 1024 * 1024)
static int upload_to_session(OneDriveFile *file, http_client_t *httpc,
                             const char *upload_url)
{
    long resp_code = 0;
    size_t fsize = lseek(file->fd, 0, SEEK_END);
    size_t ul_off = 0;
    size_t ul_sz = fsize % HTTP_PUT_MAX_CHUNK_SIZE;
    int rc;

    lseek(file->fd, 0, SEEK_SET);
    do {
        char header[128];
        size_t uled_sz = 0;
        void *userdata[] = {&ul_sz, &uled_sz, &file->fd};

        http_client_set_url(httpc, upload_url);
        http_client_set_method(httpc, HTTP_METHOD_PUT);
        sprintf(header, "%zu", fsize);
        http_client_set_header(httpc, "Content-Length", header);
        sprintf(header, "bytes %zu-%zu/%zu", ul_off, ul_off + ul_sz, fsize);
        http_client_set_header(httpc, "Content-Range", header);
        http_client_set_request_body(httpc,
                                     upload_to_session_request_body_cb,
                                     userdata);

        rc = http_client_request(httpc);
        if (rc < 0)
            return rc;

        ul_off += ul_sz;
        ul_sz = (fsize - ul_off) % HTTP_PUT_MAX_CHUNK_SIZE;

        rc = http_client_get_response_code(httpc, &resp_code);
        if (rc < 0)
            return rc;

        if ((ul_sz && resp_code != 202) || (!ul_sz && resp_code != 201))
            return HIVE_HTTP_STATUS_ERROR(resp_code);
    } while (ul_sz);

    return 0;
}

static int upload_file(OneDriveFile *file)
{
    oauth_token_t *token = (oauth_token_t *)file->base.token;
    http_client_t *httpc;
    char upload_url[MAX_URL_LEN] = {0};
    int rc;

    assert(token);

    rc = oauth_token_check_expire(token);
    if (rc < 0) {
        vlogE("Hive: Checking access token expired error.");
        return rc;
    }

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    rc = create_upload_session(file, httpc, upload_url, sizeof(upload_url));
    if (rc < 0) {
        http_client_close(httpc);
        return rc;
    }

    http_client_reset(httpc);

    rc = upload_to_session(file, httpc, upload_url);
    http_client_close(httpc);
    if (rc < 0)
        return rc;

    return 0;
}

static int onedrive_file_close(HiveFile *base)
{
    OneDriveFile *file = (OneDriveFile *)base;
    int rc = 0;

    if (file->dirty)
        rc = upload_file(file);

    if (!rc)
        unlink(file->tmp_path);

    deref(file);

    return rc;
}

static void onedrive_file_destructor(void *obj)
{
    OneDriveFile *file = (OneDriveFile *)obj;

    if (file->fd >= 0)
        close(file->fd);

    if (file->base.token)
        oauth_token_delete((oauth_token_t *)file->base.token);
}

static int get_file_stat(oauth_token_t *token, const char *path, cJSON **fstat)
{
    http_client_t *httpc;
    char url[MAX_URL_LEN] = {0};
    char *p;
    long resp_code = 0;
    int rc;

    assert(token);
    assert(path);
    assert(*path == '/');

    rc = oauth_token_check_expire(token);
    if (rc < 0) {
        vlogE("Hive: Checking access token expired error.");
        return rc;
    }

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    sprintf(url, "%s/root:%s", MY_DRIVE, path);

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "select", "cTag,file,@microsoft.graph.downloadUrl");
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_set_header(httpc, "Authorization", get_bearer_token(token));
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc < 0) {
        // TODO: rc;
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc < 0) {
        // TODO: rc;
        goto error_exit;
    }

    if (resp_code == 401) {
        oauth_token_set_expired(token);
        rc = HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
        goto error_exit;
    }

    if (resp_code != 200) {
        rc = HIVE_HTTP_STATUS_ERROR(resp_code);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p) {
        // TODO: rc;
        return rc;
    }

    *fstat = cJSON_Parse(p);
    free(p);

    if (!*fstat)
        return -1;

    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static size_t response_body_callback(char *buffer, size_t size,
                                     size_t nitems, void *userdata)
{
    int fd = *(size_t *)userdata;
    size_t total_sz = size * nitems;
    ssize_t nwr;

    nwr = write(fd, buffer,total_sz);
    if (nwr < 0)
        return -1;

    return nwr;
}

static int download_file(int fd, const char *download_url)
{
    http_client_t *httpc;
    long resp_code;
    int rc;

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    http_client_set_url(httpc, download_url);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_enable_response_body(httpc);
    http_client_set_response_body(httpc, response_body_callback, &fd);

    rc = http_client_request(httpc);
    if (rc < 0) {
        // TODO:
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    if (rc < 0)
        return rc;

    if (resp_code != 200)
        return -1;

    return 0;

error_exit:
    http_client_close(httpc);
    return -1;
}

int onedrive_file_open(oauth_token_t *token, const char *path,
                       int flags, const char *tmp_template, HiveFile **file)
{
    OneDriveFile *tmp;
    bool file_exists;
    cJSON *fstat = NULL;
    char *download_url;
    char *ctag;
    size_t ctag_len = 0;
    int rc;

    rc = get_file_stat(token, path, &fstat);
    if (rc < 0 && rc != HIVE_HTTP_STATUS_ERROR(404))
        return -1;

    file_exists = !rc ? true : false;

    if (file_exists) {
        cJSON *download_url_json;
        cJSON *ctag_json;

        if (!cJSON_GetObjectItemCaseSensitive(fstat, "file")) {
            cJSON_Delete(fstat);
            return -1;
        }

        download_url_json =
        cJSON_GetObjectItemCaseSensitive(fstat, "@microsoft.graph.downloadUrl");
        if (!download_url_json || !download_url_json->string ||
            !*download_url_json->valuestring) {
            cJSON_Delete(fstat);
            return -1;
        }
        download_url = alloca(strlen(download_url_json->valuestring) + 1);
        strcpy(download_url, download_url_json->valuestring);

        ctag_json = cJSON_GetObjectItemCaseSensitive(fstat, "cTag");
        if (!ctag_json || !ctag_json->string || !*ctag_json->valuestring) {
            cJSON_Delete(fstat);
            return -1;
        }
        ctag = alloca(strlen(ctag_json->valuestring) + 1);
        strcpy(ctag, ctag_json->valuestring);

        cJSON_Delete(fstat);

        ctag_len = strlen(ctag);
    }

    if (HIVE_F_IS_EQ(flags, HIVE_F_RDONLY)) {
        if (!file_exists)
            return -1;
    } else if (HIVE_F_IS_SET(flags, HIVE_F_CREAT | HIVE_F_EXCL) && file_exists)
        return -1;
    else if (!HIVE_F_IS_SET(flags, HIVE_F_CREAT) && !file_exists)
        return -1;

    tmp = rc_zalloc(sizeof(OneDriveFile) + ctag_len + 1, onedrive_file_destructor);
    if (!tmp)
        return -1;

    tmp->base.token = (token_base_t *)token;
    strcpy(tmp->base.path, path);
    tmp->base.flags = flags;

    tmp->base.lseek = onedrive_file_lseek;
    tmp->base.read  = onedrive_file_read;
    tmp->base.write = onedrive_file_write;
    tmp->base.close = onedrive_file_close;

    if (file_exists)
        strcpy(tmp->ctag, ctag);

    strcpy(tmp->tmp_path, tmp_template);
    tmp->fd = mkstemp(tmp->tmp_path);
    if (tmp->fd < 0) {
        deref(tmp);
        return -1;
    }

    if (file_exists && !HIVE_F_IS_SET(flags, HIVE_F_TRUNC)) {
        rc = download_file(tmp->fd, download_url);
        if (rc < 0) {
            unlink(tmp->tmp_path);
            deref(tmp);
            return -1;
        }
    } else
        tmp->dirty = true;

    if (!HIVE_F_IS_SET(flags, HIVE_F_APPEND))
        lseek(tmp->fd, 0, SEEK_SET);

    *file = &tmp->base;
    return 0;
}
