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

typedef struct HiveClient HiveClient;
typedef struct HiveConnect HiveConnect;

/**
 * \~English
 * Various backends supported.
 */
enum HiveBackendType {
    /**
     * \~English
     * IPFS.
     */
    HiveBackendType_IPFS      = 0x01,

    /**
     * \~English
     * OneDrive.
     */
    HiveBackendType_OneDrive  = 0x10,

    /**
     * \~English
     * OwnCloud(not implemented).
     */
    HiveBackendType_ownCloud  = 0x51,

    /**
     * \~English
     * End of backend types(not a valid type).
     */
    HiveBackendType_Butt        = 0x99
};

/**
 * \~English
 * Max single value length in a key:value(s) pair.
 */
#define HIVE_MAX_VALUE_LEN (32U * 1024)

/**
 * \~English
 * Max file size to be stored in the backend.
 */
#define HIVE_MAX_FILE_SIZE (32U * 1024 * 1024)

/**
 * \~English
 * Max IPFS CID length.
 */
#define HIVE_MAX_IPFS_CID_LEN (64)

/******************************************************************************
 * Log configuration.
 *****************************************************************************/

/**
 * \~English
 * Hive log level to control or filter log output.
 */
typedef enum HiveLogLevel {
    /**
     * \~English
     * Log level None
     * Indicate disable log output.
     */
    HiveLogLevel_None = 0,
    /**
     * \~English
     * Log level fatal.
     * Indicate output log with level 'Fatal' only.
     */
    HiveLogLevel_Fatal = 1,
    /**
     * \~English
     * Log level error.
     * Indicate output log above 'Error' level.
     */
    HiveLogLevel_Error = 2,
    /**
     * \~English
     * Log level warning.
     * Indicate output log above 'Warning' level.
     */
    HiveLogLevel_Warning = 3,
    /**
     * \~English
     * Log level info.
     * Indicate output log above 'Info' level.
     */
    HiveLogLevel_Info = 4,
    /**
     * \~English
     * Log level debug.
     * Indicate output log above 'Debug' level.
     */
    HiveLogLevel_Debug = 5,
    /**
     * \~English
     * Log level trace.
     * Indicate output log above 'Trace' level.
     */
    HiveLogLevel_Trace = 6,
    /**
     * \~English
     * Log level verbose.
     * Indicate output log above 'Verbose' level.
     */
    HiveLogLevel_Verbose = 7
} HiveLogLevel;

/**
 * \~English
 * Initialize log options for Hive.
 * If this API is never called, the default log level is 'Info'; The default log
 * file is stdout.
 *
 * @param
 *      level       [in] The log level to control internal log output.
 * @param
 *      log_file    [in] the log file name.
 *                       If the log_file is NULL, Hive will not write
 *                       log to file.
 * @param
 *      log_printer [in] the user defined log printer. Can be NULL.
 */
HIVE_API
void hive_log_init(HiveLogLevel level, const char *log_file,
                   void (*log_printer)(const char *format, va_list args));

/******************************************************************************
 * Type definitions of all options.
 *****************************************************************************/

/**
 * \~English
 * Global options.
 */
typedef struct HiveOptions {
    /**
     * \~English
     * The path to a directory where persistent data stores.
     */
    char *data_location;
} HiveOptions;

/**
 * \~English
 * Common part of options that all types of connections share.
 */
typedef struct HiveConnectOptions {
    /**
     * \~English
     * Specifies the backend type of the connection.
     */
    int backendType;
} HiveConnectOptions;

/**
 * \~English
 * User defined function to open authentication URL during Oauth2 authorization process.
 *
 * @param
 *      url         [in] Authentication URL to be opened.
 * @param
 *      context     [in] User data.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1.
 */
typedef int HiveRequestAuthenticationCallback(const char *url, void *context);

/**
 * \~English
 * The OneDrive connection options.
 */
typedef struct OneDriveConnectOptions {
    /**
     * \~English
     * Specifies the backend type of the connection.
     */
    int backendType;

    /**
     * \~English
     * The Client ID OneDrive assigns to the app.
     */
    const char *client_id;

    /**
     * \~English
     * The scope the app acquires from the delegated user.
     * Hive APIs require Files.ReadWrite.AppFolder scope.
     */
    const char *scope;

    /**
     * \~English
     * The redirect URL where the client gets the authorization code.
     */
    const char *redirect_url;

    /**
     * \~English
     * Callback to open Oauth2 authorization URL.
     */
    HiveRequestAuthenticationCallback *callback;

    /**
     * \~English
     * User data to be passed as context parameter to callback.
     */
    void *context;
} OneDriveConnectOptions;

/**
 * \~English
 * The IPFS node.
 */
typedef struct IPFSNode {
    /**
     * \~English
     * The ipv4 address.
     */
    const char *ipv4;

    /**
     * \~English
     * The ipv6 address.
     */
    const char *ipv6;

    /**
     * \~English
     * TCP service port.
     */
    const char *port;
} IPFSNode;

/**
 * \~English
 * The IPFS connection options.
 */
typedef struct IPFSConnectOptions {
    /**
     * \~English
     * Specifies the backend type of the connection.
     */
    int backendType;

    /**
     * \~English
     * The count of IPFS RPC nodes.
     */
    size_t rpc_node_count;

    /**
     * \~English
     * The array of addresses of IPFS RPC nodes.
     */
    IPFSNode *rpcNodes;
} IPFSConnectOptions;

/******************************************************************************
 * Client APIs
 *****************************************************************************/

/**
 * \~English
 * Create a Hive client instance where all connection instances are created.
 *
 * @param
 *      options     [in] A pointer to a valid Options structure.
 *
 * @return
 *      If no error occurs, return the pointer of Hive client instance.
 *      Otherwise, return NULL, and a specific error code can be
 *      retrieved by calling hive_get_error().
 */
HIVE_API
HiveClient *hive_client_new(const HiveOptions *options);

/**
 * \~English
 * Close the Hive client instance.
 *
 * @param
 *      client      [in] A handle identifying the Hive client instance.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_client_close(HiveClient *client);

/******************************************************************************
 * Connection APIs
 *****************************************************************************/

/**
 * \~English
 * Connect to a specific backend. Credentials are loaded from persistent location
 * configured to the client instance passed.
 *
 * @param
 *      client      [in] A client instance.
 * @param
 *      options     [in] A pointer to a valid Options structure of the backend.
 *
 * @return
 *      If no error occurs, return the pointer of Hive connect instance.
 *      Otherwise, return NULL, and a specific error code can be
 *      retrieved by calling hive_get_error().
 */
HIVE_API
HiveConnect *hive_client_connect(HiveClient *client, HiveConnectOptions *options);

/**
 * \~English
 * Disconnect from the backend.
 *
 * @param
 *      connect     [in] A connect instance.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_client_disconnect(HiveConnect *connect);

/**
 * \~English
 * Set the encrypt key.
 *
 * @param
 *      connect     [in] A connect instance.
 * @param
 *      encrypt_key [in] Encrypt key.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_set_encrypt_key(HiveConnect *connect, const char *encrypt_key);

/**
 * \~English
 * Upload a file to the backend.
 *
 * @param
 *      connect     [in] A connect instance.
 * @param
 *      from        [in] Source file path.
 * @param
 *      encrypt     [in] Whether to encrypt the file.
 * @param
 *      filename    [in] Destination file name.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_put_file(HiveConnect *connect, const char *from, bool encrypt, const char *filename);

/**
 * \~English
 * Upload buffer content to a file in the backend.
 *
 * @param
 *      connect     [in] A connect instance.
 * @param
 *      from        [in] A pointer to buffer.
 * @param
 *      length      [in] Length of the buffer.
 * @param
 *      encrypt     [in] Whether to encrypt the buffer content.
 * @param
 *      filename    [in] Destination file name.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_put_file_from_buffer(HiveConnect *connect, const void *from, size_t length, bool encrypt, const char *filename);

/**
 * \~English
 * Get the length of a file in the backend.
 *
 * @param
 *      connect    [in] A connect instance.
 * @param
 *      filename   [in] File name.
 *
 * @return
 *      If no error occurs, return the length of the file. Otherwise, return -1, and a
 *      specific error code can be retrieved by calling hive_get_error().
 */
HIVE_API
ssize_t hive_get_file_length(HiveConnect *connect, const char *filename);

/**
 * \~English
 * Download a file in the backend to buffer.
 *
 * @param
 *      connect    [in] A connect instance.
 * @param
 *      filename   [in] File name.
 * @param
 *      decrypt    [in] Whether to decrypt file content.
 * @param
 *      to         [in] A pointer to buffer.
 * @param
 *      buflen     [in] Length of the buffer.
 *
 * @return
 *      If no error occurs, return the length of the file. Otherwise, return -1,
 *      and a specific error code can be retrieved by calling hive_get_error().
 */
HIVE_API
ssize_t hive_get_file_to_buffer(HiveConnect *connect, const char *filename, bool decrypt, void *to, size_t buflen);

/**
 * \~English
 * Download a file in the backend.
 *
 * @param
 *      connect    [in] A connect instance.
 * @param
 *      filename   [in] Source file name.
 * @param
 *      decrypt    [in] Whether to decrypt file content.
 * @param
 *      to         [in] Destination file path.
 *
 * @return
 *      If no error occurs, return the length of the file. Otherwise, return -1,
 *      and a specific error code can be retrieved by calling hive_get_error().
 */
HIVE_API
ssize_t hive_get_file(HiveConnect *connect, const char *filename, bool decrypt, const char *to);

/**
 * \~English
 * Delete a file from the backend.
 *
 * @param
 *      connect    [in] A connect instance.
 * @param
 *      filename   [in] file name.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_delete_file(HiveConnect *connect, const char *filename);

/**
 * \~English
 * An application-defined function that iterate the each file in the backend.
 *
 * @param
 *      filename    [in] File name.
 * @param
 *      context     [in] The application-defined context data.
 *
 * @return
 *      Return true to continue iteration, false to abort from iteration
 *      immediately.
 */
typedef bool HiveFilesIterateCallback(const char *filename, void *context);

/**
 * \~English
 * List all files in the backend.
 *
 * @param
 *      connect    [in] A connect instance.
 * @param
 *      callback   [in] An application-defined function to iterate each file.
 * @param
 *      context    [in] The application defined context data.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_list_files(HiveConnect *connect, HiveFilesIterateCallback *callback, void *context);

/**
 * \~English
 * IPFS CID type.
 */
typedef struct IPFSCid {
    /**
     * \~English
     * CID string.
     */
    char content[HIVE_MAX_IPFS_CID_LEN + 1];
} IPFSCid;

/**
 * \~English
 * Upload a file to IPFS.
 *
 * @param
 *      connect     [in] A connect instance.
 * @param
 *      from        [in] Source file path.
 * @param
 *      encrypt     [in] Whether to encrypt file content.
 * @param
 *      cid         [in] CID of uploaded file.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_ipfs_put_file(HiveConnect *connect, const char *from, bool encrypt, IPFSCid *cid);

/**
 * \~English
 * Upload buffer content to IPFS.
 *
 * @param
 *      connect     [in] A connect instance.
 * @param
 *      from        [in] A pointer to buffer.
 * @param
 *      length      [in] Length of the buffer.
 * @param
 *      encrypt     [in] Whether to encrypt buffer content.
 * @param
 *      cid         [in] CID of uploaded content.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_ipfs_put_file_from_buffer(HiveConnect *connect, const void *from, size_t length, bool encrypt, IPFSCid *cid);

/**
 * \~English
 * Get the length of a file in IPFS.
 *
 * @param
 *      connect    [in] A connect instance.
 * @param
 *      cid        [in] CID of the file.
 *
 * @return
 *      If no error occurs, return the length of the file. Otherwise, return -1, and a
 *      specific error code can be retrieved by calling hive_get_error().
 */
HIVE_API
ssize_t hive_ipfs_get_file_length(HiveConnect *connect, const IPFSCid *cid);

/**
 * \~English
 * Download a file in IPFS to buffer.
 *
 * @param
 *      connect    [in] A connect instance.
 * @param
 *      cid        [in] CID of the file.
 * @param
 *      decrypt    [in] Whether to decrypt file content.
 * @param
 *      to         [in] A pointer to buffer.
 * @param
 *      buflen     [in] Length of the buffer.
 *
 * @return
 *      If no error occurs, return the length of the file. Otherwise, return -1,
 *      and a specific error code can be retrieved by calling hive_get_error().
 */
HIVE_API
ssize_t hive_ipfs_get_file_to_buffer(HiveConnect *connect, const IPFSCid *cid,  bool decrypt, void *to, size_t bulen);

/**
 * \~English
 * Download a file in IPFS.
 *
 * @param
 *      connect    [in] A connect instance.
 * @param
 *      cid        [in] CID of the file.
 * @param
 *      decrypt    [in] Whether to decrypt file content.
 * @param
 *      to         [in] Destination file path.
 *
 * @return
 *      If no error occurs, return the length of the file. Otherwise, return -1,
 *      and a specific error code can be retrieved by calling hive_get_error().
 */
HIVE_API
ssize_t hive_ipfs_get_file(HiveConnect *connect, const IPFSCid *cid, bool decrypt, const char *to);

/**
 * \~English
 * Append value to the specified key.
 *
 * @param
 *      connect    [in] A connect instance.
 * @param
 *      key        [in] Key.
 * @param
 *      value      [in] Value.
 * @param
 *      length     [in] Value length.
 * @param
 *      encrypt    [in] Whether to encrypt value.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_put_value(HiveConnect *connect, const char *key, const void *value, size_t length, bool encrypt);

/**
 * \~English
 * Overwrite value of the specified key with specified value.
 *
 * @param
 *      connect    [in] A connect instance.
 * @param
 *      key        [in] Key.
 * @param
 *      value      [in] Value.
 * @param
 *      length     [in] Value length.
 * @param
 *      encrypt    [in] Whether to encrypt value.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_set_value(HiveConnect *connect, const char *key, const void *value, size_t length, bool encrypt);

/**
 * \~English
 * An application-defined function that iterate the each value of a specific key.
 *
 * @param
 *      key         [in] Key.
 * @param
 *      value       [in] Value.
 * @param
 *      length      [in] Value length.
 * @param
 *      context     [in] The application-defined context data.
 *
 * @return
 *      Return true to continue iteration, false to abort from iteration
 *      immediately.
 */
typedef bool HiveKeyValuesIterateCallback(const char *key, const void *value, size_t length, void *context);

/**
 * \~English
 * Get all values of a key.
 *
 * @param
 *      connect    [in] A connect instance.
 * @param
 *      key        [in] Key.
 * @param
 *      decrypt    [in] Whether to decrypt value.
 * @param
 *      callback   [in] An application-defined function that iterate the each value.
 * @param
 *      context    [in] The application-defined context data.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_get_values(HiveConnect *connect, const char *key, bool decrypt, HiveKeyValuesIterateCallback *callback, void *context);

/**
 * \~English
 * Delete key:value(s) pair.
 *
 * @param
 *      connect    [in] A connect instance.
 * @param
 *      key        [in] The key to be deleted.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
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
