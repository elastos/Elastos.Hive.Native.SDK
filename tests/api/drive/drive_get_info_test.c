#include <CUnit/Basic.h>
#include <ela_hive.h>

#include "config.h"
#include "test_context.h"
#include "test_helper.h"

static void test_drive_get_info(void)
{
    HiveDriveInfo info;
    int rc;

    rc = hive_drive_get_info(test_ctx.drive, &info);
    CU_ASSERT_FATAL(rc == HIVEOK);
}

static CU_TestInfo cases[] = {
    { "test_drive_get_info", test_drive_get_info },
    { NULL, NULL }
};

CU_TestInfo *drive_get_info_test_get_cases(void)
{
    return cases;
}

int onedrive_drive_get_info_test_suite_init(void)
{
    int rc;

    test_ctx.client = onedrive_client_new();
    if (!test_ctx.client)
        return -1;

    rc = hive_client_login(test_ctx.client, open_authorization_url, NULL);
    if (rc < 0)
        return -1;

    test_ctx.drive = hive_drive_open(test_ctx.client);
    if (!test_ctx.drive)
        return -1;

    return 0;
}

int onedrive_drive_get_info_test_suite_cleanup(void)
{
    test_context_cleanup();

    return 0;
}

int ipfs_drive_get_info_test_suite_init(void)
{
    int rc;

    test_ctx.client = ipfs_client_new();
    if (!test_ctx.client)
        return -1;

    rc = hive_client_login(test_ctx.client, NULL, NULL);
    if (rc < 0)
        return -1;

    test_ctx.drive = hive_drive_open(test_ctx.client);
    if (!test_ctx.drive)
        return -1;

    return 0;
}

int ipfs_drive_get_info_test_suite_cleanup(void)
{
    test_context_cleanup();

    return 0;
}
