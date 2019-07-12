#include <CUnit/Basic.h>
#include <ela_hive.h>
#include <config.h>
#include <limits.h>

#if defined(_WIN32) || defined(_WIN64)
#include <crystal.h>
#endif

#include "test_context.h"
#include "test_helper.h"

static char working_dir_name[PATH_MAX];

static dir_entry onedrive_dir_entry = ONEDRIVE_DIR_ENTRY("test", "directory");
static dir_entry ipfs_dir_entry = IPFS_DIR_ENTRY("test", "directory");

static bool list_nonexist_dir_cb(const KeyValue *info, size_t size, void *context)
{
    (void)info;
    (void)size;
    (void)context;

    return true;
}

static void test_list_nonexist_dir(void)
{
    int rc;

    rc = hive_drive_list_files(test_ctx.drive, get_random_file_name(),
                               list_nonexist_dir_cb, NULL);
    CU_ASSERT(rc == -1);
}

static void test_list_empty_dir(void)
{
    list_files_test_scheme(test_ctx.drive, working_dir_name, NULL, 0);
}

static void test_list_dir_with_entry(void)
{
    int rc;
    char dir_name[PATH_MAX];
    dir_entry *entry = (dir_entry *)test_ctx.ext;

    snprintf(dir_name, sizeof(dir_name), "%s/test", working_dir_name);

    rc = hive_drive_mkdir(test_ctx.drive, dir_name);
    CU_ASSERT_FATAL(rc == HIVEOK);

    list_files_test_scheme(test_ctx.drive, working_dir_name, entry, 1);

    rc = hive_drive_delete_file(test_ctx.drive, dir_name);
    CU_ASSERT(rc == HIVEOK);
}

static CU_TestInfo cases[] = {
    { "test_list_nonexist_dir"   , test_list_nonexist_dir   },
    { "test_list_empty_dir"      , test_list_empty_dir      },
    { "test_list_dir_with_entry" , test_list_dir_with_entry },
    { NULL, NULL }
};

CU_TestInfo *list_files_test_get_cases(void)
{
    return cases;
}

int onedrive_list_files_test_suite_init(void)
{
    int rc;

    test_ctx.ext = &onedrive_dir_entry;

    test_ctx.client = onedrive_client_new();
    if (!test_ctx.client) {
        CU_FAIL("Error: test suite initialize error");
        return -1;
    }

    rc = hive_client_login(test_ctx.client, open_authorization_url, NULL);
    if (rc < 0) {
        CU_FAIL("Error: test suite initialize error");
        return -1;
    }

    test_ctx.drive = hive_drive_open(test_ctx.client);
    if (!test_ctx.drive) {
        CU_FAIL("Error: test suite initialize error");
        return -1;
    }

    strcpy(working_dir_name, get_random_file_name());

    rc = hive_drive_mkdir(test_ctx.drive, working_dir_name);
    if (rc < 0) {
        CU_FAIL("Error: test suite initialize error");
        return -1;
    }

    return 0;
}

int onedrive_list_files_test_suite_cleanup(void)
{
    int rc;

    rc = hive_drive_delete_file(test_ctx.drive, working_dir_name);
    if (rc < 0) {
        CU_FAIL("Error: test suite initialize error");
        return -1;
    }

    test_context_cleanup();

    return 0;
}

int ipfs_list_files_test_suite_init(void)
{
    int rc;

    test_ctx.ext = &ipfs_dir_entry;

    test_ctx.client = ipfs_client_new();
    if (!test_ctx.client) {
        CU_FAIL("Error: test suite initialize error");
        return -1;
    }

    rc = hive_client_login(test_ctx.client, NULL, NULL);
    if (rc < 0) {
        CU_FAIL("Error: test suite initialize error");
        return -1;
    }

    test_ctx.drive = hive_drive_open(test_ctx.client);
    if (!test_ctx.drive) {
        CU_FAIL("Error: test suite initialize error");
        return -1;
    }

    strcpy(working_dir_name, get_random_file_name());

    rc = hive_drive_mkdir(test_ctx.drive, working_dir_name);
    if (rc < 0) {
        CU_FAIL("Error: test suite initialize error");
        return -1;
    }

    return 0;
}

int ipfs_list_files_test_suite_cleanup(void)
{
    int rc;

    rc = hive_drive_delete_file(test_ctx.drive, working_dir_name);
    if (rc < 0) {
        CU_FAIL("Error: test suite initialize error");
        return -1;
    }

    test_context_cleanup();

    return 0;
}
