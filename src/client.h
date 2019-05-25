#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "elastos_hive.h"

typedef void client_tsx_t;

struct HiveClient {
    int (*login)(HiveClient *);
    int (*logout)(HiveClient *);
    int (*get_info)(HiveClient *, char **result);
    int (*list_drives)(HiveClient *, char **result);
    HiveDrive *(*drive_open)(HiveClient *, const HiveDriveOptions *);
    void (*destructor_func)(HiveClient *);

    int (*perform_tsx)(HiveClient *, client_tsx_t *);
    int (*invalidate_credential)(HiveClient *);
};

int hive_client_invalidate_credential(HiveClient *);

int hive_client_perform_transaction(HiveClient *, client_tsx_t *);

#endif // __CLIENT_H__
