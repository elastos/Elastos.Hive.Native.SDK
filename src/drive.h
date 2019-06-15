#ifndef __HIVE_DRIVE_H__
#define __HIVE_DRIVE_H__

#include "ela_hive.h"

struct HiveDrive {
    int (*get_info)     (HiveDrive *, HiveDriveInfo *result);
    int (*stat_file)    (HiveDrive *, const char *path, HiveFileInfo *);
    int (*list_files)   (HiveDrive *, const char *path, char **result);
    int (*makedir)      (HiveDrive *, const char *path);
    int (*move_file)    (HiveDrive *, const char *from, const char *to);
    int (*copy_file)    (HiveDrive *, const char *from, const char *to);
    int (*delete_file)  (HiveDrive *, const char *path);
    void (*close)       (HiveDrive *);
};

#endif // __HIVE_DRIVE_H__
