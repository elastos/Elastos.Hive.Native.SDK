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

#ifdef __cplusplus
} // extern "C"
#endif

#if defined(__APPLE__)
#pragma GCC diagnostic pop
#endif

#endif // __ELASTOS_HIVE_H__
