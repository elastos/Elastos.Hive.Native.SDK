/*
 * Copyright (c) 2019 Elastos Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <crystal.h>
#define ftruncate _chsize
#endif

#include <crystal.h>
#include <cjson/cJSON.h>

#include "ela_hive.h"
#include "hive_client.h"
#include "oauth_token.h"
#include "onedrive_constants.h"
#include "http_client.h"
#include "http_status.h"

typedef struct OneDriveFile {
    HiveFile base;
    oauth_token_t *token;
    bool dirty;
    int fd;
    char tmp_path[PATH_MAX];
    // upstream properties
    char ctag[MAX_CTAG_LEN];
    char dl_url[MAX_URL_LEN];
} OneDriveFile;

static ssize_t onedrive_file_lseek(HiveFile *base, ssize_t offset, Whence whence)
{
    OneDriveFile *file = (OneDriveFile *)base;
    int whence_sys;
    ssize_t rc;

    switch (whence) {
    case HiveSeek_Cur:
        whence_sys = SEEK_CUR;
        break;
    case HiveSeek_Set:
        whence_sys = SEEK_SET;
        break;
    case HiveSeek_End:
        whence_sys = SEEK_END;
        break;
    default:
        assert(0);
        vlogE("OneDriveFile: unsupported whence parameter.");
        return HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED);
    }

    rc = lseek(file->fd, (long)offset, whence_sys);
    if (rc < 0) {
        vlogE("OneDriveFile: failed to call lseek() (%d).", errno);
        return HIVE_SYS_ERROR(errno);
    }

    return rc;
}

static ssize_t onedrive_file_read(HiveFile *base, char *buf, size_t bufsz)
{
    OneDriveFile *file = (OneDriveFile *)base;
    ssize_t rc;

    rc = read(file->fd, buf, (unsigned)bufsz);
    if (rc < 0) {
        vlogE("OneDriveFile: failed to call read().");
        return HIVE_SYS_ERROR(errno);
    }

    return rc;
}

static ssize_t onedrive_file_write(HiveFile *base, const char *buf, size_t bufsz)
{
    OneDriveFile *file = (OneDriveFile *)base;
    ssize_t nwr;

    nwr = write(file->fd, buf, (unsigned)bufsz);
    if (nwr < 0) {
        vlogE("OneDriveFile: failed to read: failed to call write().");
        return HIVE_SYS_ERROR(errno);
    }

    file->dirty = true;
    return nwr;
}

static int create_upload_session(OneDriveFile *file,
                                 http_client_t *httpc,
                                 char *upload_url, size_t upload_url_len)
{
    char url[MAX_URL_LEN] = {0};
    long resp_code = 0;
    int rc;
    char *p;
    cJSON *session;
    cJSON *upload_url_json;

    sprintf(url, "%s/root:%s:/createUploadSession", MY_DRIVE, file->base.path);

    http_client_set_url(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_header(httpc, "Authorization", get_bearer_token(file->token));
    http_client_set_request_body_instant(httpc, NULL, 0);
    if (file->ctag[0])
        http_client_set_header(httpc, "if-match", file->ctag);
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc) {
        vlogE("OneDriveFile: failed to create http client instance.");
        return HIVE_CURL_ERROR(rc);
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc) {
        vlogE("OneDriveFile: failed to get http response code.");
        return HIVE_CURL_ERROR(rc);
    }

    if (resp_code == HttpStatus_Unauthorized) {
        vlogE("OneDriveFile: access token expired.");
        oauth_token_set_expired(file->token);
        return HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
    }

    if (resp_code != HttpStatus_OK) {
        vlogE("OneDriveFile: error from http response (%d).", resp_code);
        return HIVE_HTTP_STATUS_ERROR(resp_code);
    }

    p = http_client_move_response_body(httpc, NULL);
    if (!p) {
        vlogE("OneDriveFile: failed to get http response body.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    session = cJSON_Parse(p);
    free(p);

    if (!session) {
        vlogE("OneDriveFile: failed to parse json object from response.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    upload_url_json = cJSON_GetObjectItemCaseSensitive(session, "uploadUrl");
    if (!upload_url_json || !cJSON_IsString(upload_url_json) ||
        !upload_url_json->valuestring || !*upload_url_json->valuestring) {
        vlogE("OneDriveFile: failed to create json object.");
        cJSON_Delete(session);
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
    }

    rc = snprintf(upload_url, upload_url_len, "%s", upload_url_json->valuestring);
    cJSON_Delete(session);
    if (rc < 0 || rc >= upload_url_len) {
        vlogE("OneDriveFile: upload URL too long.");
        return HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);
    }

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
    nrd = read(fd, buffer, (unsigned)sz2ul);
    if (nrd < 0) {
        vlogE("OneDriveFile: failed to call read().");
        return HTTP_CLIENT_REQBODY_ABORT;
    }

    *uled_sz += nrd;
    vlogI("OneDriveFile: Successfully read %d bytes from temporary file to be uploaded.", nrd);
    return (size_t)nrd;
}

#define HTTP_PUT_MAX_CHUNK_SIZE (60U * 1024 * 1024)
static int upload_to_session(OneDriveFile *file, http_client_t *httpc,
                             const char *upload_url)
{
    long resp_code = 0;
    size_t fsize = lseek(file->fd, 0, SEEK_END);
    size_t ul_off;
    size_t ul_sz;
    int rc;

    lseek(file->fd, 0, SEEK_SET);
    ul_off = 0;
    ul_sz = fsize % HTTP_PUT_MAX_CHUNK_SIZE;
    while (ul_sz) {
        char header[128];
        size_t uled_sz = 0;
        void *userdata[] = {&ul_sz, &uled_sz, &file->fd};

        http_client_set_url(httpc, upload_url);
        http_client_set_method(httpc, HTTP_METHOD_PUT);
        sprintf(header, "%zu", ul_sz);
        http_client_set_header(httpc, "Content-Length", header);
        sprintf(header, "bytes %zu-%zu/%zu", ul_off, ul_off + ul_sz - 1, fsize);
        http_client_set_header(httpc, "Content-Range", header);
        http_client_set_header(httpc, "Transfer-Encoding", "");
        http_client_set_header(httpc, "Expect", "");
        http_client_set_request_body(httpc,
                                     upload_to_session_request_body_cb,
                                     userdata);

        rc = http_client_request(httpc);
        if (rc) {
            vlogE("OneDriveFile: failed to perform http request.");
            return HIVE_CURL_ERROR(rc);
        }

        ul_off += ul_sz;
        ul_sz = (fsize - ul_off) % HTTP_PUT_MAX_CHUNK_SIZE;

        rc = http_client_get_response_code(httpc, &resp_code);
        if (rc) {
            vlogE("OneDriveFile: failed to get http response code.");
            return HIVE_CURL_ERROR(rc);
        }

        if ((ul_sz && resp_code != HttpStatus_Accepted) ||
            (!ul_sz && ((!file->ctag[0] && resp_code != HttpStatus_Created) ||
                        (file->ctag[0] && resp_code != HttpStatus_OK)))) {
            vlogE("OneDriveFile: error from http response (%d).", resp_code);
            return HIVE_HTTP_STATUS_ERROR(resp_code);
        }
    }

    return 0;
}

static int upload_file(OneDriveFile *file)
{
    http_client_t *httpc;
    char upload_url[MAX_URL_LEN] = {0};
    int rc;

    rc = oauth_token_check_expire(file->token);
    if (rc < 0) {
        vlogE("OneDriveFile: checking access token expired error.");
        return rc;
    }

    httpc = http_client_new();
    if (!httpc) {
        vlogE("OneDriveFile: failed to create http client instance.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    rc = create_upload_session(file, httpc, upload_url, sizeof(upload_url));
    if (rc < 0) {
        vlogE("OneDriveFile: failed to create upload session.");
        http_client_close(httpc);
        return rc;
    }

    vlogI("OneDriveFile: Susscessfully created an upload session.");

    http_client_reset(httpc);

    rc = upload_to_session(file, httpc, upload_url);
    http_client_close(httpc);
    if (rc < 0) {
        vlogE("OneDriveFile: failed to upload to session.");
        return rc;
    }

    vlogI("OneDriveFile: Susscessfully uploaded temporary file to onedrive.");

    return 0;
}

static int onedrive_file_close(HiveFile *base)
{
    OneDriveFile *file = (OneDriveFile *)base;

    deref(file);

    return 0;
}

static void onedrive_file_destructor(void *obj)
{
    OneDriveFile *file = (OneDriveFile *)obj;

    if (file->fd >= 0)
        close(file->fd);

    if (!file->dirty)
        unlink(file->tmp_path);

    if (file->token)
        oauth_token_delete(file->token);
}

static int get_file_stat(oauth_token_t *token, const char *path,
                         char *ctag, size_t ctag_len,
                         char *dl_url, size_t dl_url_len)
{
    http_client_t *httpc;
    char url[MAX_URL_LEN] = {0};
    char *p;
    long resp_code = 0;
    int rc;
    cJSON *fstat;
    cJSON *download_url_json;
    cJSON *ctag_json;

    assert(token);
    assert(path);
    assert(*path == '/');

    rc = oauth_token_check_expire(token);
    if (rc < 0) {
        vlogE("OneDriveFile: checking access token expired error.");
        return rc;
    }

    httpc = http_client_new();
    if (!httpc) {
        vlogE("OneDriveFile: failed to create http client instance.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    sprintf(url, "%s/root:%s", MY_DRIVE, path);

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "select", "cTag,file,@microsoft.graph.downloadUrl");
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_set_header(httpc, "Authorization", get_bearer_token(token));
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc) {
        rc = HIVE_CURL_ERROR(rc);
        vlogE("OneDriveFile: failed to perform http request.");
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc) {
        rc = HIVE_CURL_ERROR(rc);
        vlogE("OneDriveFile: failed to get http response code.");
        goto error_exit;
    }

    if (resp_code == HttpStatus_Unauthorized) {
        vlogE("OneDriveFile: access token expired.");
        oauth_token_set_expired(token);
        rc = HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
        goto error_exit;
    }

    if (resp_code != HttpStatus_OK) {
        vlogE("OneDriveFile: error from http response (%d).", resp_code);
        rc = HIVE_HTTP_STATUS_ERROR(resp_code);
        goto error_exit;
    }

    p = http_client_move_response_body(httpc, NULL);
    http_client_close(httpc);

    if (!p) {
        vlogE("OneDriveFile: failed to get http response body.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    fstat = cJSON_Parse(p);
    free(p);

    if (!fstat) {
        vlogE("OneDriveFile: bad json format for response.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    if (!cJSON_GetObjectItemCaseSensitive(fstat, "file")) {
        vlogE("OneDriveFile: missing file json object for response.");
        cJSON_Delete(fstat);
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
    }

    download_url_json =
    cJSON_GetObjectItemCaseSensitive(fstat, "@microsoft.graph.downloadUrl");
    if (!download_url_json || !download_url_json->string ||
        !*download_url_json->valuestring ||
        strlen(download_url_json->valuestring) >= dl_url_len) {
        vlogE("OneDriveFile: missing @microsoft.graph.downloadUrl json object for response.");
        cJSON_Delete(fstat);
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
    }
    strcpy(dl_url, download_url_json->valuestring);

    ctag_json = cJSON_GetObjectItemCaseSensitive(fstat, "cTag");
    if (!ctag_json || !ctag_json->string || !*ctag_json->valuestring ||
        strlen(ctag_json->valuestring) >= ctag_len) {
        vlogE("OneDriveFile: missing cTag json object for response.");
        cJSON_Delete(fstat);
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
    }
    strcpy(ctag, ctag_json->valuestring);

    cJSON_Delete(fstat);

    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static size_t response_body_callback(char *buffer, size_t size,
                                     size_t nitems, void *userdata)
{
    int fd = *(int *)userdata;
    size_t total_sz = size * nitems;
    ssize_t nwr;

    nwr = write(fd, buffer, (unsigned)total_sz);
    if (nwr < 0) {
        vlogE("OneDriveFile: calling write() failure.");
        return 0;
    }

    vlogI("OneDriveFile: Successfully get %d bytes from onedrive.", nwr);

    return (size_t)nwr;
}

static int download_file(int fd, const char *download_url)
{
    http_client_t *httpc;
    long resp_code;
    int rc;

    httpc = http_client_new();
    if (!httpc) {
        vlogE("OneDriveFile: failed to create http client.");
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
    }

    http_client_set_url(httpc, download_url);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_enable_response_body(httpc);
    http_client_set_response_body(httpc, response_body_callback, &fd);

    rc = http_client_request(httpc);
    if (rc) {
        rc = HIVE_CURL_ERROR(rc);
        vlogE("OneDriveFile: failed to perform http request.");
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    if (rc) {
        vlogE("OneDriveFile: failed to get http response code.");
        return HIVE_CURL_ERROR(rc);
    }

    if (resp_code != HttpStatus_OK) {
        vlogE("OneDriveFile: error from http response (%d).", resp_code);
        return HIVE_HTTP_STATUS_ERROR(resp_code);
    }

    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static int onedrive_file_commit(HiveFile *base)
{
    OneDriveFile *file = (OneDriveFile *)base;
    int rc = 0;

    if (!file->dirty)
        return 0;

    rc = upload_file(file);
    if (rc < 0) {
        vlogE("OneDriveFile: failed to commit temporary file.");
        return rc;
    }

    file->dirty = false;

    rc = get_file_stat(file->token, base->path,
                       file->ctag, sizeof(file->ctag),
                       file->dl_url, sizeof(file->dl_url));
    if (rc < 0) {
        vlogE("OneDriveFile: failed to get file status.");
        return rc;
    }

    return 0;
}

static int onedrive_file_discard(HiveFile *base)
{
    OneDriveFile *file = (OneDriveFile *)base;
    int rc;

    if (!file->dirty)
        return 0;

    ftruncate(file->fd, 0);
    lseek(file->fd, 0, SEEK_SET);

    if (!file->ctag[0])
        return 0;

    rc = download_file(file->fd, file->dl_url);
    file->dirty = false;
    if (rc < 0)
        return rc;
    lseek(file->fd, 0, SEEK_SET);

    return 0;
}

#if defined(_WIN32) || defined(_WIN64)
static int mkstemp(char *template)
{
    errno_t err;

    err = _mktemp_s(template, strlen(template) + 1);
    if (err) {
        errno = err;
        vlogE("OneDriveFile: failed to call _mktemp_s() (%d).", errno);
        return -1;
    }

    return open(template, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
}
#endif

int onedrive_file_open(oauth_token_t *token, const char *path,
                       int flags, const char *tmp_template, HiveFile **file)
{
    OneDriveFile *tmp;
    bool file_exists;
    char download_url[MAX_URL_LEN];
    char ctag[MAX_CTAG_LEN];
    int rc;

    rc = get_file_stat(token, path, ctag, sizeof(ctag),
                       download_url, sizeof(download_url));
    if (rc < 0 && rc != HIVE_HTTP_STATUS_ERROR(HttpStatus_NotFound)) {
        vlogE("OneDriveFile: get file status failure.");
        return rc;
    }

    file_exists = !rc ? true : false;

    if (HIVE_F_IS_EQ(flags, HIVE_F_RDONLY)) {
        if (!file_exists) {
            vlogE("OneDriveFile: set readonly flag while file does not exist.");
            return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);
        }
    } else if (HIVE_F_IS_SET(flags, HIVE_F_CREAT | HIVE_F_EXCL) && file_exists) {
        vlogE("OneDriveFile: set EXCL flag while file exists.");
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);
    } else if (!HIVE_F_IS_SET(flags, HIVE_F_CREAT) && !file_exists) {
        vlogE("OneDriveFile: set no CREAT flag while file does not exist.");
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);
    }

    tmp = rc_zalloc(sizeof(OneDriveFile), onedrive_file_destructor);
    if (!tmp)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    strcpy(tmp->base.path, path);
    tmp->base.flags   = flags;
    tmp->base.lseek   = onedrive_file_lseek;
    tmp->base.read    = onedrive_file_read;
    tmp->base.write   = onedrive_file_write;
    tmp->base.commit  = onedrive_file_commit;
    tmp->base.discard = onedrive_file_discard;
    tmp->base.close   = onedrive_file_close;

    tmp->token        = ref(token);
    if (file_exists) {
        strcpy(tmp->ctag, ctag);
        strcpy(tmp->dl_url, download_url);
    }

    strcpy(tmp->tmp_path, tmp_template);
    tmp->fd = mkstemp(tmp->tmp_path);
    if (tmp->fd < 0) {
        vlogE("OneDriveFile: failed to call mkstemp().");
        deref(tmp);
        return HIVE_SYS_ERROR(errno);
    }

    if (file_exists && !HIVE_F_IS_SET(flags, HIVE_F_TRUNC)) {
        rc = download_file(tmp->fd, download_url);
        if (rc < 0) {
            vlogE("OneDriveFile: failed to download from onedrive.");
            unlink(tmp->tmp_path);
            deref(tmp);
            return rc;
        }
    } else
        tmp->dirty = true;

    if (!HIVE_F_IS_SET(flags, HIVE_F_APPEND))
        lseek(tmp->fd, 0, SEEK_SET);

    *file = &tmp->base;
    return 0;
}
