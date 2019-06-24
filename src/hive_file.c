#include <assert.h>
#include "ela_hive.h"
#include "hive_client.h"
#include "hive_error.h"

int hive_file_seek(HiveFile *file, uint64_t offset, int whence)
{
    int rc;

    if (!file || (whence < HiveSeek_Set && whence > HiveSeek_End)) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    assert(file->token);

    if (!file->lseek) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->lseek(file, offset, whence);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

ssize_t hive_file_read(HiveFile *file, char *buf, size_t bufsz)
{
    ssize_t rc;

    if (!file || !buf || !bufsz) { // TODO: mode.
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    assert(file->token);

    if (!file->read) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->read(file, buf, bufsz);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

ssize_t hive_file_write(HiveFile *file, const char *buf, size_t bufsz)
{
    ssize_t rc;

    if (!file || !buf || !bufsz) { // TODO: mode.
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    assert(file->token);

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
    int rc = 0;

    if (!file) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    assert(file->token);

    if (file->close)
        rc = file->close(file);

    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

int hive_file_commit(HiveFile *file)
{
    int rc;

    if (!file) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    assert(file->token);

    if (!file->commit) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->commit(file);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

int hive_file_discard(HiveFile *file)
{
    int rc;

    if (!file) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    assert(file->token);

    if (!file->discard) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->discard(file);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}
