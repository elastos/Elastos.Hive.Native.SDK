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
#include <elastos_hive.h>

extern int onedrv_open_oauth_url(const char *url);

static OneDriveOptions onedrv_option;
static struct timeval end;

void hive_ready_for_login(void)
{
    struct timeval start;

    start.tv_sec = end.tv_sec;
    start.tv_usec = end.tv_usec;

    gettimeofday(&end, NULL);

    int time_inter = end.tv_sec - start.tv_sec;
    if (time_inter < 2 && time_inter >= 0)
       sleep(2-time_inter);

    return;
}

int hive_login_record_time(HiveClient *client)
{
    int rc = hive_client_login(client);
    gettimeofday(&end, NULL);

    return rc;
}

static void test_hive_client_login_without_new(void)
{
    int rc;

    rc = hive_authorize_record_time(NULL);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static void test_hive_client_login(void)
{
    int rc;
    HiveClient *client;

    client = hive_client_new((HiveOptions*)(&onedrv_option));
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    hive_ready_for_login();
    rc = hive_login_record_time(client);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_client_logout(client);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_client_close(client);
    CU_ASSERT_EQUAL(rc, 0);
    return;
}

static void test_hive_client_double_login(void)
{
    int rc;
    HiveClient *client;

    client = hive_client_new((HiveOptions*)(&onedrv_option));
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    hive_ready_for_login();
    rc = hive_login_record_time(client);
    CU_ASSERT_EQUAL(rc, 0);

    hive_ready_for_login();
    rc = hive_login_record_time(client);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_client_logout(client);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_client_close(client);
    CU_ASSERT_EQUAL(rc, 0);

    return;
}

static int hive_client_login_test_suite_init(void)
{
    strcpy(onedrv_option.base.persistent_location, global_config.profile);
    onedrv_option.base.drive_type = HiveDriveType_OneDrive;
    onedrv_option.client_id = global_config.oauthinfo.client_id;
    onedrv_option.scope = global_config.oauthinfo.scope;
    onedrv_option.redirect_url = global_config.oauthinfo.redirect_url;
    onedrv_option.grant_authorize = onedrv_open_oauth_url;

    return 0;
}

static int hive_client_login_test_suite_cleanup(void)
{
    onedrv_option.base.drive_type = HiveDriveType_Butt;
    onedrv_option.client_id = "";
    onedrv_option.scope = "";
    onedrv_option.redirect_url = "";
    onedrv_option.grant_authorize = "";
    onedrv_option.base.persistent_location = "";

    return 0;
}

static CU_TestInfo cases[] = {
    {   "test_hive_login_without_new", test_hive_login_without_new },
    {   "test_hive_login",             test_hive_login             },
    {   "test_hive_double_login",      test_hive_double_login      },
    {   NULL,                          NULL                        }
};

static CU_SuiteInfo suite[] = {
    {   "hive client login test", hive_client_login_test_suite_init, hive_client_login_test_suite_cleanup, NULL, NULL, cases },
    {    NULL,                    NULL,                              NULL,                                 NULL, NULL, NULL  }
};

CU_SuiteInfo* hive_client_login_test_suite_info(void)
{
    return suite;
}
