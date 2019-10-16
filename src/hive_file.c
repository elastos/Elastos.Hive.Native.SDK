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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#endif

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include <crystal.h>

#include "hive_error.h"
#include "hive_client.h"

int hive_put_file(HiveConnect *connect, const char *from, bool encrypt,
                  const char *filename)
{
    ssize_t nrd;
    void *buf;
    size_t fsize;
    int rc;
    int fd;

    if (!connect || !from || !*from || !filename || !*filename) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    fd = open(from, O_RDONLY);
    if (fd < 0) {
        hive_set_error(HIVE_SYS_ERROR(errno));
        return -1;
    }

    fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    if (fsize > HIVE_MAX_FILE_SIZE) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_LIMIT_EXCEEDED));
        close(fd);
        return -1;
    }

    buf = (uint8_t *)calloc(1, fsize);
    if (!buf) {
        close(fd);
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        return -1;
    }

    nrd = read(fd, buf,
#if defined(_WIN32) || defined(_WIN64)
               (unsigned)
#endif
               fsize);
    close(fd);

    if (nrd != fsize) {
        free(buf);
        hive_set_error(HIVE_SYS_ERROR(errno));
        return -1;
    }

    rc = hive_put_file_from_buffer(connect, buf, fsize, encrypt, filename);
    free(buf);

    return rc;
}

int hive_put_file_from_buffer(HiveConnect *connect, const void *from, size_t length,
                              bool encrypt, const char *filename)
{
    int rc;

    if (!connect || (!from && length) || length > HIVE_MAX_FILE_SIZE ||
        !filename || !*filename) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!connect->put_file_from_buffer) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = connect->put_file_from_buffer(connect, from, length, encrypt, filename);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

ssize_t hive_get_file_length(HiveConnect *connect, const char *filename)
{
    ssize_t rc;

    if (!connect || !filename || !*filename) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!connect->get_file_length) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = connect->get_file_length(connect, filename);
    if (rc < 0) {
        hive_set_error((int)rc);
        return -1;
    }

    return rc;
}

ssize_t hive_get_file_to_buffer(HiveConnect *connect, const char *filename,
                                  bool decrypt, void *to, size_t buflen)
{
    ssize_t rc;

    if (!connect || !filename || !*filename || !to || !buflen) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!connect->get_file_to_buffer) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = connect->get_file_to_buffer(connect, filename, decrypt, to, buflen);
    if (rc < 0) {
        hive_set_error((int)rc);
        return -1;
    }

    return rc;
}

ssize_t hive_get_file(HiveConnect *connect, const char *filename, bool decrypt,
                      const char *to)
{
    ssize_t fsize;
    uint8_t *buf;
    ssize_t nwr;
    int fd;

    if (!connect || !filename || !*filename || !to || !*to) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    fsize = hive_get_file_length(connect, filename);
    if (fsize < 0)
        return -1;

    fd = open(to, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        hive_set_error(HIVE_SYS_ERROR(errno));
        return -1;
    }

    if (!fsize) {
        close(fd);
        return 0;
    }

    buf = (uint8_t *)calloc(1, fsize);
    if (!buf) {
        close(fd);
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        return -1;
    }

    fsize = hive_get_file_to_buffer(connect, filename, decrypt, buf, (size_t)fsize);
    if (fsize < 0) {
        close(fd);
        free(buf);
        return -1;
    }

    nwr = write(fd, buf,
#if defined(_WIN32) || defined(_WIN64)
                (unsigned)
#endif
                fsize);
    free(buf);
    close(fd);

    if (nwr != fsize) {
        unlink(filename);
        hive_set_error(HIVE_SYS_ERROR(errno));
        return -1;
    }

    return fsize;
}

int hive_ipfs_put_file(HiveConnect *connect, const char *from, bool encrypt,
                       IPFSCid *cid)
{
    ssize_t nrd;
    uint8_t *buf;
    size_t fsize;
    int rc;
    int fd;

    if (!connect || !from || !*from || !cid) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    fd = open(from, O_RDONLY);
    if (fd < 0) {
        hive_set_error(HIVE_SYS_ERROR(errno));
        return -1;
    }

    fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    if (fsize > HIVE_MAX_FILE_SIZE) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_LIMIT_EXCEEDED));
        close(fd);
        return -1;
    }

    buf = (uint8_t *)calloc(1, fsize);
    if (!buf) {
        close(fd);
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        return -1;
    }

    nrd = read(fd, buf,
#if defined(_WIN32) || defined(_WIN64)
               (unsigned)
#endif
               fsize);
    close(fd);

    if (nrd != fsize) {
        free(buf);
        hive_set_error(HIVE_SYS_ERROR(errno));
        return -1;
    }

    rc = hive_ipfs_put_file_from_buffer(connect, buf, fsize, encrypt, cid);
    free(buf);

    return rc;
}

int hive_ipfs_put_file_from_buffer(HiveConnect *connect, const void *from,
                                   size_t length, bool encrypt, IPFSCid *cid)
{
    int rc;

    if (!connect || (!from && length) || length > HIVE_MAX_FILE_SIZE || !cid) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!connect->ipfs_put_file_from_buffer) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = connect->ipfs_put_file_from_buffer(connect, from, length, encrypt, cid);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

ssize_t hive_ipfs_get_file_length(HiveConnect *connect, const IPFSCid *cid)
{
    ssize_t rc;

    if (!connect || !cid || !cid->content[0]) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!connect->ipfs_get_file_length) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = connect->ipfs_get_file_length(connect, cid);
    if (rc < 0) {
        hive_set_error((int)rc);
        return -1;
    }

    return rc;
}

ssize_t hive_ipfs_get_file_to_buffer(HiveConnect *connect, const IPFSCid *cid,
                                     bool decrypt, void *to, size_t buflen)
{
    ssize_t rc;

    if (!connect || !cid || !cid->content[0] || !to || !buflen) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!connect->ipfs_get_file_to_buffer) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = connect->ipfs_get_file_to_buffer(connect, cid, decrypt, to, buflen);
    if (rc < 0) {
        hive_set_error((int)rc);
        return -1;
    }

    return rc;
}

ssize_t hive_ipfs_get_file(HiveConnect *connect, const IPFSCid *cid, bool decrypt,
                           const char *to)
{
    ssize_t fsize;
    uint8_t *buf;
    ssize_t nwr;
    int fd;

    if (!connect || !cid || !cid->content[0] || !to || !*to) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    fsize = hive_ipfs_get_file_length(connect, cid);
    if (fsize < 0)
        return -1;

    fd = open(to, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        hive_set_error(HIVE_SYS_ERROR(errno));
        return -1;
    }

    if (!fsize) {
        close(fd);
        return 0;
    }

    buf = (uint8_t *)calloc(1, fsize);
    if (!buf) {
        close(fd);
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        return -1;
    }

    fsize = connect->ipfs_get_file_to_buffer(connect, cid, decrypt, buf, fsize);
    if (fsize < 0) {
        close(fd);
        free(buf);
        hive_set_error((int)fsize);
        return -1;
    }

    nwr = write(fd, buf,
#if defined(_WIN32) || defined(_WIN64)
                (unsigned)
#endif
                fsize);
    free(buf);
    close(fd);

    if (nwr != fsize) {
        hive_set_error(HIVE_SYS_ERROR(errno));
        unlink(to);
        return -1;
    }

    return fsize;
}

int hive_delete_file(HiveConnect *connect, const char *filename)
{
    int rc;

    if (!connect || !filename || !*filename) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!connect->delete_file) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = connect->delete_file(connect, filename);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

int hive_list_files(HiveConnect *connect, HiveFilesIterateCallback *callback,
                    void *context)
{
    int rc;

    if (!connect || !callback) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!connect->list_files) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = connect->list_files(connect, callback, context);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

