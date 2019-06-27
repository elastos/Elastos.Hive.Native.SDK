#include "ela_hive.h"
#include "hive_client.h"
#include "hive_error.h"

ssize_t hive_file_seek(HiveFile *file, ssize_t offset, int whence)
{
    ssize_t rc;

    if (!file || (whence < HiveSeek_Set || whence > HiveSeek_End)) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!file->lseek) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->lseek(file, offset, whence);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return rc;
}

ssize_t hive_file_read(HiveFile *file, char *buf, size_t bufsz)
{
    ssize_t rc;

    if (!file || !buf || !bufsz || HIVE_F_IS_SET(file->flags, HIVE_F_WRONLY)) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!file->read) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->read(file, buf, bufsz);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return rc;
}

ssize_t hive_file_write(HiveFile *file, const char *buf, size_t bufsz)
{
    ssize_t rc;

    if (!file || !buf || !bufsz ||
        (!HIVE_F_IS_SET(file->flags, HIVE_F_WRONLY) &&
         !HIVE_F_IS_SET(file->flags, HIVE_F_RDWR))) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!file->write) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->write(file, buf, bufsz);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return rc;
}

int hive_file_close(HiveFile *file)
{
    int rc;

    if (!file)
        return 0;

    rc = file->close(file);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return rc;
}

int hive_file_commit(HiveFile *file)
{
    int rc;

    if (!file || (!HIVE_F_IS_SET(file->flags, HIVE_F_WRONLY) &&
                  !HIVE_F_IS_SET(file->flags, HIVE_F_RDWR))) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!file->commit) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->commit(file);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return rc;
}

int hive_file_discard(HiveFile *file)
{
    int rc;

    if (!file || (!HIVE_F_IS_SET(file->flags, HIVE_F_WRONLY) &&
                  !HIVE_F_IS_SET(file->flags, HIVE_F_RDWR))) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!file->discard) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->discard(file);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return rc;
}
