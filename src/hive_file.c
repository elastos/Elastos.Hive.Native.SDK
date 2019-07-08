#include <crystal.h>

#include "ela_hive.h"
#include "hive_client.h"
#include "hive_error.h"

ssize_t hive_file_seek(HiveFile *file, ssize_t offset, int whence)
{
    ssize_t rc;

    vlogD("File: Calling hive_file_seek().");

    if (!file || (whence < HiveSeek_Set || whence > HiveSeek_End)) {
        vlogE("File: failed to set file position: invalid arguments.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!file->lseek) {
        vlogE("File: failed to set file position: file type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->lseek(file, offset, whence);
    if (rc < 0) {
        vlogE("File: failed to set file position (%d).", rc);
        hive_set_error(rc);
        return -1;
    }

    return rc;
}

ssize_t hive_file_read(HiveFile *file, char *buf, size_t bufsz)
{
    ssize_t rc;

    vlogD("File: Calling hive_file_read().");

    if (!file || !buf || !bufsz || HIVE_F_IS_SET(file->flags, HIVE_F_WRONLY)) {
        vlogE("File: Failed to read from file: invalid arguments.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!file->read) {
        vlogE("File: Failed to read from file: file type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->read(file, buf, bufsz);
    if (rc < 0) {
        vlogE("File: Failed to read from file (%d).", rc);
        hive_set_error(rc);
        return -1;
    }

    return rc;
}

ssize_t hive_file_write(HiveFile *file, const char *buf, size_t bufsz)
{
    ssize_t rc;

    vlogD("File: Calling hive_file_write().");

    if (!file || !buf || !bufsz ||
        (!HIVE_F_IS_SET(file->flags, HIVE_F_WRONLY) &&
         !HIVE_F_IS_SET(file->flags, HIVE_F_RDWR))) {
        vlogE("File: Failed to write to file: invalid arguments.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!file->write) {
        vlogE("File: Failed to write to file: file type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->write(file, buf, bufsz);
    if (rc < 0) {
        vlogE("File: Failed to write to file (%d).", rc);
        hive_set_error(rc);
        return -1;
    }

    return rc;
}

int hive_file_close(HiveFile *file)
{
    int rc;

    vlogD("File: Calling hive_file_close().");

    if (!file) {
        vlogE("File: Failed to close file: no file instance given.");
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);
    }

    rc = file->close(file);
    if (rc < 0) {
        vlogE("File: Failed to close file (%d).", rc);
        hive_set_error(rc);
        return -1;
    }

    return rc;
}

int hive_file_commit(HiveFile *file)
{
    int rc;

    vlogD("File: Calling hive_file_commit().");

    if (!file || (!HIVE_F_IS_SET(file->flags, HIVE_F_WRONLY) &&
                  !HIVE_F_IS_SET(file->flags, HIVE_F_RDWR))) {
        vlogE("File: Failed to commit file: invalid arguments.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!file->commit) {
        vlogE("File: Failed to commit file: file type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->commit(file);
    if (rc < 0) {
        vlogE("File: Failed to commit file (%d).", rc);
        hive_set_error(rc);
        return -1;
    }

    return rc;
}

int hive_file_discard(HiveFile *file)
{
    int rc;

    vlogD("File: Calling hive_file_discard().");

    if (!file || (!HIVE_F_IS_SET(file->flags, HIVE_F_WRONLY) &&
                  !HIVE_F_IS_SET(file->flags, HIVE_F_RDWR))) {
        vlogE("File: Failed to discard file: invalid arguments.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!file->discard) {
        vlogE("File: Failed to discard file: file type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->discard(file);
    if (rc < 0) {
        vlogE("File: Failed to discard file (%d).", rc);
        hive_set_error(rc);
        return -1;
    }

    return rc;
}
