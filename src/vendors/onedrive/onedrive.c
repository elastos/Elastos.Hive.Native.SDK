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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#endif

#include <string.h>
#include <crystal.h>
#include <cjson/cJSON.h>

#include "hive_error.h"
#include "onedrive_constants.h"
#include "http_client.h"
#include "http_status.h"
#include "mkdirs.h"
#include "oauth_token.h"
#include "hive_client.h"

typedef struct OneDriveConnect {
    HiveConnect base;
    oauth_token_t *token;
    char keystore_path[PATH_MAX];
} OneDriveConnect;

static int disconnect(HiveConnect *base)
{
    assert(base);

    deref(base);
    return 0;
}

static cJSON *load_keystore_in_json(const char *path)
{
    struct stat st;
    char buf[4096] = {0};
    cJSON *json;
    int rc;
    int fd;

    assert(path);

    rc = stat(path, &st);
    if (rc < 0)
        return NULL;

    if (!st.st_size || st.st_size > sizeof(buf)) {
        errno = ERANGE;
        return NULL;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return NULL;

    rc = (int)read(fd, buf, st.st_size);
    close(fd);

    if (rc < 0 || rc != st.st_size)
        return NULL;

    json = cJSON_Parse(buf);
    if (!json) {
        errno = ENOMEM;
        return NULL;
    }

    return json;
}

static int oauth_writeback(const cJSON *json, void *user_data)
{
    OneDriveConnect *connect = (OneDriveConnect *)user_data;
    char *json_str;
    int json_str_len;
    int fd;
    int bytes;

    json_str = cJSON_PrintUnformatted(json);
    if (!json_str || !*json_str)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    fd = open(connect->keystore_path, O_WRONLY | O_TRUNC | O_CREAT,
              S_IRUSR | S_IWUSR);
    if (fd < 0) {
        free(json_str);
        return HIVE_SYS_ERROR(errno);
    }

    json_str_len = (int)strlen(json_str);
    bytes = (int)write(fd, json_str, json_str_len + 1);
    free(json_str);
    close(fd);

    if (bytes != json_str_len + 1)
        return HIVE_SYS_ERROR(errno);

    return 0;
}

static void onedrive_connect_destructor(void *obj)
{
    OneDriveConnect *connect = (OneDriveConnect *)obj;

    if (connect->token)
        oauth_token_delete(connect->token);
}

static int expire_token(HiveConnect *base)
{
    OneDriveConnect *connect = (OneDriveConnect *)base;

    oauth_token_set_expired(connect->token);
    return 0;
}

static int __upload_empty_file(OneDriveConnect *connect, http_client_t *httpc,
                               const char *file_path)
{
    char url[MAX_URL_LEN] = {0};
    long resp_code = 0;
    int rc;

    sprintf(url, "%s:%s:/content", APP_ROOT, file_path);

    http_client_set_url(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_PUT);
    http_client_set_header(httpc, "Authorization", get_bearer_token(connect->token));
    http_client_set_request_body_instant(httpc, NULL, 0);

    rc = http_client_request(httpc);
    if (rc)
        return HIVE_CURL_ERROR(rc);

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc)
        return HIVE_CURL_ERROR(rc);

    if (resp_code == HttpStatus_Unauthorized) {
        oauth_token_set_expired(connect->token);
        return HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
    }

    if (resp_code != HttpStatus_Created && resp_code != HttpStatus_OK)
        return HIVE_HTTP_STATUS_ERROR(resp_code);

    return 0;
}

static int __create_upload_session(OneDriveConnect *connect,
                                   http_client_t *httpc,
                                   const char *file_path,
                                   void *upload_url, size_t len)
{
    char url[MAX_URL_LEN] = {0};
    cJSON *upload_url_json;
    long resp_code = 0;
    cJSON *resp;
    char *p;
    int rc;

    sprintf(url, "%s:%s:/createUploadSession", APP_ROOT, file_path);

    http_client_set_url(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_header(httpc, "Authorization", get_bearer_token(connect->token));
    http_client_set_request_body_instant(httpc, NULL, 0);
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc)
        return HIVE_CURL_ERROR(rc);

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc)
        return HIVE_CURL_ERROR(rc);

    if (resp_code == HttpStatus_Unauthorized) {
        oauth_token_set_expired(connect->token);
        return HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
    }

    if (resp_code != HttpStatus_OK)
        return HIVE_HTTP_STATUS_ERROR(resp_code);

    p = http_client_move_response_body(httpc, NULL);
    if (!p)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    resp = cJSON_Parse(p);
    free(p);

    if (!resp)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    upload_url_json= cJSON_GetObjectItemCaseSensitive(resp, "uploadUrl");
    if (!upload_url_json || !cJSON_IsString(upload_url_json) ||
        !upload_url_json->valuestring ||
        !*upload_url_json->valuestring) {
        cJSON_Delete(resp);
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
    }

    rc = snprintf(upload_url, len, "%s", upload_url_json->valuestring);
    cJSON_Delete(resp);

    if (rc < 0 || rc >= (int)len)
        return HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);

    return 0;
}

static int __upload_to_session(OneDriveConnect *connect, http_client_t *httpc,
                               const char *upload_url, const void *buf, size_t len)
{
    long resp_code = 0;
    char header[128];
    int rc;

    http_client_set_url(httpc, upload_url);
    http_client_set_method(httpc, HTTP_METHOD_PUT);
    sprintf(header, "%zu", len);
    http_client_set_header(httpc, "Content-Length", header);
    sprintf(header, "bytes 0-%zu/%zu", len - 1, len);
    http_client_set_header(httpc, "Content-Range", header);
    http_client_set_header(httpc, "Transfer-Encoding", "");
    http_client_set_header(httpc, "Expect", "");
    http_client_set_request_body_instant(httpc, buf, len);

    rc = http_client_request(httpc);
    if (rc)
        return HIVE_CURL_ERROR(rc);

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc)
        return HIVE_CURL_ERROR(rc);

    if (resp_code != HttpStatus_Created && resp_code != HttpStatus_OK)
        return HIVE_HTTP_STATUS_ERROR(resp_code);

    return 0;
}

static int __put_file_from_buffer(OneDriveConnect *connect,
                                  const void *from, size_t length,
                                  bool encrypt,
                                  const char *path)
{
    char url[MAX_URL_LEN] = {0};
    http_client_t *httpc;
    int rc;

    rc = oauth_token_check_expire(connect->token);
    if (rc < 0)
        return rc;

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    if (!length) {
        rc = __upload_empty_file(connect, httpc, path);
        http_client_close(httpc);
        return rc;
    }

    rc = __create_upload_session(connect, httpc, path, url, sizeof(url));
    if (rc < 0) {
        http_client_close(httpc);
        return rc;
    }

    http_client_reset(httpc);

    rc = __upload_to_session(connect, httpc, url, from, length);
    http_client_close(httpc);
    if (rc < 0)
        return rc;

    return 0;
}

static int put_file_from_buffer(HiveConnect *base, const void *from,
                                size_t length, bool encrypt, const char *filename)
{
    OneDriveConnect *connect = (OneDriveConnect *)base;
    char path[PATH_MAX] = {0};

    snprintf(path, sizeof(path), "%s/%s", FILES_DIR, filename);

    return __put_file_from_buffer(connect, from, length, encrypt, path);
}

static int __get_file_info(OneDriveConnect *connect, http_client_t *httpc, const char *file_path,
                           const char *query, cJSON **response)
{
    char url[MAX_URL_LEN] = {0};
    long resp_code = 0;
    cJSON *resp;
    char *p;
    int rc;

    sprintf(url, "%s:%s", APP_ROOT, file_path);

    http_client_set_url(httpc, url);
    http_client_set_query(httpc, "select", query);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_set_header(httpc, "Authorization", get_bearer_token(connect->token));
    http_client_enable_response_body(httpc);

    rc = http_client_request(httpc);
    if (rc)
        return HIVE_CURL_ERROR(rc);

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc)
        return HIVE_CURL_ERROR(rc);

    if (resp_code == HttpStatus_Unauthorized) {
        oauth_token_set_expired(connect->token);
        return HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
    }

    if (resp_code != HttpStatus_OK)
        return HIVE_HTTP_STATUS_ERROR(resp_code);

    p = http_client_move_response_body(httpc, NULL);
    if (!p)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    resp = cJSON_Parse(p);
    free(p);

    if (!resp)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    if (strstr(query, "size")) {
        cJSON *size;

        size = cJSON_GetObjectItemCaseSensitive(resp, "size");
        if (!size || !cJSON_IsNumber(size) || (ssize_t)size->valuedouble < 0) {
            cJSON_Delete(resp);
            return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
        }
    }

    if (strstr(query, "@microsoft.graph.downloadUrl")) {
        cJSON *download_url;

        download_url = cJSON_GetObjectItemCaseSensitive(resp, "@microsoft.graph.downloadUrl");
        if (!download_url || !cJSON_IsString(download_url) ||
            !download_url->string || !*download_url->valuestring) {
            cJSON_Delete(resp);
            return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
        }
    }

    *response = resp;

    return 0;
}

static ssize_t __get_file_length(OneDriveConnect *connect, const char *file_path)
{
    http_client_t *httpc;
    ssize_t fsize;
    cJSON *resp;
    cJSON *size;
    int rc;

    rc = oauth_token_check_expire(connect->token);
    if (rc < 0)
        return rc;

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    rc = __get_file_info(connect, httpc, file_path, "size", &resp);
    http_client_close(httpc);
    if (rc < 0)
        return rc;

    size = cJSON_GetObjectItemCaseSensitive(resp, "size");
    fsize = (ssize_t)size->valuedouble;
    cJSON_Delete(resp);

    return fsize;
}

static ssize_t get_file_length(HiveConnect *base, const char *filename)
{
    OneDriveConnect *connect = (OneDriveConnect *)base;
    char file_path[PATH_MAX];
    int rc;

    rc = snprintf(file_path, sizeof(file_path), "%s/%s", FILES_DIR, filename);
    if (rc < 0 || rc >= sizeof(file_path))
        return HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);

    return __get_file_length(connect, file_path);
}

static size_t __download_file_response_body_callback(char *buffer, size_t size,
                                                     size_t nitems, void *userdata)
{
    size_t total_sz = size * nitems;
    uint8_t **buf = (uint8_t **)userdata;

    memcpy(*buf, buffer, total_sz);
    *buf += total_sz;

    return total_sz;
}

static int __download_file(OneDriveConnect *connect, http_client_t *httpc, const char *download_url, void *buf)
{
    long resp_code;
    int rc;

    http_client_set_url(httpc, download_url);
    http_client_set_method(httpc, HTTP_METHOD_GET);
    http_client_enable_response_body(httpc);
    http_client_set_response_body(httpc, __download_file_response_body_callback, &buf);

    rc = http_client_request(httpc);
    if (rc)
        return HIVE_CURL_ERROR(rc);

    rc = http_client_get_response_code(httpc, &resp_code);
    if (rc)
        return HIVE_CURL_ERROR(rc);

    if (resp_code != HttpStatus_OK)
        return HIVE_HTTP_STATUS_ERROR(resp_code);

    return 0;
}

static ssize_t __get_file_to_buffer(OneDriveConnect *connect, const char *file_path,
                                    bool decrypt, void *to, size_t buflen)
{
    http_client_t *httpc;
    cJSON *download_url;
    ssize_t fsize;
    cJSON *resp;
    cJSON *size;
    int rc;

    rc = oauth_token_check_expire(connect->token);
    if (rc < 0)
        return rc;

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    rc = __get_file_info(connect, httpc, file_path,
                         "size,@microsoft.graph.downloadUrl", &resp);
    if (rc < 0) {
        http_client_close(httpc);
        return rc;
    }

    size = cJSON_GetObjectItemCaseSensitive(resp, "size");
    fsize = (ssize_t)size->valuedouble;
    download_url = cJSON_GetObjectItemCaseSensitive(resp, "@microsoft.graph.downloadUrl");

    if ((ssize_t)buflen > 0 && fsize > (ssize_t)buflen) {
        cJSON_Delete(resp);
        http_client_close(httpc);
        return HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);
    }

    if (!fsize) {
        cJSON_Delete(resp);
        http_client_close(httpc);
        return 0;
    }

    http_client_reset(httpc);
    rc = __download_file(connect, httpc, download_url->valuestring, to);
    cJSON_Delete(resp);
    http_client_close(httpc);
    if (rc < 0)
        return rc;

    return fsize;
}

static ssize_t get_file_to_buffer(HiveConnect *base, const char *filename,
                                  bool decrypt, void *to, size_t buflen)
{
    OneDriveConnect *connect = (OneDriveConnect *)base;
    char file_path[PATH_MAX];
    int rc;

    rc = snprintf(file_path, sizeof(file_path), "%s/%s", FILES_DIR, filename);
    if (rc < 0 || rc >= sizeof(file_path))
        return HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);

    return __get_file_to_buffer(connect, file_path, decrypt, to, buflen);
}

static int __delete_file(OneDriveConnect *connect, const char *file_path)
{
    char url[MAX_URL_LEN] = {0};
    http_client_t *httpc;
    long resp_code = 0;
    int rc;

    rc = oauth_token_check_expire(connect->token);
    if (rc < 0)
        return rc;

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    sprintf(url, "%s:%s:", APP_ROOT, file_path);
    http_client_set_url(httpc, url);
    http_client_set_method(httpc, HTTP_METHOD_DELETE);
    http_client_set_header(httpc, "Authorization", get_bearer_token(connect->token));

    rc = http_client_request(httpc);
    if (rc) {
        rc = HIVE_CURL_ERROR(rc);
        goto error_exit;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);

    if (rc)
        return HIVE_CURL_ERROR(rc);

    if (resp_code == HttpStatus_Unauthorized) {
        oauth_token_set_expired(connect->token);
        return HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
    }

    if (resp_code != HttpStatus_NoContent)
        return HIVE_HTTP_STATUS_ERROR(resp_code);

    return 0;

error_exit:
    http_client_close(httpc);
    return rc;
}

static int delete_file(HiveConnect *base, const char *filename)
{
    OneDriveConnect *connect = (OneDriveConnect *)base;
    char file_path[PATH_MAX];
    int rc;

    rc = snprintf(file_path, sizeof(file_path), "%s/%s", FILES_DIR, filename);
    if (rc < 0 || rc >= sizeof(file_path))
        return HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);

    return __delete_file(connect, file_path);
}

static int __merge_array(cJSON *sub, cJSON *array)
{
    cJSON *item;
    size_t sub_sz = cJSON_GetArraySize(sub);
    size_t i;

    for (i = 0; i < sub_sz; ++i) {
        cJSON *name;
        cJSON *file;
        cJSON *folder;

        item = cJSON_GetArrayItem(sub, 0);

        name = cJSON_GetObjectItemCaseSensitive(item, "name");
        if (!name || !cJSON_IsString(name) || !name->valuestring || !*name->valuestring) {
            return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
        }

        file = cJSON_GetObjectItemCaseSensitive(item, "file");
        folder = cJSON_GetObjectItemCaseSensitive(item, "folder");
        if ((file && folder) || (!file && !folder)) {
            return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
        }

        if (file && !cJSON_IsObject(file)) {
            return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
        }

        if (folder && !cJSON_IsObject(folder)) {
            return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
        }

        cJSON_AddItemToArray(array, cJSON_DetachItemFromArray(sub, 0));
    }

    return 0;
}

static void __notify_user_files(cJSON *array, HiveFilesIterateCallback *callback, void *context)
{
    cJSON *item;

    cJSON_ArrayForEach(item, array) {
        cJSON *name;
        bool resume;

        name = cJSON_GetObjectItemCaseSensitive(item, "name");
        assert(name);

        resume = callback(name->valuestring, context);

        if (!resume)
            return;
    }
    callback(NULL, context);
}

static int list_files(HiveConnect *base, HiveFilesIterateCallback *callback, void *context)
{
    OneDriveConnect *connect = (OneDriveConnect *)base;
    char url[MAX_URL_LEN] = {0};
    char *next_url = NULL;
    http_client_t *httpc;
    cJSON *json = NULL;
    long resp_code;
    cJSON *array;
    int rc;

    rc = oauth_token_check_expire(connect->token);
    if (rc < 0)
        return rc;

    httpc = http_client_new();
    if (!httpc)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    sprintf(url, "%s:%s:/children", APP_ROOT, FILES_DIR);

    array = cJSON_CreateArray();
    if (!array) {
        rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
        goto error_exit;
    }

    next_url = url;
    while (next_url) {
        cJSON *sub_array;
        cJSON *next_link;
        char *p;

        http_client_reset(httpc);
        http_client_set_url(httpc, next_url);
        http_client_set_method(httpc, HTTP_METHOD_GET);
        http_client_set_header(httpc, "Authorization", get_bearer_token(connect->token));
        http_client_enable_response_body(httpc);

        rc = http_client_request(httpc);
        if (json)
            cJSON_Delete(json);

        if (rc) {
            rc = HIVE_CURL_ERROR(rc);
            break;
        }

        rc = http_client_get_response_code(httpc, &resp_code);
        if (rc) {
            rc = HIVE_CURL_ERROR(rc);
            break;
        }

        if (resp_code == HttpStatus_Unauthorized) {
            oauth_token_set_expired(connect->token);
            rc = HIVE_GENERAL_ERROR(HIVEERR_TRY_AGAIN);
            break;
        }

        if (resp_code != HttpStatus_OK) {
            rc = HIVE_HTTP_STATUS_ERROR(resp_code);
            break;
        }

        p = http_client_move_response_body(httpc, NULL);
        if (!p) {
            rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
            break;
        }

        json = cJSON_Parse(p);
        free(p);
        if (!json) {
            rc = HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);
            break;
        }

        sub_array = cJSON_GetObjectItemCaseSensitive(json, "value");
        if (!sub_array || !cJSON_IsArray(sub_array)) {
            cJSON_Delete(json);
            rc = HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
            break;
        }

        rc = __merge_array(sub_array, array);
        if (rc < 0) {
            cJSON_Delete(json);
            break;
        }

        next_link = cJSON_GetObjectItemCaseSensitive(json, "@odata.nextLink");
        if (next_link && (!cJSON_IsString(next_link) || !next_link->valuestring ||
                          !*next_link->valuestring)) {
            cJSON_Delete(json);
            rc = HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);
            break;
        }

        if (next_link)
            next_url = next_link->valuestring;
        else {
            next_url = NULL;
            cJSON_Delete(json);
            __notify_user_files(array, callback, context);
            rc = 0;
        }
    }

    cJSON_Delete(array);
    http_client_close(httpc);
    return rc;

error_exit:
    http_client_close(httpc);
    return rc;
}

typedef struct KVEntry {
    uint32_t val_len;
    uint8_t  val[0];
} KVEntry;

static int put_value(HiveConnect *base, const char *key, const void *value,
                     size_t length, bool encrypt)
{
    OneDriveConnect *connect = (OneDriveConnect *)base;
    char path[PATH_MAX] = {0};
    ssize_t size_new;
    ssize_t size;
    KVEntry *kv;
    uint8_t *buf;
    int rc;

    snprintf(path, sizeof(path), "%s/%s", KEYS_DIR, key);

    size = __get_file_length(connect, path);
    if ((int)size == HIVE_HTTP_STATUS_ERROR(HttpStatus_NotFound))
        size = 0;
    else if (size < 0)
        return (int)size;

    size_new = size + sizeof(KVEntry) + length;
    buf = calloc(1, size_new);
    if (!buf)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    if (size > 0) {
        rc = (int)__get_file_to_buffer(connect, path, encrypt, buf, size);
        if (rc < 0) {
            free(buf);
            return rc;
        }

        if (rc != (int)size) {
            free(buf);
            return HIVE_GENERAL_ERROR(HIVEERR_INVALID_PERSISTENCE_FILE);
        }
    }

    kv = (KVEntry *)(buf + size);
    kv->val_len = htonl((uint32_t)length);
    memcpy(kv->val, value, length);

    rc = __put_file_from_buffer(connect, buf, size_new, encrypt, path);
    free(buf);

    return rc;
}

static int set_value(HiveConnect *base, const char *key, const void *value,
                     size_t length, bool encrypt)
{
    OneDriveConnect *connect = (OneDriveConnect *)base;
    char path[PATH_MAX] = {0};
    KVEntry *kv;
    int rc;

    snprintf(path, sizeof(path), "%s/%s", KEYS_DIR, key);

    kv = (KVEntry *)calloc(1, sizeof(KVEntry) + length);
    if (!kv)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    kv->val_len = htonl((uint32_t)length);
    memcpy(kv->val, value, length);

    rc = __put_file_from_buffer(connect, kv, sizeof(KVEntry) + length,
                                encrypt, path);
    free(kv);

    return rc;
}

static int get_values(HiveConnect *base, const char *key, bool decrypt,
                      HiveKeyValuesIterateCallback *callback, void *context)
{
    OneDriveConnect *connect = (OneDriveConnect *)base;
    char path[PATH_MAX] = {0};
    ssize_t data_len;
    bool proceed;
    KVEntry *kv;
    ssize_t rc;
    void *buf;

    snprintf(path, sizeof(path), "%s/%s", KEYS_DIR, key);

    data_len = __get_file_length(connect, path);
    if ((int)data_len == HIVE_HTTP_STATUS_ERROR(HttpStatus_NotFound))
        data_len = 0;
    else if (data_len < 0)
        return (int)data_len;

    if (data_len > 0 && data_len <= sizeof(KVEntry))
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_PERSISTENT_DATA);

    if (!data_len)
        return 0;

    buf = calloc(1, data_len);
    if (!buf)
        return HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY);

    rc = __get_file_to_buffer(connect, path, decrypt, buf, data_len);
    if (rc < 0) {
        free(buf);
        return (int)rc;
    }

    if (rc != data_len) {
        free(buf);
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_PERSISTENCE_FILE);
    }

    kv = buf;
    while (1) {
        size_t entry_len;

        kv->val_len = ntohl(kv->val_len);
        if (kv->val_len > HIVE_MAX_VALUE_LEN) {
            free(buf);
            return HIVE_GENERAL_ERROR(HIVEERR_BAD_PERSISTENT_DATA);
        }

        entry_len = sizeof(KVEntry) + kv->val_len;
        data_len -= entry_len;
        if (data_len < 0 || (data_len > 0 && data_len <= sizeof(KVEntry))) {
            free(buf);
            return HIVE_GENERAL_ERROR(HIVEERR_BAD_PERSISTENT_DATA);
        }

        proceed = callback(key, kv->val, kv->val_len, context);
        if (!data_len || !proceed)
            break;

        kv = (KVEntry *)((uint8_t *)kv + entry_len);
    }

    free(buf);
    return 0;
}

static int delete_key(HiveConnect *base, const char *key)
{
    OneDriveConnect *connect = (OneDriveConnect *)base;
    char path[PATH_MAX];
    int rc;

    snprintf(path, sizeof(path), "%s/%s", KEYS_DIR, key);

    rc = __delete_file(connect, path);
    if (rc < 0 && rc != HIVE_HTTP_STATUS_ERROR(HttpStatus_NotFound))
        return rc;

    return 0;
}

HiveConnect *onedrive_client_connect(HiveClient *client, const HiveConnectOptions *opts)
{
    OneDriveConnectOptions *options = (OneDriveConnectOptions *)opts;
    oauth_options_t oauth_opts;
    OneDriveConnect *connect;
    char path_tmp[PATH_MAX];
    cJSON *keystore = NULL;
    int rc;

    assert(options);

    if (!options->client_id || !*options->client_id ||
        !options->redirect_url || !*options->redirect_url ||
        !options->scope || !*options->scope ||
        !strstr(options->scope, "Files.ReadWrite.AppFolder") ||
        !options->callback || options->backendType != HiveBackendType_OneDrive) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return NULL;
    }

    connect = (OneDriveConnect *)rc_zalloc(sizeof(OneDriveConnect), onedrive_connect_destructor);
    if (!connect) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        return NULL;
    }

    rc = snprintf(connect->keystore_path, sizeof(connect->keystore_path),
                  "%s/.data/onedrive.json", client->data_location);
    if (rc < 0 || rc >= (int)sizeof(connect->keystore_path)) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        deref(connect);
        return NULL;
    }

    strcpy(path_tmp, connect->keystore_path);
    rc = mkdirs(dirname(path_tmp), S_IRWXU);
    if (rc < 0 && errno != EEXIST) {
        hive_set_error(HIVE_SYS_ERROR(errno));
        deref(connect);
        return NULL;
    }

    if (!access(connect->keystore_path, F_OK)) {
        keystore = load_keystore_in_json(connect->keystore_path);
        if (!keystore) {
            hive_set_error(HIVE_SYS_ERROR(errno));
            deref(connect);
            return NULL;
        }
    }

    oauth_opts.store         = keystore;
    oauth_opts.authorize_url = URL_OAUTH METHOD_AUTHORIZE;
    oauth_opts.token_url     = URL_OAUTH METHOD_TOKEN;
    oauth_opts.client_id     = options->client_id;
    oauth_opts.scope         = options->scope;
    oauth_opts.redirect_url  = options->redirect_url;

    connect->token = oauth_token_new(&oauth_opts, &oauth_writeback, connect);
    if (keystore)
        cJSON_Delete(keystore);

    if (!connect->token) {
        deref(connect);
        return NULL;
    }

    connect->base.put_file_from_buffer = put_file_from_buffer;
    connect->base.get_file_length      = get_file_length;
    connect->base.get_file_to_buffer   = get_file_to_buffer;
    connect->base.list_files           = list_files;
    connect->base.delete_file          = delete_file;
    connect->base.put_value            = put_value;
    connect->base.set_value            = set_value;
    connect->base.get_values           = get_values;
    connect->base.delete_key           = delete_key;
    connect->base.disconnect           = disconnect;
    connect->base.expire_token         = expire_token;

    rc = oauth_token_request(connect->token,
                             (oauth_request_func_t *)options->callback,
                             options->context);
    if (rc < 0) {
        deref(connect);
        return NULL;
    }

    return &connect->base;
}
