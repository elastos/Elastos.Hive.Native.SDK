#ifndef __CLIENT_IMPL_H
#define __CLIENT_IMPL_H

#include "client.h"

struct HiveClient {
    int (*login)(HiveClient *);
    int (*logout)(HiveClient *);
    int (*list_drives)(HiveClient *, char **);
    HiveDrive *(*drive_open)(HiveClient *, const HiveDriveOptions *);
    void (*destructor_func)(HiveClient *);

    int (*get_access_token)(HiveClient *, char **access_token);
    int (*refresh_access_token)(HiveClient *, char **access_token);
};

int hive_client_get_access_token(HiveClient *, char **access_token);

int hive_client_refresh_access_token(HiveClient *, char **access_token);

#endif // __CLIENT_IMPL_H__
