#ifndef __HIVE_CLIENT_H__
#define __HIVE_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>

#include "ela_hive.h"
#include "token_base.h"

#define HIVE_F_IS_SET(flags1, flags2) (((flags1) &  (flags2)) == (flags2))
#define HIVE_F_IS_EQ(flags1, flags2)  ((flags1) == (flags2))

struct HiveClient {
    token_base_t *token;

    int (*login)        (HiveClient *, HiveRequestAuthenticationCallback *, void *);
    int (*logout)       (HiveClient *);
    int (*get_info)     (HiveClient *, HiveClientInfo *);
    int (*get_drive)    (HiveClient *, HiveDrive **);
    int (*close)        (HiveClient *);
};

struct HiveDrive {
    token_base_t *token;

    int (*get_info)     (HiveDrive *, HiveDriveInfo *);
    int (*stat_file)    (HiveDrive *, const char *path, HiveFileInfo *);
    int (*list_files)   (HiveDrive *, const char *path, HiveFilesIterateCallback *, void *);
    int (*make_dir)     (HiveDrive *, const char *path);
    int (*move_file)    (HiveDrive *, const char *from, const char *to);
    int (*copy_file)    (HiveDrive *, const char *from, const char *to);
    int (*delete_file)  (HiveDrive *, const char *path);
    int (*open_file)    (HiveDrive *, const char *path, const char *mode, HiveFile **);
    void (*close)       (HiveDrive *);
};

struct HiveFile {
    token_base_t *token;

    char path[PATH_MAX];
    char *mode;

    int (*lseek)        (HiveFile *, uint64_t offset, int whence);
    ssize_t (*read)     (HiveFile *, char *buf, size_t bufsz);
    ssize_t (*write)    (HiveFile *, const char *buf, size_t bufsz);
    int (*commit)       (HiveFile *);
    int (*discard)      (HiveFile *);
    int (*close)        (HiveFile *);
};

inline static bool is_absolute_path(const char *path)
{
    return (path && path[0] == '/');
}

#ifdef __cplusplus
}
#endif

#endif // __HIVE_CLIENT_H__
