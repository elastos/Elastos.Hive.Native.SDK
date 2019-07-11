#include <CUnit/Basic.h>
#include <ela_hive.h>
#include <config.h>

#include "test_context.h"
#include "test_helper.h"

typedef HiveClient *(*client_new_callback)();

static void test_client_new(void)
{
    HiveClient *client;
    client_new_callback client_new = (client_new_callback)test_ctx.ext;

    client = client_new();
    CU_ASSERT_PTR_NOT_NULL(client);
}

static CU_TestInfo cases[] = {
    { "test_client_new", test_client_new },
    { NULL, NULL }
};

CU_TestInfo *client_new_test_get_cases(void)
{
    return cases;
}

int onedrive_client_new_test_suite_init(void)
{
    test_ctx.ext = onedrive_client_new;

    return 0;
}

int onedrive_client_new_test_suite_cleanup(void)
{
    test_context_cleanup();

    return 0;
}

int ipfs_client_new_test_suite_init(void)
{
    test_ctx.ext = ipfs_client_new;

    return 0;
}

int ipfs_client_new_test_suite_cleanup(void)
{
    test_context_cleanup();

    return 0;
}
