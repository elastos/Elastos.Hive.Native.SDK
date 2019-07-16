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

#include <limits.h>

#if defined(_WIN32) || defined(_WIN64)
#include <crystal.h>
#endif

#include <CUnit/Basic.h>
#include <ela_hive.h>

#include "config.h"
#include "test_context.h"
#include "test_helper.h"

static char working_dir_name[PATH_MAX];

typedef struct {
    dir_entry entry;
    ssize_t (*write_callback)(HiveFile *file, const char *buf, size_t bufsz);
    void (*test_file_commit)();
} extension;

static ssize_t onedrive_file_write(HiveFile *file, const char *buf, size_t bufsz);
static void onedrive_test_file_commit(void);
static void ipfs_test_file_commit(void);

static extension onedrive_ext = {
    .entry = ONEDRIVE_DIR_ENTRY("test", "file"),
    .write_callback = onedrive_file_write,
    .test_file_commit = onedrive_test_file_commit
};

static extension ipfs_ext = {
    .entry = IPFS_DIR_ENTRY("test", "file"),
    .write_callback = hive_file_write,
    .test_file_commit = ipfs_test_file_commit
};

static ssize_t onedrive_file_write(HiveFile *file, const char *buf, size_t bufsz)
{
    ssize_t nwr;
    int rc;

    nwr = hive_file_write(file, buf, bufsz);
    if (nwr < 0)
        return nwr;

    rc = hive_file_commit(file);
    if (rc < 0)
        return (ssize_t)rc;

    return nwr;
}

static void test_open_file_mode_rwa(void)
{
    int rc;
    ssize_t nwr, nrd;
    char file_path[PATH_MAX];
    HiveFile *file;
    char buf[1024];
    extension *ext = (extension *)test_ctx.ext;

    snprintf(file_path, sizeof(file_path), "%s/test", working_dir_name);

    // test mode "r" when file does not exist
    file = hive_file_open(test_ctx.drive, file_path, "r");
    if (file) {
        CU_FAIL("hive_file_open() failed");
        hive_file_close(file);
        return;
    }

    // test mode "w" when file does not exist
    file = hive_file_open(test_ctx.drive, file_path, "w");
    CU_ASSERT_PTR_NOT_NULL_FATAL(file);

    nwr = ext->write_callback(file, "hello world!", strlen("hello world!"));
    if (nwr != strlen("hello world!")) {
        CU_FAIL("hive_file_write() failed");
        hive_file_close(file);
        return;
    }

    nrd = hive_file_read(file, buf, sizeof(buf));
    hive_file_close(file);
    if (nrd >= 0) {
        CU_FAIL("hive_file_read() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    rc = list_files_test_scheme(test_ctx.drive, working_dir_name, &ext->entry, 1);
    if (rc < 0) {
        CU_FAIL("list_files_test_scheme() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    // test mode "r" when file exists
    file = hive_file_open(test_ctx.drive, file_path, "r");
    if (!file) {
        CU_FAIL("hive_file_open() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nrd = hive_file_read(file, buf, sizeof(buf));
    if (nrd != strlen("hello world!") ||
        strncmp(buf, "hello world!", strlen("hello world!"))) {
        CU_FAIL("hive_file_read() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nwr = ext->write_callback(file, "hello world!", strlen("hello world!"));
    hive_file_close(file);
    if (nwr >= 0) {
        CU_FAIL("hive_file_write() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    // test mode "w" when file exists
    file = hive_file_open(test_ctx.drive, file_path, "w");
    if (!file) {
        CU_FAIL("hive_file_open() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nwr = ext->write_callback(file, "hello world!", strlen("hello world!"));
    hive_file_close(file);
    if (nwr != strlen("hello world!")) {
        CU_FAIL("hive_file_write() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    // test mode "a" when file exists
    file = hive_file_open(test_ctx.drive, file_path, "a");
    if (!file) {
        CU_FAIL("hive_file_open() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nwr = ext->write_callback(file, "!!", 2);
    if (nwr != 2) {
        CU_FAIL("hive_file_write() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nrd = hive_file_read(file, buf, sizeof(buf));
    hive_file_close(file);
    if (nrd >= 0) {
        CU_FAIL("hive_file_read() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    file = hive_file_open(test_ctx.drive, file_path, "r");
    if (!file) {
        CU_FAIL("hive_file_open() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nrd = hive_file_read(file, buf, sizeof(buf));
    hive_file_close(file);
    hive_drive_delete_file(test_ctx.drive, file_path);
    if (nrd != strlen("hello world!!!") ||
        strncmp(buf, "hello world!!!", strlen("hello world!!!"))) {
        CU_FAIL("hive_file_read() failed");
        return;
    }

    // test mode "a" when file does not exist
    file = hive_file_open(test_ctx.drive, file_path, "a");
    if (!file) {
        CU_FAIL("hive_file_open() failed");
        return;
    }

    nwr = ext->write_callback(file, "hello world!", strlen("hello world!"));
    hive_file_close(file);
    if (nwr != strlen("hello world!")) {
        CU_FAIL("hive_file_write() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    file = hive_file_open(test_ctx.drive, file_path, "r");
    if (!file) {
        CU_FAIL("hive_file_open() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nrd = hive_file_read(file, buf, sizeof(buf));
    hive_file_close(file);
    hive_drive_delete_file(test_ctx.drive, file_path);
    if (nrd != strlen("hello world!") ||
        strncmp(buf, "hello world!", strlen("hello world!"))) {
        CU_FAIL("hive_file_read() failed");
        return;
    }
}

static void test_open_file_mode_rwa_plus(void)
{
    int rc;
    ssize_t nwr, nrd, lpos;
    char file_path[PATH_MAX];
    HiveFile *file;
    char buf[1024];
    extension *ext = (extension *)test_ctx.ext;

    snprintf(file_path, sizeof(file_path), "%s/test", working_dir_name);

    // test mode "r+" when file does not exist
    file = hive_file_open(test_ctx.drive, file_path, "r+");
    if (file) {
        CU_FAIL("hive_file_open() failed");
        hive_file_close(file);
        return;
    }

    // test mode "w+" when file does not exist
    file = hive_file_open(test_ctx.drive, file_path, "w+");
    CU_ASSERT_PTR_NOT_NULL_FATAL(file);

    nwr = ext->write_callback(file, "hello world!", strlen("hello world!"));
    if (nwr != strlen("hello world!")) {
        CU_FAIL("hive_file_write() failed");
        hive_file_close(file);
        return;
    }

    lpos = hive_file_seek(file, 0, HiveSeek_Set);
    if (lpos != 0) {
        CU_FAIL("hive_file_seek() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nrd = hive_file_read(file, buf, sizeof(buf));
    hive_file_close(file);
    if (nrd != strlen("hello world!") ||
        strncmp(buf, "hello world!", strlen("hello world!"))) {
        CU_FAIL("hive_file_read() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    rc = list_files_test_scheme(test_ctx.drive, working_dir_name, &ext->entry, 1);
    if (rc < 0) {
        CU_FAIL("list_files_test_scheme() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    // test mode "r+" when file exists
    file = hive_file_open(test_ctx.drive, file_path, "r+");
    if (!file) {
        CU_FAIL("hive_file_open() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nrd = hive_file_read(file, buf, sizeof(buf));
    if (nrd != strlen("hello world!") ||
        strncmp(buf, "hello world!", strlen("hello world!"))) {
        CU_FAIL("hive_file_read() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    lpos = hive_file_seek(file, 0, HiveSeek_Set);
    if (lpos != 0) {
        CU_FAIL("hive_file_seek() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nwr = ext->write_callback(file, "hello world!", strlen("hello world!"));
    if (nwr != strlen("hello world!")) {
        CU_FAIL("hive_file_write() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    lpos = hive_file_seek(file, 0, HiveSeek_Set);
    if (lpos != 0) {
        CU_FAIL("hive_file_seek() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nrd = hive_file_read(file, buf, sizeof(buf));
    hive_file_close(file);
    if (nrd != strlen("hello world!") ||
        strncmp(buf, "hello world!", strlen("hello world!"))) {
        CU_FAIL("hive_file_read() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    // test mode "w+" when file exists
    file = hive_file_open(test_ctx.drive, file_path, "w+");
    if (!file) {
        CU_FAIL("hive_file_open() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nwr = ext->write_callback(file, "hello world!", strlen("hello world!"));
    hive_file_close(file);
    if (nwr != strlen("hello world!")) {
        CU_FAIL("hive_file_write() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    // test mode "a+" when file exists
    file = hive_file_open(test_ctx.drive, file_path, "a+");
    if (!file) {
        CU_FAIL("hive_file_open() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nwr = ext->write_callback(file, "!!", 2);
    if (nwr != 2) {
        CU_FAIL("hive_file_write() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    lpos = hive_file_seek(file, 0, HiveSeek_Set);
    if (lpos != 0) {
        CU_FAIL("hive_file_seek() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nrd = hive_file_read(file, buf, sizeof(buf));
    hive_file_close(file);
    hive_drive_delete_file(test_ctx.drive, file_path);
    if (nrd != strlen("hello world!!!") ||
        strncmp(buf, "hello world!!!", strlen("hello world!!!"))) {
        CU_FAIL("hive_file_read() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    // test mode "a+" when file does not exist
    file = hive_file_open(test_ctx.drive, file_path, "a+");
    if (!file) {
        CU_FAIL("hive_file_open() failed");
        return;
    }

    nwr = ext->write_callback(file, "hello world!", strlen("hello world!"));
    if (nwr != strlen("hello world!")) {
        CU_FAIL("hive_file_write() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    lpos = hive_file_seek(file, 0, HiveSeek_Set);
    if (lpos != 0) {
        CU_FAIL("hive_file_seek() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nrd = hive_file_read(file, buf, sizeof(buf));
    hive_file_close(file);
    hive_drive_delete_file(test_ctx.drive, file_path);
    if (nrd != strlen("hello world!") ||
        strncmp(buf, "hello world!", strlen("hello world!"))) {
        CU_FAIL("hive_file_read() failed");
        return;
    }
}

static void test_file_seek(void)
{
    ssize_t nwr, nrd, lpos;
    char file_path[PATH_MAX];
    HiveFile *file;
    char buf[1024];
    extension *ext = (extension *)test_ctx.ext;

    snprintf(file_path, sizeof(file_path), "%s/test", working_dir_name);

    file = hive_file_open(test_ctx.drive, file_path, "w+");
    CU_ASSERT_PTR_NOT_NULL_FATAL(file);

    nwr = ext->write_callback(file, "hello world!", strlen("hello world!"));
    if (nwr != strlen("hello world!")) {
        CU_FAIL("hive_file_write() failed");
        hive_file_close(file);
        return;
    }

    lpos = hive_file_seek(file, 0, HiveSeek_Set);
    if (lpos != 0) {
        CU_FAIL("hive_file_seek() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nrd = hive_file_read(file, buf, sizeof(buf));
    if (nrd != strlen("hello world!") ||
        strncmp(buf, "hello world!", strlen("hello world!"))) {
        CU_FAIL("hive_file_read() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    lpos = hive_file_seek(file, 1, HiveSeek_Set);
    if (lpos != 1) {
        CU_FAIL("hive_file_seek() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    lpos = hive_file_seek(file, strlen("hello ") - 1, HiveSeek_Cur);
    if (lpos != strlen("hello ")) {
        CU_FAIL("hive_file_seek() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nrd = hive_file_read(file, buf, sizeof(buf));
    if (nrd != strlen("world!") ||
        strncmp(buf, "world!", strlen("world!"))) {
        CU_FAIL("hive_file_read() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    lpos = hive_file_seek(file, -strlen("world!"), HiveSeek_End);
    if (lpos != strlen("hello ")) {
        CU_FAIL("hive_file_seek() failed");
        hive_file_close(file);
        hive_drive_delete_file(test_ctx.drive, file_path);
        return;
    }

    nrd = hive_file_read(file, buf, sizeof(buf));
    hive_file_close(file);
    hive_drive_delete_file(test_ctx.drive, file_path);
    if (nrd != strlen("world!") ||
        strncmp(buf, "world!", strlen("world!"))) {
        CU_FAIL("hive_file_read() failed");
        return;
    }
}

static void onedrive_test_file_commit(void)
{
    int rc;
    ssize_t nwr, nrd;
    char file_path[PATH_MAX];
    HiveFile *file;
    char buf[1024];

    snprintf(file_path, sizeof(file_path), "%s/test", working_dir_name);

    file = hive_file_open(test_ctx.drive, file_path, "w+");
    CU_ASSERT_PTR_NOT_NULL_FATAL(file);

    nwr = hive_file_write(file, "hello world!", strlen("hello world!"));
    if (nwr != strlen("hello world!")) {
        CU_FAIL("hive_file_write() failed");
        hive_file_close(file);
        return;
    }

    rc = hive_file_commit(file);
    if (rc < 0) {
        CU_FAIL("hive_file_commit() failed");
        hive_file_close(file);
        return;
    }

    nwr = hive_file_write(file, "!!", 2);
    if (nwr != 2) {
        CU_FAIL("hive_file_write() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        hive_file_close(file);
        return;
    }

    rc = hive_file_discard(file);
    if (rc < 0) {
        CU_FAIL("hive_file_discard() failed");
        hive_drive_delete_file(test_ctx.drive, file_path);
        hive_file_close(file);
        return;
    }

    nrd = hive_file_read(file, buf, sizeof(buf));
    hive_file_close(file);
    hive_drive_delete_file(test_ctx.drive, file_path);
    if (nrd != strlen("hello world!") ||
        strncmp(buf, "hello world!", strlen("hello world!"))) {
        CU_FAIL("hive_file_read() failed");
        return;
    }
}

static void ipfs_test_file_commit(void)
{
    int rc;
    char file_path[PATH_MAX];
    HiveFile *file;

    snprintf(file_path, sizeof(file_path), "%s/test", working_dir_name);

    file = hive_file_open(test_ctx.drive, file_path, "w+");
    CU_ASSERT_PTR_NOT_NULL_FATAL(file);

    rc = hive_file_commit(file);
    if (rc == 0) {
        CU_FAIL("hive_file_commit() failed");
        hive_file_close(file);
        return;
    }

    rc = hive_file_discard(file);
    hive_file_close(file);
    if (rc == 0) {
        CU_FAIL("hive_file_discard() failed");
        return;
    }
}

static void test_file_commit(void)
{
    extension *ext = (extension *)test_ctx.ext;
    void (*test_file_commit_cb)() = ext->test_file_commit;

    test_file_commit_cb();
}

static CU_TestInfo cases[] = {
    { "test_open_file_mode_rwa"     , test_open_file_mode_rwa      },
    { "test_open_file_mode_rwa_plus", test_open_file_mode_rwa_plus },
    { "test_file_seek"              , test_file_seek               },
    { "test_file_commit"            , test_file_commit             },
    { NULL, NULL }
};

CU_TestInfo *file_apis_test_get_cases(void)
{
    return cases;
}

int onedrive_file_apis_test_suite_init(void)
{
    int rc;

    test_ctx.ext = &onedrive_ext;

    test_ctx.client = onedrive_client_new();
    if (!test_ctx.client)
        return -1;

    rc = hive_client_login(test_ctx.client, open_authorization_url, NULL);
    if (rc < 0)
        return -1;

    test_ctx.drive = hive_drive_open(test_ctx.client);
    if (!test_ctx.drive)
        return -1;

    strcpy(working_dir_name, get_random_file_name());

    rc = hive_drive_mkdir(test_ctx.drive, working_dir_name);
    if (rc < 0)
        return -1;

    return 0;
}

int onedrive_file_apis_test_suite_cleanup(void)
{
    int rc;

    rc = hive_drive_delete_file(test_ctx.drive, working_dir_name);
    if (rc < 0)
        return -1;

    test_context_cleanup();

    return 0;
}

int ipfs_file_apis_test_suite_init(void)
{
    int rc;

    test_ctx.ext = &ipfs_ext;

    test_ctx.client = ipfs_client_new();
    if (!test_ctx.client)
        return -1;

    rc = hive_client_login(test_ctx.client, NULL, NULL);
    if (rc < 0)
        return -1;

    test_ctx.drive = hive_drive_open(test_ctx.client);
    if (!test_ctx.drive)
        return -1;

    strcpy(working_dir_name, get_random_file_name());

    rc = hive_drive_mkdir(test_ctx.drive, working_dir_name);
    if (rc < 0)
        return -1;

    return 0;
}

int ipfs_file_apis_test_suite_cleanup(void)
{
    int rc;

    rc = hive_drive_delete_file(test_ctx.drive, working_dir_name);
    if (rc < 0)
        return -1;

    test_context_cleanup();

    return 0;
}

