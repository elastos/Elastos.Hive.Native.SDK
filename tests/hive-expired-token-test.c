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
extern int hive_delete_profile_file(const char* profile_name);
extern void hive_ready_for_oauth(void);
extern int onedrv_open_oauth_url(const char *url);

static hive_1drv_opt_t onedrv_option;
static hive_t *hive = NULL;
static const char* test_file = "/Documents/Test_Expired_Token";

static void test_hive_expired_token(void)
{
    int rc;

    rc = hive_set_expired(hive);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_mkdir(hive, test_file);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_delete(hive, test_file);
    CU_ASSERT_EQUAL(rc, 0);

    return;
}


static int hive_expired_token_test_suite_init(void)
{
    onedrv_option.base.type = HIVE_TYPE_ONEDRIVE;
    strcpy(onedrv_option.profile_path, "hive1drv.json");
    onedrv_option.open_oauth_url = onedrv_open_oauth_url;

    if (hive_global_init())
        return -1;

    hive = hive_new((hive_opt_t *)(&onedrv_option));
    if(!hive)
        return -1;

    hive_ready_for_oauth();
    if(hive_authorize(hive) != 0)
        return -1;

    return 0;
}

static int hive_expired_token_test_suite_cleanup(void)
{
    onedrv_option.base.type = HIVE_TYPE_NONEXIST;
    strcpy(onedrv_option.profile_path, "");

    hive_kill(hive);
    hive_global_cleanup();

    return hive_delete_profile_file(profile_name);
}

static CU_TestInfo cases[] = {
    {   "test_hive_expired_token",         test_hive_expired_token  },
    {   NULL,                              NULL                     }
};

static CU_SuiteInfo suite[] = {
    {   "hive expired token test", hive_expired_token_test_suite_init, hive_expired_token_test_suite_cleanup, NULL, NULL, cases },
    {    NULL,             NULL,                       NULL,                          NULL, NULL, NULL  }
};

CU_SuiteInfo* hive_expired_token_test_suite_info(void)
{
    return suite;
}
