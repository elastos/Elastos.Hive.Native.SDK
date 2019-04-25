#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>
#include <stdio.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <hive.h>

static hive_1drv_opt_t onedrv_option;
static hive_t *hive = NULL;

static const char *file_path  = "/Documents/HiveTest";
static const char *profile_name = "hive1drv.json";

static
void onedrv_open_oauth_url(const char *url)
{
    ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    return;
}

static int hive_delete_profile_file(char* profile_name)
{
    int rc = 0;

    if (access(profile_name, F_OK) == 0)
        rc = remove(profile_name);

    return rc;
}

static void test_hive_mkdir_without_authorize(void)
{
    int rc;

    rc = hive_mkdir(hive, file_path);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static void test_hive_mkdir(void)
{
    int rc;

    rc = hive_authorize(hive);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_mkdir(hive, file_path);
    CU_ASSERT_EQUAL(rc, 0);

    return;
}

static int hive_mkdir_test_suite_init(void)
{
    onedrv_option.base.type = HIVE_TYPE_ONEDRIVE;
    strcpy(onedrv_option.profile_path, "hive1drv.json");
    onedrv_option.open_oauth_url = onedrv_open_oauth_url;

    if (hive_delete_profile_file(profile_name))
        return -1;

    if (hive_global_init())
        return -1;

    hive = hive_new((hive_opt_t *)(&onedrv_option));
    if(!hive)
        return -1;

    return 0;
}

static int hive_mkdir_test_suite_cleanup(void)
{
    onedrv_option.base.type = HIVE_TYPE_NONEXIST;
    strcpy(onedrv_option.profile_path, "");

    hive_kill(hive);
    hive_global_cleanup();

    hive = NULL;

    return 0;
}

static CU_TestInfo cases[] = {
    {   "test_hive_mkdir_without_authorize",         test_hive_mkdir_without_authorize  },
    {   "test_hive_mkdir",                           test_hive_mkdir                    },
    {   NULL,                                        NULL                               }
};

static CU_SuiteInfo suite[] = {
    {   "hive mkdir test", hive_mkdir_test_suite_init, hive_mkdir_test_suite_cleanup, NULL, NULL, cases },
    {    NULL,             NULL,                       NULL,                          NULL, NULL, NULL  }
};

CU_SuiteInfo* hive_mkidr_test_suite_info(void)
{
    return suite;
}
