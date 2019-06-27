#ifndef __HIVE_CLIENT_H__
#define __HIVE_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>
#include <fcntl.h>

#include "ela_hive.h"

#define HIVE_F_RDONLY O_RDONLY
#define HIVE_F_WRONLY O_WRONLY
#define HIVE_F_RDWR   O_RDWR
#define HIVE_F_APPEND O_APPEND
#define HIVE_F_CREAT  O_CREAT
#define HIVE_F_TRUNC  O_TRUNC
#define HIVE_F_EXCL   O_EXCL

#define HIVE_F_IS_SET(flags1, flags2) (((flags1) & (flags2)) == (flags2))
#define HIVE_F_IS_EQ(flags1, flags2)  ((flags1) == (flags2))
#define HIVE_F_UNSET(flags1, flags2)  ((flags1) &= ~(flags2))

struct HiveClient {
    int state;  // login state.

    int (*login)        (HiveClient *, HiveRequestAuthenticationCallback *, void *);
    int (*logout)       (HiveClient *);
    int (*get_info)     (HiveClient *, HiveClientInfo *);
    int (*get_drive)    (HiveClient *, HiveDrive **);
    int (*close)        (HiveClient *);
};

struct HiveDrive {
    int (*get_info)     (HiveDrive *, HiveDriveInfo *);
    int (*stat_file)    (HiveDrive *, const char *path, HiveFileInfo *);
    int (*list_files)   (HiveDrive *, const char *path, HiveFilesIterateCallback *, void *);
    int (*make_dir)     (HiveDrive *, const char *path);
    int (*move_file)    (HiveDrive *, const char *from, const char *to);
    int (*copy_file)    (HiveDrive *, const char *from, const char *to);
    int (*delete_file)  (HiveDrive *, const char *path);
    int (*open_file)    (HiveDrive *, const char *path, int flags, HiveFile **);
    void (*close)       (HiveDrive *);
};

struct HiveFile {
    char path[PATH_MAX];
    int flags;

    ssize_t (*lseek)    (HiveFile *, ssize_t offset, int whence);
    ssize_t (*read)     (HiveFile *, char *buf, size_t bufsz);
    ssize_t (*write)    (HiveFile *, const char *buf, size_t bufsz);
    int     (*commit)   (HiveFile *);
    int     (*discard)  (HiveFile *);
    int     (*close)    (HiveFile *);
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
#include <winnt.h>

inline static
int __sync_val_compare_and_swap(int *ptr, int oldval, int newval)
{
    return (int)InterlockedCompareExchange((LONG *)ptr,
                                           (LONG)newval, (LONG)oldval);
}
#endif

#define _test_and_swap __sync_val_compare_and_swap

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
