#include <stdlib.h>
#include <stdio.h>
#include <CUnit/Basic.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <Shellapi.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <elastos_hive.h>
#include "config.h"

static OneDriveOptions onedrv_option;

int onedrv_open_oauth_url(const char *url)
{
#if defined(_WIN32) || defined(_WIN64)
   ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#else
    char open_url[PATH_MAX];
    memcpy(open_url, "open ", 5);
    memcpy(open_url+5, url, strlen(url)+1 );

    system(open_url);
#endif
    return 0;
}

static void test_hive_client_new(void)
{
    int rc;
    HiveClient *client;

    onedrv_option.base.drive_type = HiveDriveType_OneDrive;

    client = hive_client_new((HiveOptions*)(&onedrv_option));
    CU_ASSERT_PTR_NOT_NULL(client);

    rc = hive_client_close(client);
    CU_ASSERT_EQUAL(rc, 0);

    return;
}

static void test_hive_client_new_with_default_type(void)
{
    int rc;
    HiveClient *client;

    onedrv_option.base.drive_type = HiveDriveType_Butt;

    client = hive_client_new((HiveOptions*)(&onedrv_option));
    CU_ASSERT_PTR_NULL(client);

    rc = hive_client_close(client);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static int hive_client_new_test_suite_init(void)
{
    onedrv_option.base.persistent_location = global_config.profile;
    onedrv_option.client_id = global_config.oauthinfo.client_id;
    onedrv_option.scope = global_config.oauthinfo.scope;
    onedrv_option.redirect_url = global_config.oauthinfo.redirect_url;
    onedrv_option.grant_authorize = onedrv_open_oauth_url;

    return 0;
}

static int hive_client_new_test_suite_cleanup(void)
{
    onedrv_option.base.drive_type = HiveDriveType_Butt;
    onedrv_option.client_id = "";
    onedrv_option.scope = "";
    onedrv_option.redirect_url = "";
    onedrv_option.grant_authorize = NULL;
    onedrv_option.base.persistent_location = "";

    return 0;
}

static CU_TestInfo cases[] = {
    {   "test_hive_client_new",                   test_hive_client_new                   },
    {   "test_hive_client_new_with_default_type", test_hive_client_new_with_default_type },
    {   NULL,                                     NULL                                   }
};

static CU_SuiteInfo suite[] = {
    {   "hive client new test", hive_client_new_test_suite_init, hive_client_new_test_suite_cleanup, NULL, NULL, cases },
    {    NULL,                  NULL,                            NULL,                               NULL, NULL, NULL  }
};

CU_SuiteInfo* hive_client_new_test_suite_info(void)
{
    return suite;
}
