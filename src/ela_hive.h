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

typedef struct HiveClient       HiveClient;
typedef struct HiveClientInfo   HiveClientInfo;
typedef struct HiveDrive        HiveDrive;
typedef struct HiveDriveInfo    HiveDriveInfo;

struct HiveOAuthInfo {
    const char *client_id;
    const char *scope;
    const char *redirect_url;
};

enum HiveDriveType {
    HiveDriveType_Native    = 0x0,
    HiveDriveType_IPFS      = 0x01,

    HiveDriveType_OneDrive  = 0x10,
    HiveDriveType_ownCloud  = 0x51,
    HiveDriveType_Butt      = 0x99
};

typedef struct HiveOptions {
    char *persistent_location;
    int  drive_type;
} HiveOptions;

typedef struct OneDriveOptions {
    HiveOptions base;

    const char *client_id;
    const char *scope;
    const char *redirect_url;

    int (*grant_authorize)(const char *request_url);
} OneDriveOptions;

typedef struct IPFSOptions {
    HiveOptions base;

    char *uid;
    size_t bootstraps_size;
    const char *bootstraps_ip[0];
} IPFSOptions;

/**
* \~English
 * Create a new hive client instance to the specific drive.
 * All other hive APIs should be called after having client instance.
 *
 * @param
 *      options     [in] A pointer to a valid HiveOptions structure.
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
 * Destroy all associated resources with the Hive client instance.
 *
 * After calling the function, the client pointer becomes invalid.
 * No other functions should be called.
 *
 * @param
 *      client      [in] A handle identifying the Hive client instance.
 */
HIVE_API
int hive_client_close(HiveClient *client);

HIVE_API
int hive_client_login(HiveClient *client);

HIVE_API
int hive_client_logout(HiveClient *client);

HIVE_API
int hive_client_get_info(HiveClient *client, char **result);

HIVE_API
HiveDrive *hive_drive_open(HiveClient *client);

HIVE_API
int hive_drive_close(HiveDrive *client);

HIVE_API
int hive_drive_get_info(HiveDrive *, char **result);

HIVE_API
int hive_drive_file_stat(HiveDrive *, const char *file_path, char **result);

HIVE_API
int hive_drive_list_files(HiveDrive *, const char *dir_path, char **result);

HIVE_API
int hive_drive_mkdir(HiveDrive *, const char *path);

HIVE_API
int hive_drive_move_file(HiveDrive *, const char *old, const char *new);

HIVE_API
int hive_drive_copy_file(HiveDrive *, const char *src_path, const char *dest_path);

HIVE_API
int hive_drive_delete_file(HiveDrive *, const char *path);

/******************************************************************************
 * Error handling
 *****************************************************************************/

// Facility code
#define ELAF_GENERAL                                0x01
#define ELAF_SYS                                    0x02
#define ELAF_RESERVED1                              0x03
#define ELAF_RESERVED2                              0x04
#define ELAF_HTTP_CLIENT                            0x05
#define ELAF_HTTP_SERVER                            0x06

#define ELASUCCESS                                  0

/**
 * \~English
 * Argument(s) is(are) invalid.
 */
#define ELAERR_INVALID_ARGS                         0x01

/**
 * \~English
 * Runs out of memory.
 */
#define ELAERR_OUT_OF_MEMORY                        0x02

/**
 * \~English
 * Buffer size is too small.
 */
#define ELAERR_BUFFER_TOO_SMALL                     0x03

/**
 * \~English
 * Persistent data is corrupted.
 */
#define ELAERR_BAD_PERSISTENT_DATA                  0x04

/**
 * \~English
 * Persistent file is invalid.
 */
#define ELAERR_INVALID_PERSISTENCE_FILE             0x05

/**
 * \~English
 * Control packet is invalid.
 */
#define ELAERR_INVALID_CONTROL_PACKET               0x06

/**
 * \~English
 * Credential is invalid.
 */
#define ELAERR_INVALID_CREDENTIAL                   0x07

/**
 * \~English
 * Carrier ran already.
 */
#define ELAERR_ALREADY_RUN                          0x08

/**
 * \~English
 * Carrier not ready.
 */
#define ELAERR_NOT_READY                            0x09

/**
 * \~English
 * The requested entity does not exist.
 */
#define ELAERR_NOT_EXIST                            0x0A

/**
 * \~English
 * The entity exists already.
 */
#define ELAERR_ALREADY_EXIST                        0x0B

/**
 * \~English
 * There are no matched requests.
 */
#define ELAERR_NO_MATCHED_REQUEST                   0x0C

/**
 * \~English
 * User ID is invalid.
 */
#define ELAERR_INVALID_USERID                       0x0D

/**
 * \~English
 * Node ID is invalid.
 */
#define ELAERR_INVALID_NODEID                       0x0E

/**
 * \~English
 * Failed because wrong state.
 */
#define ELAERR_WRONG_STATE                          0x0F

/**
 * \~English
 * Stream busy.
 */
#define ELAERR_BUSY                                 0x10

/**
 * \~English
 * Language binding error.
 */
#define ELAERR_LANGUAGE_BINDING                     0x11

/**
 * \~English
 * Encryption failed.
 */
#define ELAERR_ENCRYPT                              0x12

/**
 * \~English
 * The content size of SDP is too long.
 */
#define ELAERR_SDP_TOO_LONG                         0x13

/**
 * \~English
 * Bad SDP information format.
 */
#define ELAERR_INVALID_SDP                          0x14

/**
 * \~English
 * Not implemented yet.
 */
#define ELAERR_NOT_IMPLEMENTED                      0x15

/**
 * \~English
 * Limits are exceeded.
 */
#define ELAERR_LIMIT_EXCEEDED                       0x16

/**
 * \~English
 * Allocate port unsuccessfully.
 */
#define ELAERR_PORT_ALLOC                           0x17

/**
 * \~English
 * Invalid proxy type.
 */
#define ELAERR_BAD_PROXY_TYPE                       0x18

/**
 * \~English
 * Invalid proxy host.
 */
#define ELAERR_BAD_PROXY_HOST                       0x19

/**
 * \~English
 * Invalid proxy port.
 */
#define ELAERR_BAD_PROXY_PORT                       0x1A

/**
 * \~English
 * Proxy is not available.
 */
#define ELAERR_PROXY_NOT_AVAILABLE                  0x1B

/**
 * \~English
 * Persistent data is encrypted, load failed.
 */
#define ELAERR_ENCRYPTED_PERSISTENT_DATA            0x1C

/**
 * \~English
 * Invalid bootstrap host.
 */
#define ELAERR_BAD_BOOTSTRAP_HOST                   0x1D

/**
 * \~English
 * Invalid bootstrap port.
 */
#define ELAERR_BAD_BOOTSTRAP_PORT                   0x1E

/**
 * \~English
 * Data is too long.
 */
#define ELAERR_TOO_LONG                             0x1F

/**
 * \~English
 * Could not friend yourself.
 */
#define ELAERR_ADD_SELF                             0x20

/**
 * \~English
 * Invalid address.
 */
#define ELAERR_BAD_ADDRESS                          0x21

/**
 * \~English
 * Friend is offline.
 */
#define ELAERR_FRIEND_OFFLINE                       0x22

/**
 * \~English
 * Unknown error.
 */
#define ELAERR_UNKNOWN                              0xFF

/*
 * \~English
 * Retrieves the last-error code value. The last-error code is maintained on a
 * per-instance basis. Multiple instance do not overwrite each other's
 * last-error code.
 *
 * @return
 *      The return value is the last-error code.
 */
HIVE_API
int ela_get_error(void);

/**
 * \~English
 * Clear the last-error code of a Carrier instance.
 */
HIVE_API
void ela_clear_error(void);

/**
 * \~English
 * Get string description to error code.
 */
HIVE_API
char *ela_get_strerror(int errnum, char *buf, size_t len);

#ifdef __cplusplus
} // extern "C"
#endif

#if defined(__APPLE__)
#pragma GCC diagnostic pop
#endif

#endif // __ELA_HIVE_H__
