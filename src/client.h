#ifndef __HIVE_CLIENT_H__
#define __HIVE_CLIENT_H__

#include <stdbool.h>
#include <stdint.h>
#if defined(_WIN32) || defined(_WIN64)
#include <winnt.h>
#endif

#include "ela_hive.h"

enum {
    RAW          = (uint32_t)0,    // The primitive state.
    LOGINING     = (uint32_t)1,    // Being in the middle of logining.
    LOGINED      = (uint32_t)2,    // Already being logined.
    LOGOUTING    = (uint32_t)3,    // Being in the middle of logout.
};

struct HiveClient {
    uint32_t state;  // login state.

    int (*login)        (HiveClient *);
    int (*logout)       (HiveClient *);
    int (*get_info)     (HiveClient *, HiveClientInfo *result);
    int (*finalize)     (HiveClient *);
    int (*list_drives)  (HiveClient *, char **result);
    int (*get_drive)    (HiveClient *, HiveDrive **);

    int (*invalidate_credential)(HiveClient *);
};

inline static void hive_set_error(int error) { }

static inline uint32_t _test_and_swap32(uint32_t *ptr, uint32_t oldval, uint32_t newval)
{
#if defined(_WIN32) || defined(_WIN64)
    return InterlockedCompareExchange(ptr, newval, oldval);
#else
    uint32_t tmp = oldval;
    __atomic_compare_exchange(ptr, &tmp, &newval, false,
                              __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return tmp;
#endif
}

inline static
int client_try_login(HiveClient *client)
{
    int rc;

    rc = _test_and_swap32(&client->state, RAW, LOGINING);
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

inline static
int client_try_logout(HiveClient *client)
{
    int rc;

    rc = _test_and_swap32(&client->state, LOGINED, LOGOUTING);
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

inline static
bool is_client_ready(HiveClient *client)
{
    int rc;

    rc = _test_and_swap32(&client->state, LOGINED, LOGINED);
    return (rc == LOGINED);
}

#endif // __HIVE_CLIENT_H__
