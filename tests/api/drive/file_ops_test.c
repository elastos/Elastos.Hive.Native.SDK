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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <crystal.h>
#endif

#include <CUnit/Basic.h>
#include <ela_hive.h>

#include "config.h"
#include "test_context.h"
#include "test_helper.h"

static char working_dir_name[PATH_MAX];

static dir_entry onedrive_dir_entries[] = {
    ONEDRIVE_DIR_ENTRY("test", "directory"),
    ONEDRIVE_DIR_ENTRY("test2", "directory")
};
static dir_entry ipfs_dir_entries[] = {
    IPFS_DIR_ENTRY("test", "directory"),
    IPFS_DIR_ENTRY("test2", "directory")
};

static bool list_nonexist_dir_cb(const KeyValue *info, size_t size, void *context)
{
    (void)info;
    (void)size;
    (void)context;

    return true;
}

static void test_mkdir(void)
{
    int rc;
    dir_entry *entries = (dir_entry *)test_ctx.ext;
    char dir_name[PATH_MAX];

    snprintf(dir_name, sizeof(dir_name), "%s/test", working_dir_name);

    rc = hive_drive_mkdir(test_ctx.drive, dir_name);
    CU_ASSERT_FATAL(rc == HIVEOK);

    list_files_test_scheme(test_ctx.drive, working_dir_name, entries, 1);

    rc = hive_drive_delete_file(test_ctx.drive, dir_name);
    CU_ASSERT(rc == HIVEOK);
}

static void test_mv_file(void)
{
    int rc;
    dir_entry *entries = (dir_entry *)test_ctx.ext;
    char dir_name1[PATH_MAX];
    char dir_name2[PATH_MAX];

    snprintf(dir_name1, sizeof(dir_name1), "%s/test", working_dir_name);
    snprintf(dir_name2, sizeof(dir_name2), "%s/test2", working_dir_name);

    rc = hive_drive_mkdir(test_ctx.drive, dir_name1);
    CU_ASSERT_FATAL(rc == HIVEOK);

    rc = list_files_test_scheme(test_ctx.drive, working_dir_name, entries, 1);
    if (rc < 0) {
        CU_FAIL("unexpected entries in dir");
        hive_drive_delete_file(test_ctx.drive, dir_name1);
        return;
    }

    rc = hive_drive_move_file(test_ctx.drive, dir_name1, dir_name2);
    if (rc < 0) {
        CU_FAIL("failed to move file");
        hive_drive_delete_file(test_ctx.drive, dir_name1);
        return;
    }

    list_files_test_scheme(test_ctx.drive, working_dir_name, entries + 1, 1);

    rc = hive_drive_delete_file(test_ctx.drive, dir_name2);
    CU_ASSERT(rc == HIVEOK);
}

static void test_mv_file_nonexist(void)
{
    int rc;
    char dir_name1[PATH_MAX];
    char dir_name2[PATH_MAX];

    snprintf(dir_name1, sizeof(dir_name1), "%s", get_random_file_name());
    snprintf(dir_name2, sizeof(dir_name2), "%s", get_random_file_name());

    rc = hive_drive_move_file(test_ctx.drive, dir_name1, dir_name2);
    CU_ASSERT_FATAL(rc < 0);
}

static void test_cp_file(void)
{
    int rc;
    dir_entry *entries = (dir_entry *)test_ctx.ext;
    char dir_name1[PATH_MAX];
    char dir_name2[PATH_MAX];

    snprintf(dir_name1, sizeof(dir_name1), "%s/test", working_dir_name);
    snprintf(dir_name2, sizeof(dir_name2), "%s/test2", working_dir_name);

    rc = hive_drive_mkdir(test_ctx.drive, dir_name1);
    CU_ASSERT_FATAL(rc == HIVEOK);

    rc = list_files_test_scheme(test_ctx.drive, working_dir_name, entries, 1);
    if (rc < 0) {
        CU_FAIL("unexpected entries in dir");
        hive_drive_delete_file(test_ctx.drive, dir_name1);
        return;
    }

    rc = hive_drive_copy_file(test_ctx.drive, dir_name1, dir_name2);
    if (rc < 0) {
        CU_FAIL("failed to copy file");
        hive_drive_delete_file(test_ctx.drive, dir_name1);
        return;
    }

    sleep(5);

    list_files_test_scheme(test_ctx.drive, working_dir_name, entries, 2);

    rc = hive_drive_delete_file(test_ctx.drive, dir_name1);
    CU_ASSERT(rc == HIVEOK);

    rc = hive_drive_delete_file(test_ctx.drive, dir_name2);
    CU_ASSERT(rc == HIVEOK);
}

static void test_cp_file_nonexist(void)
{
    int rc;
    char dir_name1[PATH_MAX];
    char dir_name2[PATH_MAX];

    snprintf(dir_name1, sizeof(dir_name1), "%s", get_random_file_name());
    snprintf(dir_name2, sizeof(dir_name2), "%s", get_random_file_name());

    rc = hive_drive_copy_file(test_ctx.drive, dir_name1, dir_name2);
    CU_ASSERT_FATAL(rc < 0);
}

static void test_rm_file_nonexist(void)
{
    int rc;
    char dir_name[PATH_MAX];

    snprintf(dir_name, sizeof(dir_name), "%s", get_random_file_name());

    rc = hive_drive_delete_file(test_ctx.drive, dir_name);
    CU_ASSERT_FATAL(rc < 0);
}

static void test_stat_file(void)
{
    int rc;
    HiveFileInfo info;

    rc = hive_drive_file_stat(test_ctx.drive, working_dir_name, &info);
    CU_ASSERT_FATAL(rc == HIVEOK);

    CU_ASSERT(!strcmp(info.type, "directory"));
}

static void test_stat_file_nonexist(void)
{
    int rc;
    char dir_name[PATH_MAX];
    HiveFileInfo info;

    snprintf(dir_name, sizeof(dir_name), "%s", get_random_file_name());

    rc = hive_drive_file_stat(test_ctx.drive, dir_name, &info);
    CU_ASSERT_FATAL(rc < 0);
}

static CU_TestInfo cases[] = {
    { "test_mkdir"             , test_mkdir              },
    { "test_mv_file"           , test_mv_file            },
    { "test_mv_file_nonexist"  , test_mv_file_nonexist   },
    { "test_cp_file"           , test_cp_file            },
    { "test_cp_file_nonexist"  , test_cp_file_nonexist   },
    { "test_rm_file_nonexist"  , test_rm_file_nonexist   },
    { "test_stat_file"         , test_stat_file          },
    { "test_stat_file_nonexist", test_stat_file_nonexist },
    { NULL, NULL }
};

CU_TestInfo *file_ops_test_get_cases(void)
{
    return cases;
}

int onedrive_file_ops_test_suite_init(void)
{
    int rc;

    test_ctx.ext = onedrive_dir_entries;

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

int onedrive_file_ops_test_suite_cleanup(void)
{
    int rc;

    rc = hive_drive_delete_file(test_ctx.drive, working_dir_name);
    if (rc < 0)
        return -1;

    test_context_cleanup();

    return 0;
}

int ipfs_file_ops_test_suite_init(void)
{
    int rc;

    test_ctx.ext = ipfs_dir_entries;

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

int ipfs_file_ops_test_suite_cleanup(void)
{
    int rc;

    rc = hive_drive_delete_file(test_ctx.drive, working_dir_name);
    if (rc < 0)
        return -1;

    test_context_cleanup();

    return 0;
}
