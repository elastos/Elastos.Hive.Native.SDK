#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <Shellapi.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <hive_impl.h>

extern int onedrv_open_oauth_url(const char *url);

const char *profile_name = "hive1drv.json";
const char *file_path = "/Documents/HiveTest/helloworld";
const char *file_newpath = "/Documents/HiveTest_New/";
const char *file_newname = "/byeworld";

static hive_1drv_opt_t onedrv_option;
static struct timeval end;

int hive_delete_profile_file(const char* profile_name)
{
    int rc = 0;

    if (!access(profile_name, F_OK))
        rc = remove(profile_name);

    return rc;
}

void hive_ready_for_oauth(void)
{
    struct timeval start;

    start.tv_sec = end.tv_sec;
    start.tv_usec = end.tv_usec;

    gettimeofday(&end, NULL);

    int time_inter = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1000000;
    if (time_inter < 2 && time_inter >= 0)
       sleep(2-time_inter);

    return;
}

int hive_authorize_record_time(hive_t * hive)
{
    int rc = -1;

    if(!hive)
        return -1;

    rc = hive_authorize(hive);
    gettimeofday(&end, NULL);

    return rc;
}

static void test_hive_authorize_without_new(void)
{
    int rc;
    hive_t *hive = NULL;

    rc = hive_authorize_record_time(hive);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static void test_hive_double_authorize(void)
{
    int rc;
    hive_t *hive = NULL;

    hive = hive_new((hive_opt_t *)(&onedrv_option));
    CU_ASSERT_PTR_NOT_NULL_FATAL(hive);

    hive_ready_for_oauth();
    rc = hive_authorize_record_time(hive);
    CU_ASSERT_EQUAL(rc, 0);

    hive_ready_for_oauth();
    rc = hive_authorize_record_time(hive);
    CU_ASSERT_EQUAL(rc, 0);

    hive_kill(hive);
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
