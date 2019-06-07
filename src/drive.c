#include <stdlib.h>
#include <stdbool.h>
#include <crystal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "drive.h"
#include "client.h"

int hive_drive_get_info(HiveDrive *drive, HiveDriveInfo *result)
{
    int rc;

    if (!drive || !result) {
        hive_set_error(-1);
        return -1;
    }

    if (!drive->get_info) {
        hive_set_error(-1);
        return -1;
    }

    ref(drive);
    rc = drive->get_info(drive, result);
    deref(drive);

    return rc;
}

void hive_drive_info_free(HiveDriveInfo *info)
{
#define FREE(ptr)           \
    do {                    \
        if (ptr) free(ptr); \
    } while (0)

    if (!info)
        return;

    FREE(info->drive_id);

    free(info);
#undef FREE
}

int hive_drive_file_stat(HiveDrive *drive, const char *path, char **result)
{
    int rc;

    if (!drive || !path || !*path || !result || path[0] != '/') {
        hive_set_error(-1);
        return  -1;
    }

    ref(drive);
    rc = drive->file_stat(drive, path, result);
    deref(drive);

    return rc;
}

int hive_drive_list_files(HiveDrive *drive, const char *path, char **result)
{
    int rc;

    if (!drive || !path || !*path || !result || path[0] != '/') {
        hive_set_error(-1);
        return  -1;
    }

    ref(drive);
    rc = drive->list_files(drive, path, result);
    deref(drive);

    return rc;
}

int hive_drive_mkdir(HiveDrive *drive, const char *path)
{
    int rc;

    if (!drive || !path || !*path || path[0] != '/') {
        hive_set_error(-1);
        return  -1;
    }

    ref(drive);
    rc = drive->makedir(drive, path);
    deref(drive);

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

    ref(drive);
    rc = drive->move_file(drive, old, new);
    deref(drive);

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

    ref(drive);
    rc = drive->copy_file(drive, src, dest);
    deref(drive);

    return rc;
}

int hive_drive_delete_file(HiveDrive *drive, const char *path)
{
    int rc;

    if (!drive || !path || !*path) {
        hive_set_error(-1);
        return -1;
    }

    ref(drive);
    rc = drive->delete_file(drive, path);
    deref(drive);

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
