#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>
#include <windows.h>
#include <Shellapi.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <hive.h>

const char *profile_name = "hive1drv.json";
const char *file_path = "/Documents/HiveTest/helloworld.txt";
const char *file_newpath = "/Documents/HiveTest_New/helloworld.txt";
const char *file_newname = "/byeworld.txt";

static hive_1drv_opt_t onedrv_option;

int hive_delete_profile_file(char* profile_name)
{
    int rc = 0;

    if (access(profile_name, F_OK) == 0)
        rc = remove(profile_name);

    return rc;
}

static
void onedrv_open_oauth_url(const char *url)
{

    ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
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

static void test_hive_double_authorize(void)
{
    int rc;
    hive_t *hive = NULL;

    hive = hive_new((hive_opt_t *)(&onedrv_option));
    CU_ASSERT_PTR_NOT_NULL_FATAL(hive);

    rc = hive_authorize(hive);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_authorize(hive);
    CU_ASSERT_EQUAL(rc, 0);

    hive_kill(hive);
    return;
}

static void test_hive_authorizing(void)
{
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

    return hive_delete_profile_file(profile_name);
}

static CU_TestInfo cases[] = {
    {   "test_hive_authorize_without_new", test_hive_authorize_without_new },
    {   "test_hive_double_authorize",      test_hive_double_authorize      },
    {   "test_hive_authorizing",           test_hive_authorizing           },
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
