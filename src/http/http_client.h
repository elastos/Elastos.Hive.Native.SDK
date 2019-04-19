#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct http_client http_client_t;

typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_PATCH
} http_method_t;

typedef enum {
    HTTP_VERSION_DEFAULT,
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1,
    HTTP_VERSION_2_0
} http_version_t;

/*
 * Http global APIs.
 */

int  http_client_init(void);
void http_client_cleanup(void);

/*
 * Http client instance.
 */

http_client_t *http_client_new(void);

void http_client_close(http_client_t *client);
void http_client_reset(http_client_t *client);

/*
 * Http client options.
 */

int http_client_set_method(http_client_t *, http_method_t method);
int http_client_set_url(http_client_t *, const char *url);
int http_client_set_url_escape(http_client_t *, const char *url);
int http_client_get_url(http_client_t *, char **url);
int http_client_set_path(http_client_t *, const char *path);
int http_client_set_query(http_client_t *, const char *name, const char *value);
int http_client_set_header(http_client_t *, const char *name, const char *value);
int http_client_set_timeout(http_client_t *, int timeout /* seconds */);
int http_client_set_version(http_client_t *, http_version_t version);

/*
 * Http client request/response body.
 */

typedef size_t (*http_client_request_body_callback_t)(char *buffer,
                                    size_t size, size_t nitems, void *userdata);

typedef size_t (*http_client_response_body_callback_t)(char *buffer,
                                    size_t size, size_t nitems, void *userdata);


int http_client_set_request_body_instant(http_client_t *, void *data, size_t len);
int http_client_set_response_body_instant(http_client_t *, void *data, size_t len);
int http_client_set_request_body(http_client_t *,
                      http_client_request_body_callback_t *cb, void *userdata);
int http_client_set_response_body(http_client_t *client,
                      http_client_response_body_callback_t *cb, void *userdata);
size_t http_client_get_response_body_length(http_client_t *);
int http_client_get_response_code(http_client_t *, long *response_code);


/*
 * Http client request API
 */
int http_client_request(http_client_t *client);

/*
 * Escape/Unescape operation APIs.
 */
char *http_client_escape(http_client_t *client, const char *data, size_t len);

char *http_client_unescape(http_client_t *client, const char *data, size_t len,
                           size_t *outlen);

void http_client_memory_free(void *ptr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __HTTP_CLIENT_H__
