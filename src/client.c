#include <stdlib.h>
#include <sys/stat.h>

#include <crystal.h>

#include "client.h"
#include "native_client.h"
#include "ipfs_client.h"
#include "onedrive_client.h"
#include "owncloud.h"

typedef struct FactoryMethod {
    int drive_type;
    HiveClient *(*factory_cb)(const HiveOptions *);
} FactoryMethod;

static FactoryMethod factory_methods[] = {
    { HiveDriveType_Native,    native_client_new   },
    // { HiveDriveType_IPFS,      ipfs_client_new     },
    { HiveDriveType_OneDrive,  onedrive_client_new },
    { HiveDriveType_ownCloud,  owncloud_client_new },
    { HiveDriveType_Butt,      NULL }
};

HiveClient *hive_client_new(const HiveOptions *options)
{
    FactoryMethod *method = &factory_methods[0];
    HiveClient *client;
    struct stat st;
    int rc;

    if (!options || !options->persistent_location ||
        !*options->persistent_location)
        return NULL;

    rc = stat(options->persistent_location, &st);
    if (rc < 0)
        return NULL;

    if (!S_ISDIR(st.st_mode))
        return NULL;

    for (; method->factory_cb; method++) {
        if (method->drive_type == options->drive_type) {
            client = method->factory_cb(options);
            break;
        }
    }

    if (!method->factory_cb) {
        hive_set_error(-1);
        return NULL;
    }

    return client;
}

int hive_client_close(HiveClient *client)
{
    if (!client)
        return 0;

    if (client->close)
        client->close(client);

    return 0;
}

int hive_client_login(HiveClient *client,
                      HiveRequestAuthenticationCallback *callback,
                      void *context)
{
    int rc;

    if (!client || !callback) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    /*
     * Check login state.
     * 1. If already logined, return OK immediately, else
     * 2. if being in progress of logining, then return error. else
     * 3. It's in raw state, conduct the login process.
     */
    rc = client_try_login(client);
    switch(rc) {
    case 0:
        break;

    case 1:
        vlogD("Hive: This client logined already");
        return 0;

    case -1:
    default:
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_WRONG_STATE));
        return -1;
    }

    if (client->login)
        rc = client->login(client, callback, context);

    if (rc < 0) {
        // recover back to 'RAW' state.
        _test_and_swap32(&client->state, LOGINING, RAW);
        hive_set_error(rc);
        return -1;
    }

    // When conducting all login stuffs successfully, then change to be
    // 'LOGINED'.
    _test_and_swap32(&client->state, LOGINING, LOGINED);
    return 0;
}

int hive_client_logout(HiveClient *client)
{
    int rc;

    if (!client) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    rc = client_try_logout(client);
    switch(rc) {
    case 0:
        break;

    case 1:
        return 0;

    case -1:
    default:
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_WRONG_STATE));
        return -1;
    }

    if (client->logout)
        client->logout(client);

    _test_and_swap32(&client->state, LOGOUTING, RAW);

    return 0;
}

int hive_client_get_info(HiveClient *client, HiveClientInfo *info)
{
    if (!client || !info) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!is_client_ready(client))  {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_READY));
        return -1;
    }

    if (!client->get_info) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    return client->get_info(client, info);
}

#if 0
int hive_client_list_drives(HiveClient *client, char **result)
{
    int rc;

    if (!client || !result) {
        hive_set_error(-1);
        return -1;
    }

    if (!is_client_ready(client))  {
        hive_set_error(-1);
        return -1;
    }

    if (!client->list_drives) {
        hive_set_error(-1);
        return -1;
    }

    rc = client->list_drives(client, result);
    if (rc < 0)  {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}
#endif

HiveDrive *hive_drive_open(HiveClient *client)
{
    HiveDrive *drive;
    int rc;

    if (!client) {
        hive_set_error(-1);
        return NULL;
    }

    if (!is_client_ready(client))  {
        hive_set_error(-1);
        return NULL;
    }

    if  (!client->get_drive) {
        hive_set_error(-1);
        return NULL;
    }

    rc = client->get_drive(client, &drive);
    if (rc < 0) {
        hive_set_error(rc);
        return NULL;
    }

    return drive;
}

#if 0
int hive_client_invalidate_credential(HiveClient *client)
{
    int rc;

    if (!client) {
        hive_set_error(-1);
        return -1;
    }

    if (!is_client_ready(client))  {
        hive_set_error(-1);
        return -1;
    }

    rc = client->invalidate_credential(client);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}
#endif
