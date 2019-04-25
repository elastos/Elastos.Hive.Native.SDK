#include <stdlib.h>
#include <stdio.h>
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

static void test_hive_init(void)
{
    int rc;

    rc = hive_global_init();
    CU_ASSERT_EQUAL_FATAL(rc, 0);

    hive_global_cleanup();
    return;
}

static void test_hive_double_init(void)
{
    int rc;

    rc = hive_global_init();
    CU_ASSERT_EQUAL_FATAL(rc, 0);

    rc = hive_global_init();
    CU_ASSERT_EQUAL_FATAL(rc, 0);

    hive_global_cleanup();
    return;
}

static void test_hive_new(void)
{
    int rc;
    hive_t *hive = NULL;

    rc = hive_global_init();
    CU_ASSERT_EQUAL_FATAL(rc, 0);

    hive = hive_new((hive_opt_t *)(&onedrv_option));
    CU_ASSERT_PTR_NOT_NULL_FATAL(hive);

    hive_kill(hive);
    hive = NULL;

    onedrv_option.base.type = HIVE_TYPE_NONEXIST;

    hive = hive_new((hive_opt_t *)(&onedrv_option));
    CU_ASSERT_PTR_NULL_FATAL(hive);

    hive_kill(hive);
    hive_global_cleanup();

    return;
}

static void test_hive_new_without_init(void)
{
    hive_t *hive = NULL;

    onedrv_option.base.type = HIVE_TYPE_ONEDRIVE;

    hive = hive_new((hive_opt_t *)(&onedrv_option));
    CU_ASSERT_PTR_NULL_FATAL(hive);

    hive_kill(hive);

    return;
}

static int hive_new_test_suite_init(void)
{
    onedrv_option.base.type = HIVE_TYPE_ONEDRIVE;
    strcpy(onedrv_option.profile_path, "hive1drv.json");
    onedrv_option.open_oauth_url = onedrv_open_oauth_url;

    return 0;
}

static int hive_new_test_suite_cleanup(void)
{
    onedrv_option.base.type = HIVE_TYPE_NONEXIST;
    strcpy(onedrv_option.profile_path, "");

    return 0;
}

static CU_TestInfo cases[] = {
    {   "test_hive_init",             test_hive_init             },
    {   "test_hive_double_init",      test_hive_double_init      },
    {   "test_hive_new",              test_hive_new              },
    {   "test_hive_new_without_init", test_hive_new_without_init },
    {   NULL,                         NULL                       }
};

static CU_SuiteInfo suite[] = {
    {   "hive new test", hive_new_test_suite_init, hive_new_test_suite_cleanup, NULL, NULL, cases },
    {    NULL,           NULL,                     NULL,                        NULL, NULL, NULL  }
};

CU_SuiteInfo* hive_new_test_suite_info(void)
{
    return suite;
}
