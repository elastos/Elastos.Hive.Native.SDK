#ifndef __HIVE_CLIENT_H__
#define __HIVE_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <winnt.h>
#endif

#include "ela_hive.h"

struct HiveClient {
    uint32_t state;  // login state.

    int (*login)        (HiveClient *, HiveRequestAuthenticationCallback *, void *);
    int (*logout)       (HiveClient *);
    int (*get_info)     (HiveClient *, HiveClientInfo *);
    int (*get_drive)    (HiveClient *, HiveDrive **);
    int (*close)        (HiveClient *);
};

struct HiveDrive {
    HiveClient *client;

    int (*get_info)     (HiveDrive *, HiveDriveInfo *);
    int (*stat_file)    (HiveDrive *, const char *path, HiveFileInfo *);
    int (*list_files)   (HiveDrive *, const char *path, HiveFilesIterateCallback *, void *);
    int (*make_dir)     (HiveDrive *, const char *path);
    int (*move_file)    (HiveDrive *, const char *from, const char *to);
    int (*copy_file)    (HiveDrive *, const char *from, const char *to);
    int (*delete_file)  (HiveDrive *, const char *path);
    void (*close)       (HiveDrive *);
};

/*
 * convinient internal APIs to check for getting athorization or not.
 */
enum {
    RAW          = (uint32_t)0,    // The primitive state.
    LOGINING     = (uint32_t)1,    // Being in the middle of logining.
    LOGINED      = (uint32_t)2,    // Already being logined.
    LOGOUTING    = (uint32_t)3,    // Being in the middle of logout.
};

#if defined(_WIN32) || defined(_WIN64)
#define _test_and_swap(ptr, oldval, newval) \
                    InterlockedCompareExchange(ptr, newval, oldval)
#else
#define _test_and_swap(ptr, oldval, newval) \
                    __sync_val_compare_and_swap(ptr, oldval, newval)
#endif

inline static int check_and_login(HiveClient *client)
{
    int rc;

    rc = _test_and_swap(&client->state, RAW, LOGINING);
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

inline static int check_and_logout(HiveClient *client)
{
    int rc;

    rc = _test_and_swap(&client->state, LOGINED, LOGOUTING);
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

inline static bool has_valid_token(HiveClient *client)
{
    return LOGINED == _test_and_swap(&client->state, LOGINED, LOGINED);
}


inline static bool is_absolute_path(const char *path)
{
    return (path && path[0] == '/');
}

#ifdef __cplusplus
}
#endif

#endif // __HIVE_CLIENT_H__
