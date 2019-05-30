#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>
#include <pthread.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <Shellapi.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <elastos_hive.h>
#include "config.h"

extern void hive_ready_for_login(void);
extern int onedrv_open_oauth_url(const char *url);
extern int hive_login_record_time(HiveClient *client);

static int used_id = 0;
static OneDriveOptions onedrv_option;
static HiveClient *client;

static
void* second_hive_authorize_entry(void *argv)
{
    HiveClient *client = (HiveClient*)argv;

    int rc;

    hive_ready_for_login();
    rc = hive_login_record_time(client);
    CU_ASSERT_EQUAL(rc, 0);
    return NULL;
}

static
int hive_open_login_url_for_logining(const char *url)
{
    int rc = onedrv_open_oauth_url(url);

    if (!used_id)
    {
       pthread_t tid;
       rc = pthread_create(&tid, NULL, second_hive_authorize_entry, client);
       if(!rc)
          used_id++;
    }
    return rc;
}

static void test_hive_client_logining(void)
{
    int rc;

    hive_ready_for_login();
    rc = hive_login_record_time(client);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_client_logout(client);
    CU_ASSERT_EQUAL(rc, 0);

    return;
}

static int hive_client_logining_test_suite_init(void)
{
    onedrv_option.base.persistent_location = global_config.profile;
    onedrv_option.base.drive_type = HiveDriveType_OneDrive;
    onedrv_option.client_id = global_config.oauthinfo.client_id;
    onedrv_option.scope = global_config.oauthinfo.scope;
    onedrv_option.redirect_url = global_config.oauthinfo.redirect_url;
    onedrv_option.grant_authorize = hive_open_login_url_for_logining;

    client = hive_client_new((HiveOptions*)(&onedrv_option));
    if(!client)
        return -1;

    return 0;
}

static int hive_client_logining_test_suite_cleanup(void)
{
    onedrv_option.base.drive_type = HiveDriveType_Butt;
    onedrv_option.client_id = "";
    onedrv_option.scope = "";
    onedrv_option.redirect_url = "";
    onedrv_option.grant_authorize = NULL;
    onedrv_option.base.persistent_location = "";

    return hive_client_close(client);
}

static CU_TestInfo cases[] = {
    {   "test_hive_client_logining",           test_hive_client_logining  },
    {   NULL,                                  NULL                       }
};

static CU_SuiteInfo suite[] = {
    {   "hive client logining test", hive_client_logining_test_suite_init, hive_client_logining_test_suite_cleanup, NULL, NULL, cases },
    {    NULL,                       NULL,                                 NULL,                                    NULL, NULL, NULL  }
};

CU_SuiteInfo* hive_client_logining_test_suite_info(void)
{
    return suite;
}
