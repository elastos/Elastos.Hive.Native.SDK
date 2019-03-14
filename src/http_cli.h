#ifndef __HTTP_CLI_H__
#define __HTTP_CLI_H__

typedef struct http_client http_client_t;

typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE
} http_method_t;

typedef enum {
    HTTP_VERSION_DEFAULT,
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1,
    HTTP_VERSION_2_0
} http_version_t;

int http_client_global_init();
void http_client_global_cleanup();

http_client_t *http_client_init();

int http_client_set_method(http_client_t *client, http_method_t method);
int http_client_set_url(http_client_t *client, const char *url);
int http_client_set_url_escape(http_client_t *client, const char *url);
int http_client_get_url(http_client_t *client, char **url);
int http_client_set_path(http_client_t *client, const char *path);
int http_client_set_query(http_client_t *client, const char *name, const char *value);
int http_client_set_header(http_client_t *client, const char *name, const char *value);
int http_client_set_timeout(http_client_t *client, int timeout /* seconds */);
int http_client_set_version(http_client_t *client, http_version_t version);

typedef size_t http_client_request_body_callback(char *buffer, size_t size, size_t nitems, void *userdata);

int http_client_set_request_body_instant(http_client_t *client, void *data, size_t len);
int http_client_set_request_body(http_client_t *client, http_client_request_body_callback *body_callback, void *userdata);

typedef size_t http_client_response_body_callback(char *ptr, size_t size, size_t nmemb, void *userdata);

int http_client_set_response_body_instant(http_client_t *client, void *data, size_t len);
int http_client_set_response_body(http_client_t *client, http_client_response_body_callback *body_callback, void *userdata);
size_t http_client_get_response_body_length(http_client_t *client);

/* if need response headers, add response header functions */

int http_client_request(http_client_t *client);

int http_client_get_response_code(http_client_t *client);

int http_client_reset(http_client_t *client);

char *http_client_escape(http_client_t *client, const char *data, size_t len);

char *http_client_unescape(http_client_t *client, const char *data, size_t len, size_t *outlen);

void http_client_memory_free(void *ptr);

const char *http_client_error(int error);

#endif // __HTTP_CLI_H__
