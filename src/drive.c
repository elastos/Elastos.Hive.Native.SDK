#include <stdlib.h>
#include <stdbool.h>
#include <crystal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "drive.h"
#include "client.h"

int hive_drive_get_info(HiveDrive *drive, HiveDriveInfo *drive_info)
{
    int rc;

    if (!drive || !drive_info) {
        hive_set_error(-1);
        return -1;
    }

    if (!drive->get_info) {
        hive_set_error(-1);
        return -1;
    }

    rc = drive->get_info(drive, drive_info);
    return rc;
}

int hive_drive_file_stat(HiveDrive *drive, const char *path, HiveFileInfo *file_info)
{
    int rc;

    if (!drive || !path || !*path || !file_info || path[0] != '/') {
        hive_set_error(-1);
        return  -1;
    }

    rc = drive->stat_file(drive, path, file_info);
    return rc;
}

int hive_drive_list_files(HiveDrive *drive, const char *dir_path,
                          HiveFilesIterateCallback *callback, void *context)
{
    int rc;

    if (!drive || !dir_path || dir_path[0] != '/' || strlen(dir_path) > MAXPATHLEN ||
        !callback ) {
        hive_set_error(-1);
        return  -1;
    }

    rc = drive->list_files(drive, dir_path, callback, context);
    return rc;
}

int hive_drive_mkdir(HiveDrive *drive, const char *path)
{
    int rc;

    if (!drive || !path || !*path || path[0] != '/') {
        hive_set_error(-1);
        return  -1;
    }

    rc = drive->makedir(drive, path);
    return rc;
}

int hive_drive_move_file(HiveDrive *drive, const char *old, const char *new)
{
    int rc;

    if (!drive || !old || !*old || !new || !*new ||
        strlen(old) > MAXPATHLEN ||
        strlen(new) > MAXPATHLEN ||
        strcmp(old, new) == 0) {
        hive_set_error(-1);
        return -1;
    }

    rc = drive->move_file(drive, old, new);
    return rc;
}

int hive_drive_copy_file(HiveDrive *drive, const char *src, const char *dest)
{
    int rc;

    if (!drive || !src || !*src || !dest || !*dest ||
        strlen(src) > MAXPATHLEN ||
        strlen(dest) > MAXPATHLEN ||
        strcmp(src, dest) == 0) {
        hive_set_error(-1);
        return -1;
    }

    rc = drive->copy_file(drive, src, dest);
    return rc;
}

int hive_drive_delete_file(HiveDrive *drive, const char *path)
{
    int rc;

    if (!drive || !path || !*path) {
        hive_set_error(-1);
        return -1;
    }

    rc = drive->delete_file(drive, path);
    return rc;
}

int hive_drive_close(HiveDrive *drive)
{
    if (!drive) {
        hive_set_error(-1);
        return -1;
    }

    drive->close(drive);
    return 0;
}
