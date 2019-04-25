#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>

#include <hive.h>

static hive_opt_t hive_option;
static hive_1drv_opt_t onedrv_option;
static hive_t *hive = NULL;
static char *result;

static const char *file_path  = "hive-test.txt";
static const char *file_new_path = "hive-new-test.txt";
static const char *file_name = "hive-1drv.txt";
static const char *file_new_name = "hive-new-1drv.txt";

static
void onedrv_open_oauth_url(const char *url)
{

    ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    printf("***onedrv_open_oauth_url\n");
    return;
}

static void test_hive_settime_without_mkdir(void)
{
    int rc;

    rc = hive_mkdir(hive, file_path);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static void test_hive_delete_without_mkdir(void)
{
    int rc;

    rc = hive_delete(hive, file_path);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static void test_hive_stat_without_mkdir(void)
{
    int rc;

    rc = hive_stat(hive, file_path, &result);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static void test_hive_list_without_mkdir(void)
{
    int rc;

    rc = hive_list(hive, file_path, &result);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static void test_hive_copy_without_mkdir(void)
{
    int rc;

    rc = hive_copy(hive, file_name, file_new_name);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static void test_hive_move_without_mkdir(void)
{
    int rc;

    rc = hive_move(hive, file_name, file_new_name);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static int hive_no_file_test_suite_init(void)
{
    onedrv_option.base.type = HIVE_TYPE_ONEDRIVE;
    strcpy(onedrv_option.profile_path, "hive1drv.json");
    onedrv_option.open_oauth_url = onedrv_open_oauth_url;

    int rc = hive_global_init();

    hive = hive_new((hive_opt_t *)(&onedrv_option));
    if(!hive)
        rc = -1;

    return hive_authorize(hive);
}

static int hive_no_file_test_suite_cleanup(void)
{
    onedrv_option.base.type = HIVE_TYPE_NONEXIST;
    strcpy(onedrv_option.profile_path, "");

    hive_kill(hive);
    hive_global_cleanup();

    hive = NULL;

    return 0;
}

static CU_TestInfo cases[] = {
    {   "test_hive_settime_without_mkdir", test_hive_settime_without_mkdir },
    {   "test_hive_delete_without_mkdir",  test_hive_delete_without_mkdir  },
    {   "test_hive_stat_without_mkdir",    test_hive_stat_without_mkdir    },
    {   "test_hive_list_without_mkdir",    test_hive_list_without_mkdir    },
    {   "test_hive_copy_without_mkdir",    test_hive_copy_without_mkdir    },
    {   "test_hive_move_without_mkdir",    test_hive_move_without_mkdir    },
    {   NULL,                              NULL                            }
};

static CU_SuiteInfo suite[] = {
    {   "hive no file test", hive_no_file_test_suite_init, hive_no_file_test_suite_cleanup, NULL, NULL, cases },
    {    NULL,               NULL,                         NULL,                            NULL, NULL, NULL  }
};

CU_SuiteInfo* hive_no_file_test_suite_info(void)
{
    return suite;
}
