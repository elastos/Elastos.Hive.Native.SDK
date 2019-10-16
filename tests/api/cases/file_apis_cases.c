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

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
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

static bool list_files_cb(const char *filename, void *context)
{
    const char *fname = (const char *)(((void **)context)[0]);
    bool *fpresent = (bool *)(((void **)context)[1]);

    if (!filename)
        return true;

    if (!strcmp(filename, fname))
        *fpresent = true;

    return true;
}

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

static int __put_file_test(const char *to, const char *str)
{
    bool fpresent = false;
    char from[1024];
    ssize_t fsize;
    char *buf;
    int nwr;
    int fd;
    int rc;

    sprintf(from, "%s/XXXXXX", global_config.data_location);

    fd = mkstemp(from);
    if (fd < 0) {
        CU_FAIL("mkstemp failure.");
        return -1;
    }

    nwr = write(fd, str,
#if defined(_WIN32) || defined(_WIN64)
                (unsigned)
#endif
                strlen(str));
    close(fd);
    if (nwr != strlen(str)) {
        remove(from);
        CU_FAIL("write to file failure.");
        return -1;
    }

    rc = hive_put_file(test_ctx.connect, from, true, to);
    remove(from);
    if (rc < 0) {
        CU_FAIL("put file failure.");
        return -1;
    }

    fsize = hive_get_file_length(test_ctx.connect, to);
    if (fsize != strlen(str)) {
        hive_delete_file(test_ctx.connect, to);
        CU_FAIL("get file length failure.");
        return -1;
    }

    void *args[] = {(void *)to, &fpresent};
    rc = hive_list_files(test_ctx.connect, list_files_cb, args);
    if (rc < 0 || !fpresent) {
        hive_delete_file(test_ctx.connect, to);
        CU_FAIL("list files failure.");
        return -1;
    }

    buf = alloca(strlen(str) + 1);
    memset(buf, 0, strlen(str) + 1);
    fsize = hive_get_file_to_buffer(test_ctx.connect, to, true, buf, strlen(str));
    if (fsize != strlen(str) || strcmp(buf, str)) {
        hive_delete_file(test_ctx.connect, to);
        CU_FAIL("get file to buffer failure.");
        return -1;
    }

    return 0;
}

static int __put_file_from_buffer_test(const char *to, const char *str)
{
    bool fpresent = false;
    ssize_t fsize;
    char *buf;
    int rc;

    rc = hive_put_file_from_buffer(test_ctx.connect, str, strlen(str), true, to);
    if (rc < 0) {
        CU_FAIL("put file from buffer failure.");
        return -1;
    }

    fsize = hive_get_file_length(test_ctx.connect, to);
    if (fsize != strlen(str)) {
        hive_delete_file(test_ctx.connect, to);
        CU_FAIL("get file length failure.");
        return -1;
    }

    void *args[] = {(void *)to, &fpresent};
    rc = hive_list_files(test_ctx.connect, list_files_cb, args);
    if (rc < 0 || !fpresent) {
        hive_delete_file(test_ctx.connect, to);
        CU_FAIL("list files failure.");
        return -1;
    }

    buf = alloca(strlen(str) + 1);
    memset(buf, 0, strlen(str) + 1);
    fsize = hive_get_file_to_buffer(test_ctx.connect, to, true, buf, strlen(str));
    if (fsize != strlen(str) || strcmp(buf, str)) {
        hive_delete_file(test_ctx.connect, to);
        CU_FAIL("get file to buffer failure.");
        return -1;
    }

    return 0;
}

static int __delete_file_test(const char *fname)
{
    bool fpresent = false;
    int rc;

    rc = hive_delete_file(test_ctx.connect, fname);
    if (rc < 0) {
        CU_FAIL("delete file failure.");
        return -1;
    }

    void *args[] = {(void *)fname, &fpresent};
    rc = hive_list_files(test_ctx.connect, list_files_cb, args);
    if (rc < 0 || fpresent) {
        CU_FAIL("list files failure.");
        return -1;
    }

    return 0;
}

void put_file_test(void)
{
    int rc;

    rc = __put_file_test("test.txt", "hello world");
    CU_ASSERT_TRUE_FATAL(rc == 0);

    __delete_file_test("test.txt");
}

void double_put_file_test(void)
{
    int rc;

    rc = __put_file_test("test.txt", "hello world");
    CU_ASSERT_TRUE_FATAL(rc == 0);

    rc = __put_file_test("test.txt", "hello world");
    CU_ASSERT_TRUE(rc == 0);

    rc = hive_delete_file(test_ctx.connect, "test.txt");
    CU_ASSERT_TRUE(rc == 0);
}

void put_file_from_buffer_test(void)
{
    int rc;

    rc = __put_file_from_buffer_test("test.txt", "hello world");
    CU_ASSERT_TRUE_FATAL(rc == 0);

    rc = hive_delete_file(test_ctx.connect, "test.txt");
    CU_ASSERT_TRUE(rc == 0);
}

void double_put_file_from_buffer_test(void)
{
    int rc;

    rc = __put_file_from_buffer_test("test.txt", "hello world");
    CU_ASSERT_TRUE_FATAL(rc == 0);

    rc = __put_file_from_buffer_test("test.txt", "hello world");
    CU_ASSERT_TRUE(rc == 0);

    rc = hive_delete_file(test_ctx.connect, "test.txt");
    CU_ASSERT_TRUE(rc == 0);
}

void get_nonexist_file_length_test(void)
{
    ssize_t fsize;

    fsize = hive_get_file_length(test_ctx.connect, "nonexist");
    CU_ASSERT_TRUE(fsize < 0);
}

void get_nonexist_file_to_buffer_test(void)
{
    ssize_t fsize;
    char buf[1024];

    fsize = hive_get_file_to_buffer(test_ctx.connect, "nonexist", true, buf, sizeof(buf));
    CU_ASSERT_TRUE(fsize < 0);
}

void get_file_test(void)
{
    char to[1024];
    char buf[128];
    ssize_t fsize;
    int nrd;
    int rc;
    int fd;

    sprintf(to, "%s/test.txt", global_config.data_location);

    rc = __put_file_from_buffer_test("test.txt", "hello world");
    CU_ASSERT_TRUE_FATAL(rc == 0);

    fsize = hive_get_file(test_ctx.connect, "test.txt", true, to);
    rc = hive_delete_file(test_ctx.connect, "test.txt");
    CU_ASSERT_TRUE(rc == 0);
    CU_ASSERT_TRUE_FATAL(fsize == strlen("hello world"));

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
    CU_ASSERT_TRUE(nrd == strlen("hello world") && !strcmp(buf, "hello world"));
}

void get_nonexist_file_test(void)
{
    char to[1024];
    ssize_t fsize;

    sprintf(to, "%s/XXXXXX", global_config.data_location);

    fsize = hive_get_file(test_ctx.connect, "nonexist", true, to);
    CU_ASSERT_TRUE(fsize < 0);
}

void delete_nonexist_file_test(void)
{
    int rc;

    rc = hive_delete_file(test_ctx.connect, "nonexist");
    CU_ASSERT_TRUE(rc < 0);
}
