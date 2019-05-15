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

extern const char *profile_name;
extern const char *file_path;
extern const char *file_newpath;
extern const char *file_newname;
extern const char *file_movepath;
extern int hive_delete_profile_file(const char* profile_name);
extern void hive_ready_for_oauth(void);
extern int onedrv_open_oauth_url(const char *url);
extern int hive_authorize_record_time(hive_t * hive);

static hive_opt_t hive_option;
static hive_1drv_opt_t onedrv_option;
static hive_t *hive = NULL;
static char *result;

static void test_hive_settime_without_mkdir(void)
{
    int rc;

    struct timeval time;
    gettimeofday(&time, NULL);
    time.tv_sec += 1000;
    time.tv_usec += 100000;

    rc = hive_set_timestamp(hive, file_path, time);
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

    rc = hive_copy(hive, file_path, file_newpath);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static void test_hive_move_without_mkdir(void)
{
    int rc;

    rc = hive_move(hive, file_path, file_movepath);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static int hive_no_file_test_suite_init(void)
{
    onedrv_option.base.type = HIVE_TYPE_ONEDRIVE;
    strcpy(onedrv_option.profile_path, profile_name);
    onedrv_option.open_oauth_url = onedrv_open_oauth_url;

    if (hive_global_init())
        return -1;

    hive = hive_new((hive_opt_t *)(&onedrv_option));
    if(!hive)
        return -1;

    hive_ready_for_oauth();
    return hive_authorize_record_time(hive);
}

static int hive_no_file_test_suite_cleanup(void)
{
    onedrv_option.base.type = HIVE_TYPE_NONEXIST;
    strcpy(onedrv_option.profile_path, "");

    hive_kill(hive);
    hive_global_cleanup();

    hive = NULL;

    return hive_delete_profile_file(profile_name);
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
