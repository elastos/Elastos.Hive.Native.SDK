#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "elastos_hive.h"

struct HiveClient {
    int (*login)(HiveClient *);
    int (*logout)(HiveClient *);
    int (*get_info)(HiveClient *, char **result);
    int (*list_drives)(HiveClient *, char **result);
    HiveDrive *(*drive_open)(HiveClient *, const HiveDriveOptions *);
    void (*destructor_func)(HiveClient *);

    int (*expire_access_token)(HiveClient *);
    int (*get_access_token)(HiveClient *, char **access_token);
    int (*refresh_access_token)(HiveClient *, char **access_token);
};

int hive_client_expire_access_token(HiveClient *);

int hive_client_get_access_token(HiveClient *, char **access_token);

int hive_client_refresh_access_token(HiveClient *, char **access_token);

#endif // __CLIENT_H__
