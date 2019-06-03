#ifndef __HIVE_CLIENT_H__
#define __HIVE_CLIENT_H__

#include "ela_hive.h"

struct HiveClient {
    int (*login)        (HiveClient *);
    int (*logout)       (HiveClient *);
    int (*get_info)     (HiveClient *, char **result);
    int (*finalize)     (HiveClient *);
    int (*list_drives)  (HiveClient *, char **result);

    HiveDrive *(*get_default_drive)(HiveClient *);

    int (*invalidate_credential)(HiveClient *);
};

HIVE_API
int hive_client_invalidate_credential(HiveClient *);

inline static void hive_set_error(int error) { }

#endif // __HIVE_CLIENT_H__
