#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <Shellapi.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <elastos_hive.h>

extern void hive_ready_for_login(void);
extern int onedrv_open_oauth_url(const char *url);
extern int hive_login_record_time(HiveClient * client);

static OneDriveOptions onedrv_option;
static HiveClient *client;

static void test_hive_client_drivelist_without_new(void)
{
    int rc;
    char *result;

    rc = hive_client_list_drives(NULL, &result);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static void test_hive_client_drivelist_without_result(void)
{
    int rc;

    rc = hive_client_list_drives(client, NULL);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static void test_hive_client_drivelist(void)
{
    int rc;
    char *result;

    rc = hive_client_list_drives(client, &result);
    CU_ASSERT_EQUAL(rc, 0);
    //Todo: how to apply result
    return;
}

static int hive_client_drivelist_test_suite_init(void)
{
    strcpy(onedrv_option.base.persistent_location, global_config.profile);
    onedrv_option.base.drive_type = HiveDriveType_OneDrive;
    onedrv_option.client_id = global_config.oauthinfo.client_id;
    onedrv_option.scope = global_config.oauthinfo.scope;
    onedrv_option.redirect_url = global_config.oauthinfo.redirect_url;
    onedrv_option.grant_authorize = onedrv_open_oauth_url;

    client = hive_client_new((HiveOptions*)(&onedrv_option));
    if (!client)
        return -1;

    hive_ready_for_login();
    return hive_login_record_time(client);
}

static int hive_client_drivelist_test_suite_cleanup(void)
{
    int rc;

    onedrv_option.base.drive_type = HiveDriveType_Butt;
    onedrv_option.client_id = "";
    onedrv_option.scope = "";
    onedrv_option.redirect_url = "";
    onedrv_option.grant_authorize = "";
    onedrv_option.base.persistent_location = "";

    rc = hive_client_logout(client);
    if(rc)
        return rc;

    return hive_client_close(client);
}

static CU_TestInfo cases[] = {
    {   "test_hive_client_drivelist_without_new",           test_hive_client_drivelist_without_new     },
    {   "test_hive_client_drivelist_without_result",        test_hive_client_drivelist_without_result  },
    {   "test_hive_client_drivelist",                       test_hive_client_drivelist                 },
    {   NULL,                                               NULL                                       }
};

static CU_SuiteInfo suite[] = {
    {   "hive client drivelist test", hive_client_drivelist_test_suite_init, hive_client_drivelist_test_suite_cleanup, NULL, NULL, cases },
    {    NULL,                        NULL,                                  NULL,                                     NULL, NULL, NULL  }
};

CU_SuiteInfo* hive_client_drivelist_suite_info(void)
{
    return suite;
}
