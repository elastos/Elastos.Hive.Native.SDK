#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>
#include <windows.h>
#include <Shellapi.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <pthread.h>
#include <hive.h>

extern const char *profile_name;
extern const char *file_path;
extern const char *file_newpath;
extern const char *file_newname;
static int used_id = 0;

static hive_1drv_opt_t onedrv_option;
static hive_t *hive;

extern int hive_delete_profile_file(char* profile_name);
extern void hive_ready_for_oauth(void);

static int test_hive_second_authorize(void);

static
void* second_hive_authorize_entry(void *argv)
{
    hive_t *hive = (hive_t*)argv;

    int rc;

    hive_ready_for_oauth();
    rc = hive_authorize(hive);
    CU_ASSERT_EQUAL(rc, 0);
    return NULL;
}

static
int onedrv_open_oauth_url(const char *url)
{
    int rc = 0;

    ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);

    if (!used_id)
    {
       rc = test_hive_second_authorize();
       used_id++;
    }
    return rc;
}

static void test_hive_authorizing(void)
{
    int rc;

    hive_ready_for_oauth();
    rc = hive_authorize(hive);
    CU_ASSERT_EQUAL(rc, 0);

    return;
}

static int test_hive_second_authorize(void)
{
    int rc;
    pthread_t tid;

    rc = pthread_create(&tid, NULL, second_hive_authorize_entry, hive);
    if(rc)
    {
        return -1;
    }

    return 0;
}

static int hive_authorizing_test_suite_init(void)
{
    onedrv_option.base.type = HIVE_TYPE_ONEDRIVE;
    strcpy(onedrv_option.profile_path, "hive1drv.json");
    onedrv_option.open_oauth_url = onedrv_open_oauth_url;

    if (hive_global_init() != 0)
        return -1;

    hive = hive_new((hive_opt_t *)(&onedrv_option));
    if(!hive)
        return -1;

    return 0;
}

static int hive_authorizing_test_suite_cleanup(void)
{
    onedrv_option.base.type = HIVE_TYPE_NONEXIST;
    strcpy(onedrv_option.profile_path, "");

    hive_global_cleanup();

    return hive_delete_profile_file(profile_name);
}

static CU_TestInfo cases[] = {
    {   "test_hive_authorizing",           test_hive_authorizing           },
    {   NULL,                              NULL                            }
};

static CU_SuiteInfo suite[] = {
    {   "hive authorizing test", hive_authorizing_test_suite_init, hive_authorizing_test_suite_cleanup, NULL, NULL, cases },
    {    NULL,                   NULL,                             NULL,                                NULL, NULL, NULL  }
};

CU_SuiteInfo* hive_authorizing_test_suite_info(void)
{
    return suite;
}
