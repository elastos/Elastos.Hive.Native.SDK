#ifndef __HIVE_CLIENT_H__
#define __HIVE_CLIENT_H__

#include <stdbool.h>
#include "ela_hive.h"

enum {
    RAW          = (int)0,    // The primitive state.
    LOGINING     = (int)1,    // Being in the middle of logining.
    LOGINED      = (int)2,    // Already being logined.
    LOGOUTING    = (int)3,    // Being in the middle of logout.
};

struct HiveClient {
    int state;  // login state.

    int (*login)        (HiveClient *);
    int (*logout)       (HiveClient *);
    int (*get_info)     (HiveClient *, char **result);
    int (*finalize)     (HiveClient *);
    int (*list_drives)  (HiveClient *, char **result);
    int (*get_drive)    (HiveClient *, HiveDrive **);

    int (*invalidate_credential)(HiveClient *);
};

HIVE_API
int hive_client_invalidate_credential(HiveClient *);

inline static void hive_set_error(int error) { }

inline static int client_try_login(HiveClient *client)
{
    int rc;

    rc = __sync_val_compare_and_swap(&client->state, RAW, LOGINING);
    switch(rc) {
    case RAW:
        rc = 0; // need to proceed the login routine.
        break;

    case LOGINING:
    case LOGOUTING:
    default:
        rc = -1;
        break;

    case LOGINED:
        rc = 1;
        break;
    }

    return rc;
}

inline static int client_try_logout(HiveClient *client)
{
    int rc;

    rc = __sync_val_compare_and_swap(&client->state, LOGINED, LOGOUTING);
    switch(rc) {
    case RAW:
        rc = 1;
        break;

    case LOGINING:
    case LOGOUTING:
    default:
        rc = -1;
        break;

    case LOGINED:
        rc = 0;
        break;
    }

    return rc;
}

inline static bool is_client_ready(HiveClient *client)
{
    int rc;

    rc = __sync_val_compare_and_swap(&client->state, LOGINED, LOGINED);
    return (rc == LOGINED);
}

#endif // __HIVE_CLIENT_H__
