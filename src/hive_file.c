/*
 * Copyright (c) 2019 Elastos Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <crystal.h>

#include "ela_hive.h"
#include "hive_client.h"
#include "hive_error.h"

ssize_t hive_file_seek(HiveFile *file, ssize_t offset, Whence whence)
{
    ssize_t rc;

    if (!file || (whence < HiveSeek_Set || whence > HiveSeek_End)) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return (ssize_t)-1;
    }

    if (!file->lseek) {
        vlogE("File: file type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return (ssize_t)-1;
    }

    rc = file->lseek(file, offset, whence);
    if (rc < 0) {
        vlogE("File: failed to set file position.");
        hive_set_error((int)rc);
        return (ssize_t)-1;
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
        vlogE("File: file type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->read(file, buf, bufsz);
    if (rc < 0) {
        vlogE("File: Failed to read from file.");
        hive_set_error((int)rc);
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
        vlogE("File: file type does not support this method.");
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = file->write(file, buf, bufsz);
    if (rc < 0) {
        vlogE("File: Failed to write to file.");
        hive_set_error((int)rc);
        return -1;
    }

    return rc;
}

int hive_file_close(HiveFile *file)
{
    int rc;

    if (!file)
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    rc = file->close(file);
    if (rc < 0) {
        vlogE("File: Failed to close file.");
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
        vlogE("File: file type does not support this method.");
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
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!file->discard) {
        vlogE("File: file type does not support this method.");
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
