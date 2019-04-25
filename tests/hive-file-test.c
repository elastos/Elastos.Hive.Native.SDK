#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>

#include <hive.h>

static hive_opt_t hive_option;
static hive_1drv_opt_t onedrv_option;
static hive_t *hive = NULL;

static const char *file_path  = "/Documents/HiveTest/hive-test.txt";
static const char *file_new_path = "hive-new-test.txt";
static const char *file_name = "hive-1drv.txt";
static const char *file_new_name = "hive-new-1drv.txt";

static char *result;

static
void onedrv_open_oauth_url(const char *url)
{

    ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    printf("***onedrv_open_oauth_url\n");
    return;
}

static void test_hive_file(void)
{
    int rc;

    struct timeval time;
    gettimeofday(&time, NULL);
    time.tv_sec += 1000;
    time.tv_usec += 100000;

    rc = hive_set_timestamp(hive, file_path, time);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_stat(hive,file_path, &result);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_list(hive, file_path, &result);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_copy(hive, file_name, file_new_name);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_move(hive, file_name, file_new_name);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_delete(hive, file_new_name);
    CU_ASSERT_EQUAL(rc, 0);

    return;
}

static int hive_file_test_suite_init(void)
{
    onedrv_option.base.type = HIVE_TYPE_ONEDRIVE;
    strcpy(onedrv_option.profile_path, "hive1drv.json");
    onedrv_option.open_oauth_url = onedrv_open_oauth_url;

    int rc = hive_global_init();

    hive = hive_new((hive_opt_t *)(&onedrv_option));
    if(!hive)
        rc = -1;

    rc = hive_authorize(hive);

    rc = hive_mkdir(hive, file_path);

    return rc;
}

static int hive_file_test_suite_cleanup(void)
{
    onedrv_option.base.type = HIVE_TYPE_NONEXIST;
    strcpy(onedrv_option.profile_path, "");

    hive_delete(hive, file_path);
    hive_kill(hive);
    hive_global_cleanup();

    hive = NULL;

    return 0;
}

static CU_TestInfo cases[] = {
    {   "test_hive_file",  test_hive_file },
    {   NULL,              NULL           }
};

static CU_SuiteInfo suite[] = {
    {   "hive file test", hive_file_test_suite_init, hive_file_test_suite_cleanup, NULL, NULL, cases },
    {    NULL,            NULL,                      NULL,                         NULL, NULL, NULL  }
};

CU_SuiteInfo* hive_file_test_suite_info(void)
{
    return suite;
}
