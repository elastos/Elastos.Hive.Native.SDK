#ifndef __HIVE_CLIENT_H__
#define __HIVE_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HiveClient HiveClient;

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

typedef struct OneDriveOptions {
    HiveOptions base;

    const char *client_id;
    const char *scope;
    const char *redirect_url;

    int (*grant_authorize)(const char *request_url);
} OneDriveOptions;

HiveClient *hive_client_new(const HiveOptions *options);

int hive_client_close(HiveClient *hive);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __HIVE_CLIENT_H__

