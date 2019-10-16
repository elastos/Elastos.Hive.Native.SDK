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

#include <fcntl.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <sys/stat.h>
#include <crystal.h>
#endif

#include <ela_hive.h>
#include <CUnit/Basic.h>

#include "../test_context.h"
#include "../../config.h"

#if defined(_WIN32) || defined(_WIN64)
static int mkstemp(char *template)
{
    errno_t err;

    err = _mktemp_s(template, strlen(template) + 1);
    if (err) {
        errno = err;
        return -1;
    }

    return open(template, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
}
#endif

void ipfs_put_file_test(void)
{
    char from[1024];
    IPFSCid cid_tmp;
    ssize_t fsize;
    char buf[128];
    int nwr;
    int fd;
    int rc;
    IPFSCid cid = {
        .content = "Qmf412jQZiuVUtdgnB36FXFX7xg5V6KEbSJ4dpQuhkLyfD"
    };

    sprintf(from, "%s/XXXXXX", global_config.data_location);

    fd = mkstemp(from);
    if (fd < 0) {
        CU_FAIL("mkstemp failure.");
        return;
    }

    nwr = write(fd, "hello world",
#if defined(_WIN32) || defined(_WIN64)
                (unsigned)
#endif
                strlen("hello world"));
    close(fd);
    if (nwr != strlen("hello world")) {
        remove(from);
        CU_FAIL("write to file failure.");
        return;
    }

    rc = hive_ipfs_put_file(test_ctx.connect, from, true, &cid_tmp);
    remove(from);
    CU_ASSERT_TRUE_FATAL(rc == 0);
    CU_ASSERT_TRUE_FATAL(!strcmp(cid_tmp.content, cid.content));

    fsize = hive_ipfs_get_file_length(test_ctx.connect, &cid);
    CU_ASSERT_TRUE_FATAL(fsize == strlen("hello world"));

    memset(buf, 0, sizeof(buf));
    fsize = hive_ipfs_get_file_to_buffer(test_ctx.connect, &cid, true, buf, sizeof(buf));
    CU_ASSERT_TRUE_FATAL(fsize == strlen("hello world") && !strcmp(buf, "hello world"))
}

void ipfs_put_file_from_buffer_test(void)
{
    IPFSCid cid_tmp;
    ssize_t fsize;
    char to[1024];
    char buf[128];
    int nrd;
    int fd;
    int rc;
    IPFSCid cid = {
        .content = "QmTp2hEo8eXRp6wg7jXv1BLCMh5a4F3B7buAUZNZUu772j"
    };

    sprintf(to, "%s/XXXXXX", global_config.data_location);

    rc = hive_ipfs_put_file_from_buffer(test_ctx.connect, "hello world!",
                                        strlen("hello world!"), true, &cid_tmp);
    CU_ASSERT_TRUE_FATAL(rc == 0);
    CU_ASSERT_TRUE_FATAL(!strcmp(cid_tmp.content, cid.content));

    fsize = hive_ipfs_get_file_length(test_ctx.connect, &cid);
    CU_ASSERT_TRUE_FATAL(fsize == strlen("hello world!"));

    fsize = hive_ipfs_get_file(test_ctx.connect, &cid, true, to);
    CU_ASSERT_TRUE_FATAL(fsize == strlen("hello world!"));

    fd = open(to, O_RDONLY);
    if (fd < 0) {
        remove(to);
        CU_FAIL("open file failure.");
        return;
    }

    memset(buf, 0, sizeof(buf));
    nrd = read(fd, buf, sizeof(buf));
    close(fd);
    remove(to);
    CU_ASSERT_TRUE_FATAL(nrd == strlen("hello world!") && !strcmp(buf, "hello world!"));
}
