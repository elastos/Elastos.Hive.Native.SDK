#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "drive.h"
#include "error.h"

int hive_drive_get_info(HiveDrive *drive, HiveDriveInfo *drive_info)
{
    if (!drive || !drive_info) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!drive->get_info) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    return drive->get_info(drive, drive_info);
}

int hive_drive_file_stat(HiveDrive *drive, const char *path,
                         HiveFileInfo *file_info)
{
    if (!drive || !path || path[0] != '/' || strlen(path) > MAXPATHLEN || !file_info) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!drive->stat_file) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    return drive->stat_file(drive, path, file_info);
}

int hive_drive_list_files(HiveDrive *drive, const char *path,
                          HiveFilesIterateCallback *callback, void *context)
{
    if (!drive || !path || *path != '/' || !callback) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!drive->list_files) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    return drive->list_files(drive, path, callback, context);
}

int hive_drive_mkdir(HiveDrive *drive, const char *path)
{
    if (!drive || !path || path[0] != '/' || path[1] == '\0' || strlen(path) > MAXPATHLEN) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (strcmp(path, "/") == 0) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!drive->make_dir) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    return drive->make_dir(drive, path);
}

int hive_drive_move_file(HiveDrive *drive, const char *old, const char *new)
{
    if (!drive ||
        !old || old[0] != '/' || old[1] == '\0' || strlen(old) > MAXPATHLEN ||
        !new || new[0] != '/' || new[1] == '\0' || strlen(new) > MAXPATHLEN) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (strcmp(old, new) == 0) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!drive->move_file) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    return drive->move_file(drive, old, new);
}

int hive_drive_copy_file(HiveDrive *drive, const char *src, const char *dest)
{
   if (!drive ||
        !src || src[0] != '/' || src[1] == '\0' || strlen(src) > MAXPATHLEN ||
        !dest || dest[0] != '/' || dest[1] == '\0' || strlen(dest) > MAXPATHLEN) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (strcmp(src, dest) == 0) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!drive->copy_file) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    return drive->copy_file(drive, src, dest);
}

int hive_drive_delete_file(HiveDrive *drive, const char *path)
{
    if (!drive || !path || path[0] != '/' || path[1] == '\0' ||
        strlen(path) > MAXPATHLEN) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!drive->delete_file) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    return drive->delete_file(drive, path);
}

int hive_drive_close(HiveDrive *drive)
{
    if (!drive)
        return 0;

    drive->close(drive);
    return 0;
}
