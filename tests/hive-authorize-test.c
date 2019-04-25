#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>

#include <hive.h>

static hive_1drv_opt_t onedrv_option;

static
void onedrv_open_oauth_url(const char *url)
{

    ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    printf("***onedrv_open_oauth_url\n");
    return;
}

static void test_hive_authorize(void)
{
    int rc;
    hive_t *hive = NULL;

    hive = hive_new((hive_opt_t *)(&onedrv_option));
    CU_ASSERT_PTR_NOT_NULL_FATAL(hive);

    rc = hive_authorize(hive);
    CU_ASSERT_EQUAL(rc, 0);

    hive_kill(hive);
    return;
}

static void test_hive_authorize_without_new(void)
{
    int rc;
    hive_t *hive = NULL;

    rc = hive_authorize(hive);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static int hive_authorize_test_suite_init(void)
{
    onedrv_option.base.type = HIVE_TYPE_ONEDRIVE;
    strcpy(onedrv_option.profile_path, "hive1drv.json");
    onedrv_option.open_oauth_url = onedrv_open_oauth_url;

    return hive_global_init();
}

static int hive_authorize_test_suite_cleanup(void)
{
    onedrv_option.base.type = HIVE_TYPE_NONEXIST;
    strcpy(onedrv_option.profile_path, "");

    hive_global_cleanup();

    return 0;
}

static CU_TestInfo cases[] = {
    {   "test_hive_authorize",             test_hive_authorize             },
    {   "test_hive_authorize_without_new", test_hive_authorize_without_new },
    {   NULL,                              NULL                            }
};

static CU_SuiteInfo suite[] = {
    {   "hive authorize test", hive_authorize_test_suite_init, hive_authorize_test_suite_cleanup, NULL, NULL, cases },
    {    NULL,                 NULL,                           NULL,                              NULL, NULL, NULL  }
};

CU_SuiteInfo* hive_authorize_test_suite_info(void)
{
    return suite;
}
