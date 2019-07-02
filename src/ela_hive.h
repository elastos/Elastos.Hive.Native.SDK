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
     * Drive type bottom(not a valid type).
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
 * Drive ID max length.
 */
#define HIVE_MAX_DRIVE_ID_LEN           255

/**
 * \~English
 * Drive File Id max length.
 */
#define HIVE_MAX_FILE_ID_LEN            127

/**
 * \~English
 * Drive File type max length.
 */
#define HIVE_MAX_FILE_TYPE_LEN          127

/******************************************************************************
 * Type definitions of all options.
 *****************************************************************************/

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

/**
 * \~English
 * The Hive RPC node.
 */
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

/**
 * \~English
 * The IPFS client options.
 */
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
     * The count of IPFS RPC nodes.
     */
    size_t rpc_node_count;

    /**
     * The array of addresses of IPFS RPC nodes.
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
 * A structure representing the hive drive information.
 */
typedef struct HiveDriveInfo {
    /**
     * \~English
     * Drive Id.
     */
    char driveid[HIVE_MAX_DRIVE_ID_LEN+1];
} HiveDriveInfo;

/**
 * \~English
 * A structure representing the hive file information.
 */
typedef struct HiveFileInfo {
    /**
     * \~English
     * File Id
     */
    char fileid[HIVE_MAX_FILE_ID_LEN + 1];
    char type[HIVE_MAX_FILE_TYPE_LEN + 1];
    size_t size;
} HiveFileInfo;

/******************************************************************************
 * Client APIs
 *****************************************************************************/

/**
 * \~English
 * Create a new hive client instance to the specific backend.
 *
 * If the data is stored under @options.peristent_location, the client
 * instance would be created base on it. Otherwise, a new client instance
 * would be created and returned without any user context. On sucessfully
 * returning, the state of client instance is "raw".
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
 * APIs with invalid drive and file instance would be undefined.
 *
 * All derived instances such as drive and files should be closed before
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
 * User defined function to open authentication URL during login process.
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
 * Associate a user with the @client. During the process, the user
 * delegates to the @client. On successfully returning, the state of @client
 * becomes "logined".
 *
 * This function is effective only when state of @client is "raw".
 *
 * @param
 *      client      [in] A handle identifying the Hive client instance.
 * @param
 *      callback    [in] Callback to open authentication URL during login process.
 * @param
 *      context     [in] User data to be passed as context parameter to @callback.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_client_login(HiveClient *client,
                      HiveRequestAuthenticationCallback *callback,
                      void *context);

/**
 * \~English
 * Dissociate the user from the @client. All client's data in persistent
 * location is deleted. All derived instances of this client, such as drive
 * and file instances would become invalid. And any calling APIs with invalid
 * drive and file instance would be undefined.
 *
 * This function is effective only when state of @client is "logined".
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
 * @client_info.
 *
 * This function is effective only when state of @client is "logined".
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
 * This function is effective only when state of @client is "logined". Caller is
 * responsible for closing this drive instance to reclaim resources when drive
 * instance would not be used anymore.
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
 * Get @drive's information. The result is filled into @drive_info.
 *
 * This function is effective only when state of client generating the @drive is
 * "logined".
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
 * A structure aimed to describing one of the properties of a file.
 */
typedef struct KeyValue {
    /**
     * \~English
     * Denotes the property's name.
     */
    char *key;

    /**
     * \~English
     * Denotes the property's value.
     */
    char *value;
} KeyValue;

/**
 * \~English
 * An application-defined function that iterate the each file info item.
 *
 * ElaFriendsIterateCallback is the callback function type.
 *
 * @param
 *      info        [in] A pointer to an array of KeyValue carrying various
 *                       properties of the file. NULL means end of iteration.
 * @param
 *      size        [in] the size of the KeyValue array.
 * @param
 *      context     [in] The application-defined context data.
 *
 * @return
 *      Return true to continue iteration, false to abort from iteration
 *      immediately.
 */
typedef bool HiveFilesIterateCallback(const KeyValue *info, size_t size,
                                      void *context);

/**
 * \~English
 * Iterate file info under the specified directory path.
 *
 * Callback would be invoked to iterate information of each file after having
 * acquired the list of information under the directory from remote drive.
 *
 * This function is effective only when state of client generating @drive is
 * "logined".
 *
 * @param
 *      drive      [in] A handle identifying the Hive drive instance.
 * @param
 *      path       [in] The absolute path to a directory list its file infos.
 * @param
 *      callback   [in] An application-defined function to iterate each
 *                      file info under directory.
 * @param
 *      context    [in] The application defined context data.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_drive_list_files(HiveDrive *drive, const char *path,
                          HiveFilesIterateCallback *callback, void *context);

/**
 * \~English
 * Create a directory specified by @path in @drive.
 *
 * This function is effective only when state of client generating @drive is
 * "logined".
 *
 * @param
 *      drive      [in] A handle identifying the Hive drive instance.
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
 * This function is effective only when state of client generating @drive is
 * "logined".
 *
 * @param
 *      drive      [in] A handle identifying the Hive drive instance.
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
 * This function is effective only when state of client generating @drive is
 * "logined".
 *
 * @param
 *      drive      [in] A handle identifying the Hive drive instance.
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
 * This function is effective only when state of client generating @drive is
 * "logined".
 *
 * @param
 *      drive      [in] A handle identifying the Hive drive instance.
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
 * filled into @file_info.
 *
 * This function is effective only when state of client generating @drive is
 * "logined".
 *
 * @param
 *      drive      [in] A handle identifying the Hive drive instance.
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

/******************************************************************************
 * File APIs
 *****************************************************************************/
/**
 * \~English
 *
 * Open a file specified by @path under @drive.
 *
 * This function is effective only when state of client generating @drive is
 * "logined".
 *
 * @param
 *      drive      [in] A handle identifying the Hive drive instance.
 * @param
 *      path       [in] The absolute path to a file in @drive.
 * @param
 *      mode       [in] The operation mode of the file instance(same as type
 *                      parameter of fopen()).
 *
 * @return
 *      If no error occurs, return the file instance. Otherwise, return NULL,
 *      and a specific error code can be retrieved by calling hive_get_error().
 */
HIVE_API
HiveFile *hive_file_open(HiveDrive *drive, const char *path, const char *mode);

/**
 * \~English
 * Close the hive file instance to destroy all associated resources.
 *
 * As long as the file instance is closed, it becomes invalid. And calling
 * any function related invalid drive instance would be undefined.
 *
 * @param
 *      file      [in] A handle identifying the Hive file instance.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific
 *      error code can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_file_close(HiveFile *file);

/**
 * \~English
 * Enumeration of whence parameter of hive_file_seek().
 */
enum {
    /**
     * \~English
     * The base is the beginning of the file.
     */
    HiveSeek_Set = (int)0,
    /**
     * \~English
     * The base is the current file's position indicator.
     */
    HiveSeek_Cur = (int)1,
    /**
     * \~English
     * The base is the end of file.
     */
    HiveSeek_End = (int)2,
};

/**
 * \~English
 * Modify @file's position indicator.
 *
 * This function is effective only when state of client associated with @file is
 * "logined".
 *
 * @param
 *      file       [in] A handle identifying the Hive file instance.
 * @param
 *      offset     [in] The offset relative to @whence where @file's position
 *                      indicator is to be positioned.
 * @param
 *      whence     [in] The base of @offset(one of HiveSeek_Set, HiveSeek_Cur,
 *                      HiveSeek_End).
 *
 * @return
 *      If no error occurs, return new file offset. Otherwise, return -1, and a
 *      specific error code can be retrieved by calling hive_get_error().
 */
HIVE_API
ssize_t hive_file_seek(HiveFile *file, ssize_t offset, int whence);

/**
 * \~English
 * Read @bufsz bytes of data from @file.
 *
 * This function is effective only when state of client associated with @file is
 * "logined".
 *
 * @param
 *      file       [in] A handle identifying the Hive file instance.
 * @param
 *      buf        [in] Buffer to hold data.
 * @param
 *      bufsz      [in] Length of data to be read.
 *
 *
 * @return
 *      If no error occurs, return length of data actually read. Otherwise, return -1,
 *      and a specific error code can be retrieved by calling hive_get_error().
 */
HIVE_API
ssize_t hive_file_read(HiveFile *file, char *buf, size_t bufsz);

/**
 * \~English
 * write @bufsz bytes of data to @file.
 *
 * This function is effective only when state of client associated with @file is
 * "logined".
 *
 * @param
 *      file       [in] A handle identifying the Hive file instance.
 * @param
 *      buf        [in] Buffer to hold data.
 * @param
 *      bufsz      [in] Length of data to be written.
 *
 *
 * @return
 *      If no error occurs, return length of data actually written. Otherwise, return -1,
 *      and a specific error code can be retrieved by calling hive_get_error().
 */
HIVE_API
ssize_t hive_file_write(HiveFile *file, const char *buf, size_t bufsz);

/**
 * \~English
 * Commit local change on @file to backend.
 *
 * This function is effective only when state of client associated with @file is
 * "logined".
 *
 * @param
 *      file       [in] A handle identifying the Hive file instance.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_file_commit(HiveFile *file);

/**
 * \~English
 * Discard local change on @file.
 *
 * This function is effective only when state of client associated with @file is
 * "logined".
 *
 * @param
 *      file       [in] A handle identifying the Hive file instance.
 *
 * @return
 *      If no error occurs, return 0. Otherwise, return -1, and a specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_file_discard(HiveFile *file);

/******************************************************************************
 * Error handling
 *****************************************************************************/

// Facility code
#define HIVEF_GENERAL                                0x01
#define HIVEF_SYS                                    0x02
#define HIVEF_RESERVED1                              0x03
#define HIVEF_RESERVED2                              0x04
#define HIVEF_HTTP_CLIENT                            0x05
#define HIVEF_HTTP_STATUS                            0x06

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

/**
 * \~English
 * HTTP status code.
 */
#define HttpStatus_Continue                          100
#define HttpStatus_SwitchingProtocols                101
#define HttpStatus_Processing                        102
#define HttpStatus_EarlyHints                        103
#define HttpStatus_OK                                200
#define HttpStatus_Created                           201
#define HttpStatus_Accepted                          202
#define HttpStatus_NonAuthoritativeInformation       203
#define HttpStatus_NoContent                         204
#define HttpStatus_ResetContent                      205
#define HttpStatus_PartialContent                    206
#define HttpStatus_MultiStatus                       207
#define HttpStatus_AlreadyReported                   208
#define HttpStatus_IMUsed                            226
#define HttpStatus_MultipleChoices                   300
#define HttpStatus_MovedPermanently                  301
#define HttpStatus_Found                             302
#define HttpStatus_SeeOther                          303
#define HttpStatus_NotModified                       304
#define HttpStatus_UseProxy                          305
#define HttpStatus_TemporaryRedirect                 307
#define HttpStatus_PermanentRedirect                 308
#define HttpStatus_BadRequest                        400
#define HttpStatus_Unauthorized                      401
#define HttpStatus_PaymentRequired                   402
#define HttpStatus_Forbidden                         403
#define HttpStatus_NotFound                          404
#define HttpStatus_MethodNotAllowed                  405
#define HttpStatus_NotAcceptable                     406
#define HttpStatus_ProxyAuthenticationRequired       407
#define HttpStatus_RequestTimeout                    408
#define HttpStatus_Conflict                          409
#define HttpStatus_Gone                              410
#define HttpStatus_LengthRequired                    411
#define HttpStatus_PreconditionFailed                412
#define HttpStatus_PayloadTooLarge                   413
#define HttpStatus_URITooLong                        414
#define HttpStatus_UnsupportedMediaType              415
#define HttpStatus_RangeNotSatisfiable               416
#define HttpStatus_ExpectationFailed                 417
#define HttpStatus_ImATeapot                         418
#define HttpStatus_UnprocessableEntity               422
#define HttpStatus_Locked                            423
#define HttpStatus_FailedDependency                  424
#define HttpStatus_UpgradeRequired                   426
#define HttpStatus_PreconditionRequired              428
#define HttpStatus_TooManyRequests                   429
#define HttpStatus_RequestHeaderFieldsTooLarge       431
#define HttpStatus_UnavailableForLegalReasons        451
#define HttpStatus_InternalServerError               500
#define HttpStatus_NotImplemented                    501
#define HttpStatus_BadGateway                        502
#define HttpStatus_ServiceUnavailable                503
#define HttpStatus_GatewayTimeout                    504
#define HttpStatus_HTTPVersionNotSupported           505
#define HttpStatus_VariantAlsoNegotiates             506
#define HttpStatus_InsufficientStorage               507
#define HttpStatus_LoopDetected                      508
#define HttpStatus_NotExtended                       510
#define HttpStatus_NetworkAuthenticationRequired     511

#define HIVE_MK_ERROR(facility, code)  (0x80000000 | ((facility) << 24) | \
                    ((((code) & 0x80000000) >> 8) | ((code) & 0x7FFFFFFF)))

#define HIVE_GENERAL_ERROR(code)       HIVE_MK_ERROR(HIVEF_GENERAL, code)
#define HIVE_SYS_ERROR(code)           HIVE_MK_ERROR(HIVEF_SYS, code)
#define HIVE_HTTPC_ERROR(code)         HIVE_MK_ERROR(HIVEF_HTTP_CLIENT, code)
#define HIVE_HTTP_STATUS_ERROR(code)   HIVE_MK_ERROR(HIVEF_HTTP_STATUS, code)

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
