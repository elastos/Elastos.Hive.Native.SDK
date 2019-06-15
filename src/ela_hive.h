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

/******************************************************************************
 * Generic Constants
 *****************************************************************************/

#define HIVE_MAX_IP_STRING_LEN 45

/******************************************************************************
 * Client APIs
 *****************************************************************************/

typedef struct HiveClient   HiveClient;
typedef struct HiveDrive    HiveDrive;
typedef struct HiveFile     HiveFile;

/**
 * \~English
 * Various Hive supporting backends.
 */
enum HiveDriveType {
    /**
     * \~English
     * Native file system.
     */
    HiveDriveType_Native    = 0x0,

    /**
     * \~English
     * IPFS.
     */
    HiveDriveType_IPFS      = 0x01,

    /**
     * \~English
     * OneDrive.
     */
    HiveDriveType_OneDrive  = 0x10,

    /**
     * \~English
     * OwnCloud(not implemented).
     */
    HiveDriveType_ownCloud  = 0x51,

    /**
     * \~English
     * Drive type buttom(not a valid type).
     */
    HiveDriveType_Butt      = 0x99
};

/**
 * \~English
 * User ID max length.
 */
#define HIVE_MAX_USER_ID_LEN            255

/**
 * \~English
 * User name max length.
 */
#define HIVE_MAX_USER_NAME_LEN          63

/**
 * \~English
 * User phone number max length.
 */
#define HIVE_MAX_PHONE_LEN              31

/**
 * \~English
 * User email address max length.
 */
#define HIVE_MAX_EMAIL_LEN              127

/**
 * \~English
 * User region max length.
 */
#define HIVE_MAX_REGION_LEN             127

/**
 * \~English
 * The common part of options that all kinds of client would need.
 */
typedef struct HiveOptions {
    /**
     * \~English
     * The path to a directory where all client associating data stores.
     */
    char *persistent_location;
    /**
     * \~English
     * Specifies the backend type of the client.
     */
    int drive_type;
} HiveOptions;

/**
 * \~English
 * The OneDrive client options.
 */
typedef struct OneDriveOptions {
    /**
     * \~English
     * Common options.
     */
    HiveOptions base;

    /**
     * \~English
     * The Client Id OneDrive assigns to the client.
     */
    const char *client_id;
    /**
     * \~English
     * The scope the client acquires from the delegated user. Drive and
     * file reading apis require corresponding read permissions; Drive and
     * file writing apis require corresponding write permissions;
     * hive_client_get_info() requires User.Read scope.
     */
    const char *scope;
    /**
     * \~English
     * The redirect URL through which the client gets the authorization code.
     */
    const char *redirect_url;
} OneDriveOptions;

typedef struct HiveRpcNode {
    /**
     * \~English
     * The ip address supported with ipv4 protocol.
     */
    const char *ipv4;

    /**
     * \~English
     * The ip address supported with ipv6 protocol.
     */
    const char *ipv6;

    /**
     * \~English
     * The ip port.
     * The default value is 9094.
     */
    const char *port;
} HiveRpcNode;

typedef struct IPFSOptions {
    /**
     * \~English
     * Common options.
     */
    HiveOptions base;

    /**
     * The unique uid for current user.
     */
    const char *uid;

    /**
     * The count of IPFS rpc addresses.
     */
    size_t rpc_node_count;

    /**
     * The array of rpc address of hive IPFS nodes.
     */
    HiveRpcNode *rpcNodes;
} IPFSOptions;

/**
 * \~English
 * A structure representing the hive client associated user's information.
 */
typedef struct HiveClientInfo {
    /**
     * \~English
     * User Id.
     */
    char user_id[HIVE_MAX_USER_ID_LEN+1];

    /**
     * \~English
     * User's display name.
     */
    char display_name[HIVE_MAX_USER_NAME_LEN+1];

    /**
     * \~English
     * User's email address.
     */
    char email[HIVE_MAX_EMAIL_LEN+1];

    /**
     * \~English
     * User's phone number.
     */
    char phone_number[HIVE_MAX_PHONE_LEN+1];

    /**
     * \~English
     * User's region.
     */
    char region[HIVE_MAX_REGION_LEN+1];
} HiveClientInfo;

/**
 * \~English
 * Drive ID max length.
 */
#define HIVE_MAX_DRIVE_ID_LEN            255

/**
 * \~English
 * A structure representing the hive drive information.
 */
typedef struct HiveDriveInfo {
    /**
     * \~English
     * Drive Id.
     */
    char driveid[HIVE_MAX_DRIVE_ID_LEN+1];
} HiveDriveInfo;

typedef struct HiveFileInfo {
    char fileid[128];
} HiveFileInfo;

/**
 * \~English
 * Create a new hive client instance to the specific backend.
 *
 * If the data is stored under @options.peristent_location, the client
 * instance would be created base on it. Otherwise, a new client instance
 * would be created and returned without any user context.
 *
 * All other hive APIs should be called after having client instance.
 *
 * @param
 *      options     [in] A pointer to a valid Options structure of
 *                       specific backend.
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
 * Close the hive client instance.
 *
 * After closing client instance, all derived instances of this client,
 * such as drive and file instances would become invalid. And any calling
 * APIs with invalid derive and file instance would be undefined.
 *
 * All derived instances such drive and files should be closed before
 * calling this API.
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

/**
 * \~English
 * Associate a user with the @client. During the process, the user
 * delegates to the @client.
 *
 * This function is effective only when no user is associated with the
 * @client.
 *
 * @param
 *      client      [in] A handle identifying the Hive client instance.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_client_login(HiveClient *client);

/**
 * \~English
 * Dissociate the user from the @client. All client's data in persistent
 * location is deleted. If a drive had been constructed out of the client,
 * it would also be closed. The drive handle becomes invalid after the call,
 * no functions should be called upon the drive handle.
 *
 * This function is effective only when a user is associated with the
 * @client.
 *
 * @param
 *      client      [in] A handle identifying the Hive client instance.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_client_logout(HiveClient *client);

/**
 * \~English
 * Get @client associated user's information. The result is filled into
 * @result.
 *
 * This function is effective only when a user is associated with the
 * @client.
 *
 * @param
 *      client      [in] A handle identifying the Hive client instance.
 * @param
 *      client_info [in] The HiveClientInfo pointer to receive the client
 *                       information.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_client_get_info(HiveClient *client, HiveClientInfo *client_info);

/******************************************************************************
 * Drive APIs
 *****************************************************************************/
/**
 * \~English
 *
 * Create a new Hive drive instance representing the default drive of
 * specific client.
 *
 * This function is effective only after the client has logined with
 * authentication. Caller is reponsible for close this drive instance
 * to reclaim resources when drive instance would not be used anymore.
 *
 * @param
 *      client      [in] A handle identifying the Hive client instance.
 *
 * @return
 *      If no error occurs, return the pointer of Hive drive instance.
 *      Otherwise, return NULL, and a specific error code can be
 *      retrieved by calling hive_get_error().
 */
HIVE_API
HiveDrive *hive_drive_open(HiveClient *client);

/**
 * \~English
 * Close the hive drive instance to destroy all associated resources.
 *
 * As long as the drive instance is closed, it becomes invalid. And calling
 * any function related invalid drive instance would be undefined.
 *
 * @param
 *      drive      [in] A handle identifying the Hive drive instance.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_drive_close(HiveDrive *drive);

/**
 * \~English
 * Get @drive's information. The result is filled into @result.
 *
 * This function is effective only when a user is associated with the
 * client generating the @drive.
 *
 * @param
 *      drive       [in] A handle identifying the Hive drive instance.
 * @param
 *      drive_info  [in] The HiveDriveInfo pointer to receive the drive
 *                       information.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_drive_get_info(HiveDrive *drive, HiveDriveInfo *drive_info);

/**
 * \~English
 * List files under the directory specified by @dir_path in @drive. The
 * result is a json string(not necessarily null-terminated) passed to @result.
 *
 * This function is effective only when a user is associated with the
 * client generating the @drive.
 *
 * @param
 *      drive      [in] A handle identifying the Hive client instance.
 * @param
 *      dir_path   [in] The absolute path to a directory in @drive.
 * @param
 *      result     [out] After the call, *result points to the buffer
 *                       holding the result. Call free() to release
 *                       the buffer after use.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_drive_list_files(HiveDrive *drive, const char *dir_path, char **result);

/**
 * \~English
 * Create a directory specified by @path in @drive. The result is
 * a json string(not necessarily null-terminated) passed to @result.
 *
 * This function is effective only when a user is associated with the
 * client generating the @drive.
 *
 * @param
 *      drive      [in] A handle identifying the Hive client instance.
 * @param
 *      path       [in] The absolute path to a directory in @drive.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_drive_mkdir(HiveDrive *drive, const char *path);

/**
 * \~English
 * Move the file specified by @old to the new path @new in @drive.
 *
 * This function is effective only when a user is associated with the
 * client generating the @drive.
 *
 * @param
 *      drive      [in] A handle identifying the Hive client instance.
 * @param
 *      old        [in] The absolute path of file that would be moved.
 * @param
 *      new        [in] The absolute path that a file would copy to.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_drive_move_file(HiveDrive *drive, const char *old, const char *new);

/**
 * \~English
 * Copy the file specified by @src_path to path specified by @dest_path in @drive.
 *
 * This function is effective only when a user is associated with the
 * client generating the @drive.
 *
 * @param
 *      drive      [in] A handle identifying the Hive client instance.
 * @param
 *      src        [in] The absolute path of file that would be copied.
 * @param
 *      dest       [in] The absolute path that a file would copy to.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_drive_copy_file(HiveDrive *drive, const char *src, const char *dest);

/**
 * \~English
 * Delete the file specified by @path in @drive.
 *
 * This function is effective only when a user is associated with the
 * client generating the @drive.
 *
 * @param
 *      drive      [in] A handle identifying the Hive client instance.
 * @param
 *      path       [in] The absolute path to a file in @drive.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_drive_delete_file(HiveDrive *drive, const char *path);

/**
 * \~English
 * Get status of the file specified by @file_path in @drive. The result is
 * a json string(not necessarily null-terminated) passed to @result.
 *
 * This function is effective only when a user is associated with the
 * client generating the @drive.
 *
 * @param
 *      drive      [in] A handle identifying the Hive client instance.
 * @param
 *      path       [in] The absolute path to a file in @drive.
 * @param
 *      file_info  [in] The HiveFileInfo pointer to receive the drive
 *                      information.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_drive_file_stat(HiveDrive *drive, const char *path,
                         HiveFileInfo *file_info);


HIVE_API
HiveFile *hive_file_open(HiveDrive *drive, const char *path, const char *mode);

HIVE_API
int hive_file_close(HiveFile *file);

HIVE_API
int hive_file_get_info(HiveFile *file, HiveFileInfo *file_info);

HIVE_API
char *hive_file_get_path(HiveFile *file, char *buf, size_t bufsz);

enum {
    HiveSeek_Set,
    HiveSeek_Cur,
    HiveSeek_End
};

HIVE_API
ssize_t hive_file_seek(HiveFile *, uint64_t offset, int whence);

HIVE_API
ssize_t hive_file_read(HiveFile *, char *buf, size_t bufsz);

HIVE_API
ssize_t hive_file_write(HiveFile *file, const char *buf, size_t bufsz);
/******************************************************************************
 * Error handling
 *****************************************************************************/

// Facility code
#define HIVEF_GENERAL                                0x01
#define HIVEF_SYS                                    0x02
#define HIVEF_RESERVED1                              0x03
#define HIVEF_RESERVED2                              0x04
#define HIVEF_HTTP_CLIENT                            0x05
#define HIVEF_HTTP_SERVER                            0x06

#define HIVESUCCESS                                  0

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
 * Control packet is invalid.
 */
#define HIVEERR_INVALID_CONTROL_PACKET               0x06

/**
 * \~English
 * Credential is invalid.
 */
#define HIVEERR_INVALID_CREDENTIAL                   0x07

/**
 * \~English
 * Carrier ran already.
 */
#define HIVEERR_ALREADY_RUN                          0x08

/**
 * \~English
 * Carrier not ready.
 */
#define HIVEERR_NOT_READY                            0x09

/**
 * \~English
 * The requested entity does not exist.
 */
#define HIVEERR_NOT_EXIST                            0x0A

/**
 * \~English
 * The entity exists already.
 */
#define HIVEERR_ALREADY_EXIST                        0x0B

/**
 * \~English
 * There are no matched requests.
 */
#define HIVEERR_NO_MATCHED_REQUEST                   0x0C

/**
 * \~English
 * User ID is invalid.
 */
#define HIVEERR_INVALID_USERID                       0x0D

/**
 * \~English
 * Node ID is invalid.
 */
#define HIVEERR_INVALID_NODEID                       0x0E

/**
 * \~English
 * Failed because wrong state.
 */
#define HIVEERR_WRONG_STATE                          0x0F

/**
 * \~English
 * Stream busy.
 */
#define HIVEERR_BUSY                                 0x10

/**
 * \~English
 * Language binding error.
 */
#define HIVEERR_LANGUAGE_BINDING                     0x11

/**
 * \~English
 * Encryption failed.
 */
#define HIVEERR_ENCRYPT                              0x12

/**
 * \~English
 * The content size of SDP is too long.
 */
#define HIVEERR_SDP_TOO_LONG                         0x13

/**
 * \~English
 * Bad SDP information format.
 */
#define HIVEERR_INVALID_SDP                          0x14

/**
 * \~English
 * Not implemented yet.
 */
#define HIVEERR_NOT_IMPLEMENTED                      0x15

/**
 * \~English
 * Limits are exceeded.
 */
#define HIVEERR_LIMIT_EXCEEDED                       0x16

/**
 * \~English
 * Allocate port unsuccessfully.
 */
#define HIVEERR_PORT_ALLOC                           0x17

/**
 * \~English
 * Invalid proxy type.
 */
#define HIVEERR_BAD_PROXY_TYPE                       0x18

/**
 * \~English
 * Invalid proxy host.
 */
#define HIVEERR_BAD_PROXY_HOST                       0x19

/**
 * \~English
 * Invalid proxy port.
 */
#define HIVEERR_BAD_PROXY_PORT                       0x1A

/**
 * \~English
 * Proxy is not available.
 */
#define HIVEERR_PROXY_NOT_AVAILABLE                  0x1B

/**
 * \~English
 * Persistent data is encrypted, load failed.
 */
#define HIVEERR_ENCRYPTED_PERSISTENT_DATA            0x1C

/**
 * \~English
 * Invalid bootstrap host.
 */
#define HIVEERR_BAD_BOOTSTRAP_HOST                   0x1D

/**
 * \~English
 * Invalid bootstrap port.
 */
#define HIVEERR_BAD_BOOTSTRAP_PORT                   0x1E

/**
 * \~English
 * Data is too long.
 */
#define HIVEERR_TOO_LONG                             0x1F

/**
 * \~English
 * Could not friend yourself.
 */
#define HIVEERR_ADD_SELF                             0x20

/**
 * \~English
 * Invalid address.
 */
#define HIVEERR_BAD_ADDRESS                          0x21

/**
 * \~English
 * Friend is offline.
 */
#define HIVEERR_FRIEND_OFFLINE                       0x22

/**
 * \~English
 * Unknown error.
 */
#define HIVEERR_UNKNOWN                              0xFF


#define HIVE_MK_ERROR(facility, code)  (0x80000000 | ((facility) << 24) | \
                    ((((code) & 0x80000000) >> 8) | ((code) & 0x7FFFFFFF)))

#define HIVE_GENERAL_ERROR(code)       HIVE_MK_ERROR(HIVEF_GENERAL, code)
#define HIVE_SYS_ERROR(code)           HIVE_MK_ERROR(HIVEF_SYS, code)
#define HIVE_HTTPC_ERROR(code)         HIVE_MK_ERROR(HIVEF_HTTP_CLIENT, code)
#define HIVE_HTTPS_ERROR(code)         HIVE_MK_ERROR(HIVEF_HTTP_SERVER, code)
#define HIVE_HTTP_TSX_ERROR(code)      HIVE_MK_ERROR(HIVEF_HTTP_TRANSACTION, code)

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
