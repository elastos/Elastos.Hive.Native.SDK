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

#ifndef __ELA_HIVE_H__
#define __ELA_HIVE_H__

#if defined(__APPLE__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdocumentation"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

#if defined(_WIN32) || defined(_WIN64)
typedef ptrdiff_t ssize_t;
#endif

#if defined(HIVE_STATIC)
    #define HIVE_API
#elif defined(HIVE_DYNAMIC)
    #ifdef HIVE_BUILD
        #if defined(_WIN32) || defined(_WIN64)
            #define HIVE_API __declspec(dllexport)
        #else
            #define HIVE_API __attribute__((visibility("default")))
        #endif
    #else
        #if defined(_WIN32) || defined(_WIN64)
            #define HIVE_API __declspec(dllimport)
        #else
            #define HIVE_API
        #endif
    #endif
#else
    #define HIVE_API
#endif

#define HIVE_MAX_FILE_SIZE (32U * 1024 * 1024)
#define HIVE_MAX_IPFS_CID_LEN (64)

typedef struct HiveClient HiveClient;
typedef struct HiveConnect HiveConnect;
typedef struct IPFSCid {
    char content[HIVE_MAX_IPFS_CID_LEN + 1];
} IPFSCid;

enum HiveBackendType {
    HiveBackendType_IPFS      = 0x01,
    HiveBackendType_OneDrive  = 0x10,
    HiveBackendType_ownCloud  = 0x51,
    HiveDriveType_Butt        = 0x99
};

typedef enum HiveLogLevel {
    HiveLogLevel_None    = 0,
    HiveLogLevel_Fatal   = 1,
    HiveLogLevel_Error   = 2,
    HiveLogLevel_Warning = 3,
    HiveLogLevel_Info    = 4,
    HiveLogLevel_Debug   = 5,
    HiveLogLevel_Trace   = 6,
    HiveLogLevel_Verbose = 7
} HiveLogLevel;

#define HIVE_MAX_VALUE_LEN (32U * 1024)

HIVE_API
void hive_log_init(HiveLogLevel level, const char *log_file,
                   void (*log_printer)(const char *format, va_list args));

typedef struct HiveOptions {
    char *data_location;
} HiveOptions;

typedef struct HiveConnectOptions {
    int backendType;
} HiveConnectOptions;

typedef int HiveRequestAuthenticationCallback(const char *url, void *context);

typedef struct OneDriveConnectOptions {
    int backendType;
    const char *client_id;
    const char *scope;
    const char *redirect_url;

    HiveRequestAuthenticationCallback *callback;
    void *context;
} OneDriveConnectOptions;

typedef struct HiveRpcNode {
    const char *ipv4;
    const char *ipv6;
    const char *port;
} HiveRpcNode;

typedef struct IPFSConnectOptions {
    int backendType;
    size_t rpc_node_count;
    HiveRpcNode *rpcNodes;
} IPFSConnectOptions;

HIVE_API
HiveClient *hive_client_new(const HiveOptions *options);

HIVE_API
int hive_client_close(HiveClient *client);

HIVE_API
HiveConnect *hive_client_connect(HiveClient *client, HiveConnectOptions *options);

HIVE_API
int hive_client_disconnect(HiveConnect *connect);

HIVE_API
int hive_set_encrypt_key(HiveConnect *connect, const char *encrypt_key);

HIVE_API
int hive_put_file(HiveConnect *connect, const char *from, bool encrypt, const char *filename);

HIVE_API
int hive_put_file_from_buffer(HiveConnect *connect, const void *from, size_t length, bool encrypt, const char *filename);

HIVE_API
ssize_t hive_get_file_length(HiveConnect *connect, const char *filename);

HIVE_API
ssize_t hive_get_file_to_buffer(HiveConnect *connect, const char *filename, bool decrypt, void *to, size_t buflen);

HIVE_API
ssize_t hive_get_file(HiveConnect *connect, const char *filename, bool decrypt, const char *to);

HIVE_API
int hive_delete_file(HiveConnect *connect, const char *filename);

typedef bool HiveFilesIterateCallback(const char *filename, void *context);
HIVE_API
int hive_list_files(HiveConnect *connect, HiveFilesIterateCallback *callback, void *context);

HIVE_API
int hive_ipfs_put_file(HiveConnect *connect, const char *from, bool encrypt, IPFSCid *cid);

HIVE_API
int hive_ipfs_put_file_from_buffer(HiveConnect *connect, const void *from, size_t length, bool encrypt, IPFSCid *cid);

HIVE_API
ssize_t hive_ipfs_get_file_length(HiveConnect *connect, const IPFSCid *cid);

HIVE_API
ssize_t hive_ipfs_get_file_to_buffer(HiveConnect *connect, const IPFSCid *cid,  bool decrypt, void *to, size_t bulen);

HIVE_API
ssize_t hive_ipfs_get_file(HiveConnect *connect, const IPFSCid *cid, bool decrypt, const char *to);

HIVE_API
int hive_put_value(HiveConnect *connect, const char *key, const void *value, size_t length, bool encrypt);

HIVE_API
int hive_set_value(HiveConnect *connect, const char *key, const void *value, size_t length, bool encrypt);

typedef bool HiveKeyValuesIterateCallback(const char *key, const void *value, size_t length, void *context);

HIVE_API
int hive_get_values(HiveConnect *connect, const char *key, bool decrypt, HiveKeyValuesIterateCallback *callback, void *context);

HIVE_API
int hive_delete_key(HiveConnect *connect, const char *key);

/******************************************************************************
 * Error handling
 *****************************************************************************/

// Facility code
#define HIVEF_GENERAL                                0x01
#define HIVEF_SYS                                    0x02
#define HIVEF_RESERVED1                              0x03
#define HIVEF_RESERVED2                              0x04
#define HIVEF_CURL                                   0x05
#define HIVEF_CURLU                                  0x06
#define HIVEF_HTTP_STATUS                            0x07

/**
 * \~English
 * Success.
 */
#define HIVEOK                                       0

/**
 * \~English
 * Argument(s) is(are) invalid.
 */
#define HIVEERR_INVALID_ARGS                         0x01

/**
 * \~English
 * Runs out of memory.
 */
#define HIVEERR_OUT_OF_MEMORY                        0x02

/**
 * \~English
 * Buffer size is too small.
 */
#define HIVEERR_BUFFER_TOO_SMALL                     0x03

/**
 * \~English
 * Persistent data is corrupted.
 */
#define HIVEERR_BAD_PERSISTENT_DATA                  0x04

/**
 * \~English
 * Persistent file is invalid.
 */
#define HIVEERR_INVALID_PERSISTENCE_FILE             0x05

/**
 * \~English
 * Credential is invalid.
 */
#define HIVEERR_INVALID_CREDENTIAL                   0x06

/**
 * \~English
 * Hive SDK not ready.
 */
#define HIVEERR_NOT_READY                            0x07

/**
 * \~English
 * The requested entity does not exist.
 */
#define HIVEERR_NOT_EXIST                            0x08

/**
 * \~English
 * The entity exists already.
 */
#define HIVEERR_ALREADY_EXIST                        0x09

/**
 * \~English
 * User ID is invalid.
 */
#define HIVEERR_INVALID_USERID                       0x0A

/**
 * \~English
 * Failed because wrong state.
 */
#define HIVEERR_WRONG_STATE                          0x0B

/**
 * \~English
 * Stream busy.
 */
#define HIVEERR_BUSY                                 0x0C

/**
 * \~English
 * Language binding error.
 */
#define HIVEERR_LANGUAGE_BINDING                     0x0D

/**
 * \~English
 * Encryption failed.
 */
#define HIVEERR_ENCRYPT                              0x0E

/**
 * \~English
 * Not implemented yet.
 */
#define HIVEERR_NOT_IMPLEMENTED                      0x0F

/**
 * \~English
 * Not supported.
 */
#define HIVEERR_NOT_SUPPORTED                        0x10

/**
 * \~English
 * Limits are exceeded.
 */
#define HIVEERR_LIMIT_EXCEEDED                       0x11

/**
 * \~English
 * Persistent data is encrypted, load failed.
 */
#define HIVEERR_ENCRYPTED_PERSISTENT_DATA            0x12

/**
 * \~English
 * Invalid bootstrap host.
 */
#define HIVEERR_BAD_BOOTSTRAP_HOST                   0x13

/**
 * \~English
 * Invalid bootstrap port.
 */
#define HIVEERR_BAD_BOOTSTRAP_PORT                   0x14

/**
 * \~English
 * Invalid address.
 */
#define HIVEERR_BAD_ADDRESS                          0x15

/**
 * \~English
 * Bad json format.
 */
#define HIVEERR_BAD_JSON_FORMAT                      0x16

/**
 * \~English
 * Try to call this api again.
 */
#define HIVEERR_TRY_AGAIN                            0x17

/**
 * \~English
 * Unknown error.
 */
#define HIVEERR_UNKNOWN                              0xFF

#define HIVE_MK_ERROR(facility, code)  (0x80000000 | ((facility) << 24) | \
                    ((((code) & 0x80000000) >> 8) | ((code) & 0x7FFFFFFF)))

#define HIVE_GENERAL_ERROR(code)       HIVE_MK_ERROR(HIVEF_GENERAL, code)
#define HIVE_SYS_ERROR(code)           HIVE_MK_ERROR(HIVEF_SYS, code)
#define HIVE_CURL_ERROR(code)          HIVE_MK_ERROR(HIVEF_CURL, code)
#define HIVE_CURLU_ERROR(code)         HIVE_MK_ERROR(HIVEF_CURLU, code)
#define HIVE_HTTP_STATUS_ERROR(code)   HIVE_MK_ERROR(HIVEF_HTTP_STATUS, code)

/**
 * \~English
 * Retrieves the last-error code value. The last-error code is maintained on a
 * per-instance basis. Multiple instance do not overwrite each other's
 * last-error code.
 *
 * @return
 *      The return value is the last-error code.
 */
HIVE_API
int hive_get_error(void);

/**
 * \~English
 * Clear the last-error code of a Carrier instance.
 */
HIVE_API
void hive_clear_error(void);

/**
 * \~English
 * Get string description to error code.
 */
HIVE_API
char *hive_get_strerror(int errnum, char *buf, size_t len);

#ifdef __cplusplus
} // extern "C"
#endif

#if defined(__APPLE__)
#pragma GCC diagnostic pop
#endif

#endif // __ELA_HIVE_H__
