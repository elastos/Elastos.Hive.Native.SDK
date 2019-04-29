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

extern const char *profile_name;
extern const char *file_path;
extern const char *file_newpath;
extern const char *file_newname;
extern int hive_delete_profile_file(char* profile_name);

static hive_1drv_opt_t onedrv_option;
static hive_t *hive = NULL;
static char *result;

static
void onedrv_open_oauth_url(const char *url)
{

    ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
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

    rc = hive_copy(hive, file_path, file_newpath);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_move(hive, file_path, file_newname);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_delete(hive, file_newname);
    CU_ASSERT_EQUAL(rc, 0);

    return;
}

static int hive_file_test_suite_init(void)
{
    onedrv_option.base.type = HIVE_TYPE_ONEDRIVE;
    strcpy(onedrv_option.profile_path, profile_name);
    onedrv_option.open_oauth_url = onedrv_open_oauth_url;

    if (hive_delete_profile_file(profile_name))
        return -1;

    if (hive_global_init())
        return -1;

    hive = hive_new((hive_opt_t *)(&onedrv_option));
    if(!hive)
        return -1;

    if(hive_authorize(hive))
        return -1;

    if (hive_mkdir(hive, file_path))
        return -1;

    return 0;
}

static int hive_file_test_suite_cleanup(void)
{
    onedrv_option.base.type = HIVE_TYPE_NONEXIST;
    strcpy(onedrv_option.profile_path, "");

    hive_delete(hive, file_path);
    hive_kill(hive);
    hive_global_cleanup();
    hive = NULL;

    return hive_delete_profile_file(profile_name);
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
