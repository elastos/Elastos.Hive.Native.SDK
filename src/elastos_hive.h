#ifndef __ELASTOS_HIVE_H__
#define __ELASTOS_HIVE_H__

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

typedef struct HiveClient HiveClient;
typedef struct HiveClientInfo HiveClientInfo;
typedef struct HiveDrive HiveDrive;
typedef struct HiveDriveInfo HiveDriveInfo;

struct HiveOAuthInfo {
    const char *client_id;
    const char *scope;
    const char *redirect_url;
};

enum HiveDriveType {
    HiveDriveType_Local     = 0x0,
    HiveDriveType_OneDrive  = 0x01,

    HiveDriveType_ownCloud  = 0x51,

    HiveDriveType_HiveIPFS  = 0x98,
    HiveDriveType_Butt      = 0x99
};

typedef struct HiveOptions {
    char *persistent_location;
    int  drive_type;
} HiveOptions;

typedef void HiveDriveOptions;

typedef struct OneDriveOptions {
    HiveOptions base;

    const char *client_id;
    const char *scope;
    const char *redirect_url;

    int (*grant_authorize)(const char *request_url);
} OneDriveOptions;

typedef struct HiveIpfsOptions {
    HiveOptions base;

    char *uid;
    size_t bootstraps_size;
    const char *bootstraps_ip[0];
} HiveIpfsOptions ;

typedef struct OneDriveDriveOptions {
    char drive_id[256];
} OneDriveDriveOptions;

HIVE_API
HiveClient *hive_client_new(const HiveOptions *options);

HIVE_API
int hive_client_close(HiveClient *);

HIVE_API
int hive_client_login(HiveClient *);

HIVE_API
int hive_client_logout(HiveClient *);

HIVE_API
int hive_client_get_info(HiveClient *, char **result);

HIVE_API
int hive_client_list_drives(HiveClient *, char **result);

HIVE_API
HiveDrive *hive_drive_open(HiveClient *, const HiveDriveOptions *options);

HIVE_API
int hive_drive_close(HiveDrive *);

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
