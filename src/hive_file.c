#include "ela_hive.h"
#include "hive_client.h"
#include "hive_error.h"

ssize_t hive_file_seek(HiveFile *file, uint64_t offset, int whence)
{
    ssize_t rc;

    if (!file || (whence < HiveSeek_Set || whence > HiveSeek_End))
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    if (!file->lseek)
        return HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED);

    rc = file->lseek(file, offset, whence);

    return rc;
}

ssize_t hive_file_read(HiveFile *file, char *buf, size_t bufsz)
{
    ssize_t rc;

    if (!file || !buf || !bufsz || HIVE_F_IS_SET(file->flags, HIVE_F_WRONLY))
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    if (!file->read)
        return HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED);

    rc = file->read(file, buf, bufsz);

    return rc;
}

ssize_t hive_file_write(HiveFile *file, const char *buf, size_t bufsz)
{
    ssize_t rc;

    if (!file || !buf || !bufsz ||
        (!HIVE_F_IS_SET(file->flags, HIVE_F_WRONLY) &&
         !HIVE_F_IS_SET(file->flags, HIVE_F_RDWR)))
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    if (!file->write)
        return HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED);

    rc = file->write(file, buf, bufsz);

    return rc;
}

int hive_file_close(HiveFile *file)
{
    int rc;

    if (!file)
        return 0;

    rc = file->close(file);
    return rc;
}

int hive_file_commit(HiveFile *file)
{
    int rc;

    if (!file || (!HIVE_F_IS_SET(file->flags, HIVE_F_WRONLY) &&
                  !HIVE_F_IS_SET(file->flags, HIVE_F_RDWR)))
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    if (!file->commit)
        return HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED);

    rc = file->commit(file);

    return rc;
}

int hive_file_discard(HiveFile *file)
{
    int rc;

    if (!file || (!HIVE_F_IS_SET(file->flags, HIVE_F_WRONLY) &&
                  !HIVE_F_IS_SET(file->flags, HIVE_F_RDWR)))
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    if (!file->discard)
        return HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED);

    rc = file->discard(file);

    return rc;
}
