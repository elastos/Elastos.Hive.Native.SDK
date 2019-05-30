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
static HiveDrive *drive;

static void test_hive_drive_move_without_drive(void)
{
    int rc;

    rc = hive_drive_move_file(NULL, global_config.files.file_path, global_config.files.file_newpath);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static void test_hive_dirve_move_without_oldpath(void)
{
    int rc;

    rc = hive_drive_move_file(drive, NULL, global_config.files.file_newpath);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static void test_hive_drive_move_without_newpath(void)
{
    int rc;

    rc = hive_drive_move_file(drive, global_config.files.file_path, global_config.files.file_newpath);
    CU_ASSERT_EQUAL(rc, -1);

    return;
}

static void test_hive_drive_move(void)
{
    int rc;

    rc = hive_drive_move_file(drive, global_config.files.file_path, global_config.files.file_newpath);
    CU_ASSERT_EQUAL(rc, 0);

    return;
}

static void test_hive_drive_move_dirctory(void)
{
    int rc;

    rc = hive_drive_mkdir(drive, global_config.files.file_movepath);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_drive_move_file(drive, global_config.files.file_path, global_config.files.file_movepath);
    CU_ASSERT_EQUAL(rc, 0);

    rc = hive_drive_delete_file(drive, global_config.files.file_movepath);
    CU_ASSERT_EQUAL(rc, 0);

    return;
}

static int hive_drive_move_test_suite_init(void)
{
    int rc;

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
    rc = hive_login_record_time(client);
    if(rc)
        return -1;

    drive = hive_drive_open(client, (HiveDriveOptions*)(&drv_options));
    if(!drive)
        return -1;

    return hive_drive_mkdir(drive, global_config.files.file_path);
}

static int hive_drive_move_test_suite_cleanup(void)
{
    int rc;

    onedrv_option.base.drive_type = HiveDriveType_Butt;
    onedrv_option.client_id = "";
    onedrv_option.scope = "";
    onedrv_option.redirect_url = "";
    onedrv_option.grant_authorize = NULL;
    onedrv_option.base.persistent_location = "";

    rc = hive_drive_delete_file(drive, global_config.files.file_path);
    if(rc)
        return -1;

    rc = hive_drive_close(drive);
    if(rc)
        return -1;

    rc = hive_client_logout(client);
    if(rc)
        return rc;

    return hive_client_close(client);
}

static CU_TestInfo cases[] = {
    {   "test_hive_drive_move_without_drive",         test_hive_drive_move_without_drive   },
    {   "test_hive_dirve_move_without_oldpath",       test_hive_dirve_move_without_oldpath },
    {   "test_hive_drive_move_without_newpath",       test_hive_drive_move_without_newpath },
    {   "test_hive_drive_move",                       test_hive_drive_move                 },
    {   "test_hive_drive_move_dirctory",              test_hive_drive_move_dirctory        },
    {   NULL,                                         NULL                                 }
};

static CU_SuiteInfo suite[] = {
    {   "hive drive move test", hive_drive_move_test_suite_init, hive_drive_move_test_suite_cleanup, NULL, NULL, cases },
    {    NULL,                  NULL,                            NULL,                               NULL, NULL, NULL  }
};

CU_SuiteInfo* hive_drive_move_test_suite_info(void)
{
    return suite;
}
