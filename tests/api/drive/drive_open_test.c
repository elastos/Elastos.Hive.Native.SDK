#include <CUnit/Basic.h>
#include <ela_hive.h>

#include "config.h"
#include "test_context.h"
#include "test_helper.h"

static void test_drive_open(void)
{
    HiveDrive *drive;

    drive = hive_drive_open(test_ctx.client);
    CU_ASSERT_PTR_NOT_NULL(drive);
}

static CU_TestInfo cases[] = {
    { "test_drive_open", test_drive_open },
    { NULL, NULL }
};

CU_TestInfo *drive_open_test_get_cases(void)
{
    return cases;
}

int onedrive_drive_open_test_suite_init(void)
{
    int rc;

    test_ctx.client = onedrive_client_new();
    if (!test_ctx.client)
        return -1;

    rc = hive_client_login(test_ctx.client, open_authorization_url, NULL);
    if (rc < 0)
        return -1;

    return 0;
}

int onedrive_drive_open_test_suite_cleanup(void)
{
    test_context_cleanup();

    return 0;
}

int ipfs_drive_open_test_suite_init(void)
{
    int rc;

    test_ctx.client = ipfs_client_new();
    if (!test_ctx.client)
        return -1;

    rc = hive_client_login(test_ctx.client, NULL, NULL);
    if (rc < 0)
        return -1;

    return 0;
}

int ipfs_drive_open_test_suite_cleanup(void)
{
    test_context_cleanup();

    return 0;
}
