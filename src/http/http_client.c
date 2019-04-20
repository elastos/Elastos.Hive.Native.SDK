#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include <curl/curl.h>
#include <crystal.h>

#include "http_client.h"

static CURLSH *curl_share = NULL;
static pthread_mutex_t share_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t http_client_key;

static long curl_http_versions[] = {
    CURL_HTTP_VERSION_NONE,
    CURL_HTTP_VERSION_1_0,
    CURL_HTTP_VERSION_1_1,
    CURL_HTTP_VERSION_2_0
};

typedef struct http_response_body {
    size_t used;
    size_t unused;
    void *data;
} http_response_body_t;

struct http_client {
    CURL *curl;
    CURLU *url;
    struct curl_slist *hdr;
    http_response_body_t response_body;
};

static inline int __curlcode_to_error(CURLcode code)
{
    // TODO;
    return -1;
}

static inline int __curlucode_to_error(CURLUcode code)
{
    // TODO;
    return -1;
}

#ifndef NDEBUG
static
void dump(const char *text, FILE *stream, unsigned char *ptr, size_t size)
{
    size_t i;
    size_t c;
    unsigned int width=0x10;

    fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
            text, (long)size, (long)size);

    for(i=0; i<size; i+= width) {
        fprintf(stream, "%4.4lx: ", (long)i);

        /* show hex to the left */
        for(c = 0; c < width; c++) {
            if(i+c < size)
                fprintf(stream, "%02x ", ptr[i+c]);
            else
                fputs("   ", stream);
        }

        /* show data on the right */
        for(c = 0; (c < width) && (i+c < size); c++) {
            char x = (ptr[i+c] >= 0x20 && ptr[i+c] < 0x80) ? ptr[i+c] : '.';
            fputc(x, stream);
        }

        fputc('\n', stream); /* newline */
    }
}

static
int trace_func(CURL *handle, curl_infotype type, char *data, size_t size,
               void *userp)
{
    const char *text;
    (void)handle; /* prevent compiler warning */
    (void)userp;

    switch (type) {
        case CURLINFO_TEXT:
            fprintf(stderr, "== Info: %s", data);
        default: /* in case a new one is introduced to shock us */
            return 0;

        case CURLINFO_HEADER_OUT:
            text = "=> Send header";
            break;
        case CURLINFO_DATA_OUT:
            text = "=> Send data";
            break;
        case CURLINFO_SSL_DATA_OUT:
            text = "=> Send SSL data";
            break;
        case CURLINFO_HEADER_IN:
            text = "<= Recv header";
            break;
        case CURLINFO_DATA_IN:
            text = "<= Recv data";
            break;
        case CURLINFO_SSL_DATA_IN:
            text = "<= Recv SSL data";
            break;
    }

    dump(text, stderr, (unsigned char *)data, size);
    return 0;
}
#endif

static
void curl_share_lock(CURL *handle, curl_lock_data data, curl_lock_access access,
                     void *userptr)
{
    (void)handle;
    (void)data;
    (void)access;
    (void)userptr;

    pthread_mutex_lock(&share_lock);
}

static
void curl_share_unlock(CURL *handle, curl_lock_data data, void *userptr)
{
    (void)handle;
    (void)data;
    (void)userptr;

    pthread_mutex_unlock(&share_lock);
}

static void http_client_destructor(void *arg)
{
    assert(arg);

    http_client_close((http_client_t *)arg);
}

int http_client_init()
{
    CURLcode code;
    int rc;

    code = curl_global_init(CURL_GLOBAL_ALL);
    if (code != CURLE_OK) {
        vlogE("HttpClient: Initialize global curl error (%d)", code);
        return __curlcode_to_error(code);
    }

    code = CURLE_OUT_OF_MEMORY;
    curl_share = curl_share_init();
    if (!curl_share) {
        vlogE("HttpClient: Initialize curl share error.");
        http_client_cleanup();
        return __curlcode_to_error(code);
    }

    rc = pthread_key_create(&http_client_key, http_client_destructor);
    if (rc)  {
        http_client_cleanup();
        return __curlcode_to_error(code);
    }

    curl_share_setopt(curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
    curl_share_setopt(curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
    curl_share_setopt(curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
    curl_share_setopt(curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
    curl_share_setopt(curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_PSL);
    curl_share_setopt(curl_share, CURLSHOPT_LOCKFUNC, curl_share_lock);
    curl_share_setopt(curl_share, CURLSHOPT_UNLOCKFUNC, curl_share_unlock);

    return 0;
}

void http_client_cleanup(void)
{
    pthread_key_delete(http_client_key);
    curl_share_cleanup(curl_share);
}

static void http_client_destroy(void *ptr)
{
    http_client_t *client = (http_client_t *)ptr;

    assert(client);

    if (client->curl)
        curl_easy_cleanup(client->curl);
    if (client->url)
        curl_url_cleanup(client->url);
    if (client->hdr)
        curl_slist_free_all(client->hdr);
}

http_client_t *http_client_new(void)
{
    http_client_t *client;

    client = (http_client_t *)pthread_getspecific(http_client_key);
    if (!client) {
        client = (http_client_t *)rc_zalloc(sizeof(http_client_t), http_client_destroy);
        if (!client) {
            // hive_set_error();
            return NULL;
        }

        client->url = curl_url();
        if (!client->url) {
        // hive_set_error(); out of memory.
            deref(client);
            return NULL;
        }

        client->curl = curl_easy_init();
        if (!client->curl) {
        // hive_set_error(); out of memory.
            deref(client);
            return NULL;
        }
    } else {
        http_client_reset(client);
    }

#ifndef NDEBUG
    curl_easy_setopt(client->curl, CURLOPT_DEBUGFUNCTION, trace_func);
    curl_easy_setopt(client->curl, CURLOPT_VERBOSE, 1L);
#endif
    curl_easy_setopt(client->curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(client->curl, CURLOPT_SHARE, curl_share);
    curl_easy_setopt(client->curl, CURLOPT_CURLU, client->url);
#if defined(_WIN32) || defined(_WIN64)
    curl_easy_setopt(client->curl, CURLOPT_SSL_VERIFYPEER, 0);
#endif

    pthread_setspecific(http_client_key, client);

    return client;
}

void http_client_close(http_client_t *client)
{
    if (client)
        deref(client);
}

void http_client_reset(http_client_t *client)
{
    assert(client);

    curl_easy_reset(client->curl);
    curl_url_set(client->url, CURLUPART_URL, NULL, 0);

    if (client->hdr) {
        curl_slist_free_all(client->hdr);
        client->hdr = NULL;
    }

    memset(&client->response_body, 0, sizeof(http_response_body_t));
}

int http_client_set_method(http_client_t *client, http_method_t method)
{
    CURLcode code = CURLE_UNSUPPORTED_PROTOCOL;

    switch (method) {
    case HTTP_METHOD_GET:
        code = curl_easy_setopt(client->curl, CURLOPT_HTTPGET, 1L);
        break;
    case HTTP_METHOD_POST:
        code = curl_easy_setopt(client->curl, CURLOPT_POST, 1L);
        break;
    case HTTP_METHOD_PUT:
        code = curl_easy_setopt(client->curl, CURLOPT_CUSTOMREQUEST, "PUT");
        break;
    case HTTP_METHOD_DELETE:
        code = curl_easy_setopt(client->curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        break;
    case HTTP_METHOD_PATCH:
        curl_easy_setopt(client->curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        break;
    default:
        assert(0);
        break;
    }

    return __curlcode_to_error(code);
}

int http_client_set_url(http_client_t *client, const char *url)
{
    CURLUcode code;

    assert(client);
    assert(url);
    assert(*url);

    code = curl_url_set(client->url, CURLUPART_URL, url, CURLU_URLENCODE);
    if (code != CURLUE_OK) {
        vlogE("HttpClient: Set url %s error (%d)", url, code);
        return __curlucode_to_error(code);
    }

    return 0;
}

int http_client_set_url_escape(http_client_t *client, const char *url)
{
    CURLUcode code;

    assert(client);
    assert(url);
    assert(*url);

    code = curl_url_set(client->url, CURLUPART_URL, url, 0);
    if (code != CURLUE_OK) {
        vlogE("HttpClient: Escape url %s error (%d)", url, code);
        return __curlucode_to_error(code);
    }

    return 0;
}

int http_client_get_url(http_client_t *client, char **url)
{
    CURLUcode code;

    assert(client);
    assert(url);

    code = curl_url_get(client->url, CURLUPART_URL, url, 0);
    if (code != CURLUE_OK)  {
        vlogE("HttpClient: Get url from curl error (%d)", code);
        return  __curlucode_to_error(code);
    }

    return 0;
}

int http_client_set_path(http_client_t *client, const char *path)
{
    CURLUcode code;

    assert(client);
    assert(path);
    assert(*path);

    code = curl_url_set(client->url, CURLUPART_PATH, path, CURLU_URLENCODE);
    if (code != CURLUE_OK)  {
        // TODO;
        return  __curlucode_to_error(code);
    }

    return 0;
}

int http_client_set_query(http_client_t *client,
                          const char *name, const char *value)
{
    char *query;
    CURLUcode code;

    assert(client);
    assert(name);
    assert(value);
    assert(*name);
    assert(*value);

    query = alloca(strlen(name) + strlen(value) + 2);
    sprintf(query, "%s=%s", name, value);

    code = curl_url_set(client->url, CURLUPART_QUERY, query,
                CURLU_URLENCODE | CURLU_APPENDQUERY);
    if (code != CURLUE_OK)  {
        // TODO;
        return  __curlucode_to_error(code);
    }

    return 0;
}

int http_client_set_header(http_client_t *client,
                           const char *name, const char *value)
{
    char *header;
    struct curl_slist *hdr;

    assert(client);
    assert(name);
    assert(value);
    assert(*name);
    assert(*value);

    header = alloca(strlen(name) + strlen(value) + 3);
    sprintf(header, "%s: %s", name, value);

    hdr = curl_slist_append(client->hdr, header);
    if (!hdr) {
        // TODO;
        return __curlcode_to_error(CURLE_OUT_OF_MEMORY);
    }

    client->hdr = hdr;
    return 0;
}

int http_client_set_timeout(http_client_t *client, int timeout)
{
    assert(client);
    assert(time > 0);

    curl_easy_setopt(client->curl, CURLOPT_TIMEOUT, timeout);
    return 0;
}

int http_client_set_version(http_client_t *client, http_version_t version)
{
    CURLcode code;

    assert(client);
    assert(version >= CURL_HTTP_VERSION_NONE);
    assert(version <= CURL_HTTP_VERSION_2_0);

    code = curl_easy_setopt(client->curl, CURLOPT_HTTP_VERSION,
                            curl_http_versions[version]);
    if (code != CURLE_OK) {
        // TODO;
        return __curlcode_to_error(CURLE_OUT_OF_MEMORY);
    }

    return 0;
}

int http_client_set_request_body_instant(http_client_t *client,
                                         void *data, size_t len)
{
    assert(client);
    assert(data);
    assert(len);

    curl_easy_setopt(client->curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(client->curl, CURLOPT_POSTFIELDSIZE, len);

    return 0;
}

int http_client_set_request_body(http_client_t *client,
                                 http_client_request_body_callback_t *callback,
                                 void *userdata)
{
    assert(client);
    assert(callback);

    curl_easy_setopt(client->curl, CURLOPT_READFUNCTION, callback);
    curl_easy_setopt(client->curl, CURLOPT_READDATA, userdata);

    return 0;
}

static
size_t http_response_body_write_callback(char *ptr, size_t size, size_t nmemb,
                                         void *userdata)
{
    http_response_body_t *response = (http_response_body_t *)userdata;
    size_t length = size * nmemb;

    if (response->unused < length)
        return (size_t)-1;

    memcpy((char *)response->data + response->used, ptr, length);
    response->unused -= length;
    response->used += length;

    return length;
}

int http_client_set_response_body_instant(http_client_t *client,
                                          void *data, size_t len)
{
    assert(client);
    assert(data);
    assert(len);

    client->response_body.used = 0;
    client->response_body.unused = len;
    client->response_body.data = data;

    curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION,
                     http_response_body_write_callback);

    curl_easy_setopt(client->curl, CURLOPT_WRITEDATA,
                     &client->response_body);
    return 0;
}

int http_client_set_response_body(http_client_t *client,
                                  http_client_response_body_callback_t *callback,
                                  void *userdata)
{
    assert(client);
    assert(callback);

    curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, userdata);

    return 0;
}

size_t http_client_get_response_body_length(http_client_t *client)
{
    return client->response_body.used;
}

int http_client_request(http_client_t *client)
{
    CURLcode code;

    assert(client);

    if (client->hdr)
        curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, client->hdr);

    code = curl_easy_perform(client->curl);
    if (code != CURLE_OK) {
        vlogE("HttpClient: Perform http request error (%d)", code);
        return __curlcode_to_error(code);
    }

    return 0;
}

int http_client_get_response_code(http_client_t *client, long *response_code)
{
    CURLcode code;

    assert(client);
    assert(response_code);

    code = curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE,
                             response_code);
    if (code != CURLE_OK) {
        // TODO;
        return __curlcode_to_error(code);
    }

    return 0;
}

char *http_client_escape(http_client_t *client, const char *data, size_t len)
{
    char *escaped_data;

    assert(client);
    assert(data);
    assert(len);

    escaped_data = curl_easy_escape(client->curl, data, len);
    if (!escaped_data) {
        // TODO;
        //__curlcode_to_error(CURLE_OUT_OF_MEMORY);
        return NULL;
    }

    return escaped_data;
}

char *http_client_unescape(http_client_t *client, const char *data, size_t len,
                           size_t *outlen)
{
    char *unescaped_data;
    int _outlen = 0;

    assert(client);
    assert(data);
    assert(len);
    assert(outlen);

    unescaped_data = curl_easy_unescape(client->curl, data, len, &_outlen);
    if (!unescaped_data) {
        // TODO;
        // __curlcode_to_error(CURLE_OUT_OF_MEMORY);
        return NULL;
    }

    *outlen = (size_t)_outlen;
    return unescaped_data;
}

void http_client_memory_free(void *ptr)
{
    curl_free(ptr);
}
