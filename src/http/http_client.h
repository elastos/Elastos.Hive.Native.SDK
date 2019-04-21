#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

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

/******************************************************************************
 * Http global APIs.
 *****************************************************************************/

int http_client_init(void);

void http_client_cleanup(void);

/******************************************************************************
 * Http client instance.
 *****************************************************************************/

http_client_t *http_client_new(void);

void http_client_close(http_client_t *client);

void http_client_reset(http_client_t *client);

/******************************************************************************
 * Http client options.
 *****************************************************************************/

int http_client_set_method(http_client_t *client, http_method_t method);
int http_client_set_url(http_client_t *client, const char *url);
int http_client_set_url_escape(http_client_t *client, const char *url);
int http_client_get_url(http_client_t *client, char **url);
int http_client_set_path(http_client_t *client, const char *path);
int http_client_set_query(http_client_t *client, const char *name, const char *value);
int http_client_set_header(http_client_t *client, const char *name, const char *value);
int http_client_set_timeout(http_client_t *client, int timeout /* seconds */);
int http_client_set_version(http_client_t *client, http_version_t version);

/******************************************************************************
 * Http client request body.
 *****************************************************************************/

typedef size_t (*http_client_request_body_callback_t)(char *buffer, size_t size,
                                                      size_t nitems,
                                                      void *userdata);

int http_client_set_request_body_instant(http_client_t *client,
                                        void *data, size_t len);

int http_client_set_request_body(http_client_t *client,
                                 http_client_request_body_callback_t *callback,
                                 void *userdata);

/******************************************************************************
 * Http client response body.
 *****************************************************************************/
typedef size_t (*http_client_response_body_callback_t)(char *buffer, size_t size,
                                                       size_t nmemb,
                                                       void *userdata);

int http_client_set_response_body_instant(http_client_t *client,
                                          void *data, size_t len);

int http_client_set_response_body(http_client_t *client,
                                  http_client_response_body_callback_t *callback,
                                  void *userdata);

size_t http_client_get_response_body_length(http_client_t *client);



/* if need response headers, add response header functions */

/******************************************************************************
 * Http client request API
 *****************************************************************************/
int http_client_request(http_client_t *client);

/******************************************************************************
 * Http client response API
 *****************************************************************************/
int http_client_get_response_code(http_client_t *client, long *response_code);

/******************************************************************************
 * Escape/Unescape operation APIs.
 *****************************************************************************/
char *http_client_escape(http_client_t *client, const char *data, size_t len);

char *http_client_unescape(http_client_t *client, const char *data, size_t len,
                           size_t *outlen);

// TODO:
void http_client_memory_free(void *ptr);

#endif // __HTTP_CLIENT_H__
