#include <pthread.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>

#include "http_cli.h"

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
    CURLU *url;
    CURL *curl;
    struct curl_slist *hdr;
    http_response_body_t response_body;
};

#ifndef NDEBUG
static void dump(const char *text,
    FILE *stream, unsigned char *ptr, size_t size)
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

static int my_trace(CURL *handle, curl_infotype type,
    char *data, size_t size, void *userp)
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

static void http_client_cleanup(void *arg)
{
    http_client_t *client = (http_client_t *)arg;

    if (client->curl)
        curl_easy_cleanup(client->curl);
    if (client->url)
        curl_url_cleanup(client->url);
    if (client->hdr)
        curl_slist_free_all(client->hdr);
    free(client);
}

static void curl_share_lock(CURL *handle, curl_lock_data data, curl_lock_access access, void *userptr)
{
    (void)handle;
    (void)data;
    (void)access;
    (void)userptr;

    pthread_mutex_lock(&share_lock);
}

static void curl_share_unlock(CURL *handle, curl_lock_data data, void *userptr)
{
    (void)handle;
    (void)data;
    (void)userptr;

    pthread_mutex_unlock(&share_lock);
}

int http_client_global_init()
{
    int rc;

    if (curl_global_init(CURL_GLOBAL_ALL))
        return -1;

    curl_share = curl_share_init();
    if (!curl_share) {
        http_client_global_cleanup();
        return -1;
    }

    rc = pthread_key_create(&http_client_key, http_client_cleanup);
    if (rc) {
        http_client_global_cleanup();
        return -1;
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

void http_client_global_cleanup()
{
    pthread_key_delete(http_client_key);
    curl_share_cleanup(curl_share);
}

static void http_response_reset(http_response_body_t *resp)
{
    memset(resp, 0, sizeof(*resp));
}

static int http_response_append(http_response_body_t *response, void *data, size_t len)
{
    if (response->unused < len)
        return -1;

    memcpy(response->data + response->used, data, len);
    response->unused -= len;
    response->used += len;
    return 0;
}

http_client_t *http_client_init()
{
    http_client_t *client = (http_client_t *)pthread_getspecific(http_client_key);

    if (!client) {
        client = (http_client_t *)calloc(1, sizeof(http_client_t));
        if (!client)
            return NULL;

        client->url = curl_url();
        if (!client->url) {
            http_client_cleanup(client);
            return NULL;
        }

        client->curl = curl_easy_init();
        if (!client->curl) {
            http_client_cleanup(client);
            return NULL;
        }
    } else {
        http_client_reset(client);
    }

#ifndef NDEBUG
    curl_easy_setopt(client->curl, CURLOPT_DEBUGFUNCTION, my_trace);
    curl_easy_setopt(client->curl, CURLOPT_VERBOSE, 1L);
#endif
    curl_easy_setopt(client->curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(client->curl, CURLOPT_SHARE, curl_share);
    curl_easy_setopt(client->curl, CURLOPT_CURLU, client->url);

    pthread_setspecific(http_client_key, client);

    return client;
}

int http_client_set_method(http_client_t *client, http_method_t method)
{
    int rc = 0;

    switch (method) {
    case HTTP_METHOD_GET:
        curl_easy_setopt(client->curl, CURLOPT_HTTPGET, 1L);
        break;
    case HTTP_METHOD_POST:
        curl_easy_setopt(client->curl, CURLOPT_POST, 1L);
        break;
    case HTTP_METHOD_PUT:
        curl_easy_setopt(client->curl, CURLOPT_CUSTOMREQUEST, "PUT");
        break;
    case HTTP_METHOD_DELETE:
        curl_easy_setopt(client->curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        break;
    default:
        rc = -1;
        break;
    }

    return rc;
}

int http_client_set_url(http_client_t *client, const char *url)
{
    curl_url_set(client->url, CURLUPART_URL, url, CURLU_URLENCODE);
    return 0;
}

int http_client_set_url_escape(http_client_t *client, const char *url)
{
    curl_url_set(client->url, CURLUPART_URL, url, 0);
    return 0;
}

int http_client_get_url(http_client_t *client, char **url)
{
    CURLUcode rc;

    rc = curl_url_get(client->url, CURLUPART_URL, url, 0);
    return rc == CURLUE_OK ? 0 : -1;
}

int http_client_set_path(http_client_t *client, const char *path)
{
    curl_url_set(client->url, CURLUPART_PATH, path, CURLU_URLENCODE);
    return 0;
}

int http_client_set_query(http_client_t *client, const char *name, const char *value)
{
    char *query = alloca(strlen(name) + strlen("=") + strlen(value) + 1);

    sprintf(query, "%s=%s", name, value);

    curl_url_set(client->url, CURLUPART_QUERY, query, CURLU_URLENCODE | CURLU_APPENDQUERY);
    return 0;
}

int http_client_set_header(http_client_t *client, const char *name, const char *value)
{
    char *header = alloca(strlen(name) + strlen(": ") + strlen(value) + 1);

    sprintf(header, "%s: %s", name, value);

    client->hdr = curl_slist_append(client->hdr, header);

    return 0;
}

int http_client_set_timeout(http_client_t *client, int timeout)
{
    curl_easy_setopt(client->curl, CURLOPT_TIMEOUT, timeout);
    return 0;
}

int http_client_set_version(http_client_t *client, http_version_t version)
{
    curl_easy_setopt(client->curl, CURLOPT_HTTP_VERSION, curl_http_versions[version]);
    return 0;
}

int http_client_set_request_body_instant(http_client_t *client, void *data, size_t len)
{
    curl_easy_setopt(client->curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(client->curl, CURLOPT_POSTFIELDSIZE, len);
    return 0;
}

int http_client_set_request_body(http_client_t *client, http_client_request_body_callback *body_callback, void *userdata)
{
    curl_easy_setopt(client->curl, CURLOPT_READFUNCTION, body_callback);
    curl_easy_setopt(client->curl, CURLOPT_READDATA, userdata);
    return 0;
}

static size_t http_response_body_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    http_response_body_t *response_body = (http_response_body_t *)userdata;
    int rc;

    rc = http_response_append(response_body, ptr, size * nmemb);
    return rc ? -1 : size * nmemb;
}

int http_client_set_response_body_instant(http_client_t *client, void *data, size_t len)
{
    client->response_body.used = 0;
    client->response_body.unused = len;
    client->response_body.data = data;

    curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, http_response_body_write_callback);
    curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, &client->response_body);
    return 0;
}

int http_client_set_response_body(http_client_t *client, http_client_response_body_callback *body_callback, void *userdata)
{
    curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, body_callback);
    curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, userdata);
    return 0;
}

size_t http_client_get_response_body_length(http_client_t *client)
{
    return client->response_body.used;
}

int http_client_request(http_client_t *client)
{
    CURLcode rc;

    if (client->hdr)
        curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, client->hdr);

    rc = curl_easy_perform(client->curl);
    return rc == CURLE_OK ? 0 : -1;
}

int http_client_get_response_code(http_client_t *client)
{
    CURLcode rc;
    long response_code;

    rc = curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &response_code);
    return rc == CURLE_OK ? response_code : -1;
}

int http_client_reset(http_client_t *client)
{
    curl_easy_reset(client->curl);
    curl_url_set(client->url, CURLUPART_URL, NULL, 0);
    if (client->hdr) {
        curl_slist_free_all(client->hdr);
        client->hdr = NULL;
    }
    http_response_reset(&client->response_body);

    return 0;
}

char *http_client_escape(http_client_t *client, const char *data, size_t len)
{
    return curl_easy_escape(client->curl, data, len);
}

char *http_client_unescape(http_client_t *client, const char *data, size_t len, size_t *outlen)
{
    return curl_easy_unescape(client->curl, data, len, (int *)outlen);
}

void http_client_memory_free(void *ptr)
{
    curl_free(ptr);
}

const char *http_client_error(int error)
{
    // to be implemented
    return NULL;
}

