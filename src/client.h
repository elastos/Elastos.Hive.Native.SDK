#ifndef __HIVE_CLIENT_H__
#define __HIVE_CLIENT_H__

#include "elastos_hive.h"

typedef void client_tsx_t;

struct HiveClient {
    int (*login)    (HiveClient *);
    int (*logout)   (HiveClient *);
    int (*get_info) (HiveClient *, char **result);
    int (*finalize) (HiveClient *);

    HiveDrive *(*get_default_drive)(HiveClient *);

    int (*list_drives)(HiveClient *, char **result);
    HiveDrive *(*drive_open)(HiveClient *, const HiveDriveOptions *);

    int (*perform_tsx)(HiveClient *, client_tsx_t *);
    int (*invalidate_credential)(HiveClient *);
};

HIVE_API
int hive_client_invalidate_credential(HiveClient *);

int hive_client_perform_transaction(HiveClient *, client_tsx_t *);

inline static void hive_set_error(int errno) { }

#endif // __HIVE_CLIENT_H__
