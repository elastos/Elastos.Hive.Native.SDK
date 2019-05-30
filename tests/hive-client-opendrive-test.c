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
#include "config.h"

extern void hive_ready_for_login(void);
extern int onedrv_open_oauth_url(const char *url);
extern int hive_login_record_time(HiveClient *client);

static OneDriveOptions onedrv_option;
static OneDriveDriveOptions drv_options;
static HiveClient *client;

static void test_hive_client_opendrive_without_new(void)
{
    HiveDrive *drive = hive_drive_open(NULL, &drv_options);
    CU_ASSERT_PTR_NULL(drive);

    return;
}

static void test_hive_client_opendrive_without_option(void)
{
    HiveDrive *drive = hive_drive_open(client, NULL);
    CU_ASSERT_PTR_NULL(drive);

    return;
}

static void test_hive_client_opendrive(void)
{
    int rc;

    HiveDrive *drive = hive_drive_open(client, (HiveDriveOptions*)(&drv_options));
    CU_ASSERT_PTR_NOT_NULL(drive);

    //TODO:Now, driveid is default. When client returns drive list containing drive id, we test it.

    rc = hive_drive_close(drive);
    CU_ASSERT_EQUAL(rc, 0);

    return;
}

static int hive_client_opendrive_test_suite_init(void)
{
    onedrv_option.base.persistent_location = global_config.profile;
    onedrv_option.base.drive_type = HiveDriveType_OneDrive;
    onedrv_option.client_id = global_config.oauthinfo.client_id;
    onedrv_option.scope = global_config.oauthinfo.scope;
    onedrv_option.redirect_url = global_config.oauthinfo.redirect_url;
    onedrv_option.grant_authorize = onedrv_open_oauth_url;

    strcpy(drv_options.drive_id, "default");

    client = hive_client_new((HiveOptions*)(&onedrv_option));
    if (!client)
        return -1;

    hive_ready_for_login();
    return hive_login_record_time(client);
}

static int hive_client_opendrive_test_suite_cleanup(void)
{
    int rc;

    onedrv_option.base.drive_type = HiveDriveType_Butt;
    onedrv_option.client_id = "";
    onedrv_option.scope = "";
    onedrv_option.redirect_url = "";
    onedrv_option.grant_authorize = NULL;
    onedrv_option.base.persistent_location = "";

    rc = hive_client_logout(client);
    if(rc)
        return rc;

    return hive_client_close(client);
}

static CU_TestInfo cases[] = {
    {   "test_hive_client_opendrive_without_new",           test_hive_client_opendrive_without_new     },
    {   "test_hive_client_opendrive_without_option",        test_hive_client_opendrive_without_option  },
    {   "test_hive_client_opendrive",                       test_hive_client_opendrive                 },
    {   NULL,                                               NULL                                       }
};

static CU_SuiteInfo suite[] = {
    {   "hive client opendrive test", hive_client_opendrive_test_suite_init, hive_client_opendrive_test_suite_cleanup, NULL, NULL, cases },
    {    NULL,                      NULL,                                NULL,                                   NULL, NULL, NULL  }
};

CU_SuiteInfo* hive_client_opendrive_suite_info(void)
{
    return suite;
}
