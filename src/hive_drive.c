#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <crystal.h>

#include "hive_error.h"
#include "hive_client.h"

int hive_drive_get_info(HiveDrive *drive, HiveDriveInfo *info)
{
    int rc;

    vlogD("Drive: Calling hive_drive_get_info().");

    if (!drive || !info) {
        vlogE("Drive: Failed to get drive info: invalid arguments.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!drive->get_info) {
        vlogE("Drive: Failed to get drive info: drive type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = drive->get_info(drive, info);
    if (rc < 0) {
        vlogE("Drive: Failed to get drive info (%d).", rc);
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

int hive_drive_file_stat(HiveDrive *drive, const char *path, HiveFileInfo *info)
{
    int rc;

    vlogD("Drive: Calling hive_drive_file_stat().");

    if (!drive || !info) {
        vlogE("Drive: Failed to get file status: invalid arguments.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!is_absolute_path(path)) {
        vlogE("Drive: Failed to get file status: path must be absolute.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!drive->stat_file) {
        vlogE("Drive: Failed to get file status: drive type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = drive->stat_file(drive, path, info);
    if (rc < 0) {
        vlogE("Drive: Failed to get file status (%d).", rc);
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

int hive_drive_list_files(HiveDrive *drive, const char *path,
                          HiveFilesIterateCallback *callback, void *context)
{
    int rc;

    vlogD("Drive: Calling hive_drive_list_files().");

    if (!drive || !callback) {
        vlogE("Drive: Failed to list files: invalid arguments.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!is_absolute_path(path)) {
        vlogE("Drive: Failed to list files: path must be absolute.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!drive->list_files) {
        vlogE("Drive: Failed to list files: drive type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = drive->list_files(drive, path, callback, context);
    if (rc < 0) {
        vlogE("Drive: Failed to list files (%d).", rc);
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

int hive_drive_mkdir(HiveDrive *drive, const char *path)
{
    int rc;

    vlogD("Drive: Calling hive_drive_mkdir().");

    if (!drive) {
        vlogE("Drive: Failed to make dir: invalid arguments.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!is_absolute_path(path) || strcmp(path, "/") == 0) {
        vlogE("Drive: Failed to make dir: path must be absolute.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!drive->make_dir) {
        vlogE("Drive: Failed to make dir: drive type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = drive->make_dir(drive, path);
    if (rc < 0) {
        vlogE("Drive: Failed to make dir (%d).", rc);
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

int hive_drive_move_file(HiveDrive *drive, const char *from, const char *to)
{
    int rc;

    vlogD("Drive: Calling hive_drive_move_file().");

    if (!drive) {
        vlogE("Drive: Failed to move file: invalid arguments.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!is_absolute_path(from) || !is_absolute_path(to) ||
        strcmp(from, "/") == 0  || strcmp(from, to) == 0) {
        vlogE("Drive: Failed to move file: path must be absolute.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!drive->move_file) {
        vlogE("Drive: Failed to move file: drive type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = drive->move_file(drive, from, to);
    if (rc < 0) {
        vlogE("Drive: Failed to move file (%d).", rc);
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

int hive_drive_copy_file(HiveDrive *drive, const char *src, const char *dest)
{
    int rc;

    vlogD("Drive: Calling hive_drive_copy_file().");

    if (!drive) {
        vlogE("Drive: Failed to copy file: invalid arguments.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!is_absolute_path(src) || !is_absolute_path(dest) ||
        strcmp(src, "/") == 0  || strcmp(dest, "/") == 0  ||
        strcmp(src, dest) == 0) {
        vlogE("Drive: Failed to copy file: path must be absolute.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!drive->copy_file) {
        vlogE("Drive: Failed to copy file: drive type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = drive->copy_file(drive, src, dest);
    if (rc < 0) {
        vlogE("Drive: Failed to copy file (%d).", rc);
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

int hive_drive_delete_file(HiveDrive *drive, const char *path)
{
    int rc;

    vlogD("Drive: Calling hive_drive_delete_file().");

    if (!drive) {
        vlogE("Drive: Failed to copy file: no drive instance given.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!is_absolute_path(path) || strcmp(path, "/") == 0) {
        vlogE("Drive: Failed to copy file: path must be absolute.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!drive->delete_file) {
        vlogE("Drive: Failed to copy file: drive type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = drive->delete_file(drive, path);
    if (rc < 0) {
        vlogE("Drive: Failed to copy file (%d).", rc);
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

static int mode_to_flags(const char *mode, int *flags)
{
    if (!mode)
        return -1;

    assert(flags);

    if (!strcmp(mode, "r") || !strcmp(mode, "rb")) {
        *flags = HIVE_F_RDONLY;
        return 0;
    } else if (!strcmp(mode, "w") || !strcmp(mode, "wb")) {
        *flags = HIVE_F_WRONLY | HIVE_F_CREAT | HIVE_F_TRUNC;
        return 0;
    } else if (!strcmp(mode, "a") || !strcmp(mode, "ab")) {
        *flags = HIVE_F_WRONLY | HIVE_F_CREAT | HIVE_F_APPEND;
        return 0;
    } else if (!strcmp(mode, "r+") || !strcmp(mode, "r+b") || !strcmp(mode, "rb+")) {
        *flags = HIVE_F_RDWR;
        return 0;
    } else if (!strcmp(mode, "w+") || !strcmp(mode, "w+b") || !strcmp(mode, "wb+")) {
        *flags = HIVE_F_RDWR | HIVE_F_CREAT | HIVE_F_TRUNC;
        return 0;
    } else if (!strcmp(mode, "a+") || !strcmp(mode, "a+b") || !strcmp(mode, "ab+")) {
        *flags = HIVE_F_RDWR | HIVE_F_CREAT | HIVE_F_APPEND;
        return 0;
    } else
        return -1;
}

HiveFile *hive_file_open(HiveDrive *drive, const char *path, const char *mode)
{
    int rc;
    HiveFile *file;
    int flags;

    vlogD("Drive: Calling hive_file_open().");

    if (!drive) {
        vlogE("Drive: Failed to open file: no drive instance given.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return NULL;
    }

    if (!is_absolute_path(path) || strcmp(path, "/") == 0) {
        vlogE("Drive: Failed to open file: path must be absolute.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return NULL;
    }

    if (mode_to_flags(mode, &flags) < 0) {
        vlogE("Drive: Failed to open file: invalid mode given.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return NULL;
    }

    if (!drive->open_file) {
        vlogE("Drive: Failed to open file: drive type does not supoort this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return NULL;
    }

    rc = drive->open_file(drive, path, flags, &file);
    if (rc < 0) {
        vlogE("Drive: Failed to open file (%d).", rc);
        hive_set_error(rc);
        return NULL;
    }

    return file;
}

int hive_drive_close(HiveDrive *drive)
{
    if (!drive)
        return 0;

    drive->close(drive);
    return 0;
}
