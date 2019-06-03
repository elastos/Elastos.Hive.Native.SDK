#include <stdlib.h>
#include <assert.h>

#include <crystal.h>

#include "client.h"
#include "local_client.h"
#include "onedrive_client.h"
#include "owncloud.h"
#include "hiveipfs_client.h"

typedef struct FactoryMethod {
    int drive_type;
    HiveClient * (*factory_cb)(const HiveOptions *);
} FactoryMethod;

static FactoryMethod factory_methods[] = {
    { HiveDriveType_Local,     localfs_client_new  },
    { HiveDriveType_OneDrive,  onedrive_client_new },
    { HiveDriveType_ownCloud,  owncloud_client_new },
    { HiveDriveType_HiveIPFS,  hiveipfs_client_new },
    { HiveDriveType_Butt,      NULL }
};

HiveClient *hive_client_new(const HiveOptions *options)
{
    FactoryMethod *method = &factory_methods[0];
    HiveClient *client = NULL;

    if (!options || !options->persistent_location ||
        !*options->persistent_location) {
        hive_set_error(-1);
        return NULL;
    }

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
    if (!client) {
        hive_set_error(-1);
        return -1;
    }

    if (client->finalize)
        client->finalize(client);

    return 0;
}

int hive_client_login(HiveClient *client)
{
    int rc;

    if (!client) {
        hive_set_error(-1);
        return -1;
    }

    if (!client->login) {
        hive_set_error(-1);
        return 0;
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
        hive_set_error(-1);
        return -1;
    }

    rc = client->login(client);
    if (rc < 0) {
        // recover back to 'RAW' state.
        __sync_val_compare_and_swap(&client->state, LOGINING, RAW);
        hive_set_error(-1);
        return -1;
    }

    // When conducting all login stuffs successfully, then change to be
    // 'LOGINED'.
    __sync_val_compare_and_swap(&client->state,  LOGINING, LOGINED);
    return 0;
}

int hive_client_logout(HiveClient *client)
{
    int rc;

    if (!client) {
        hive_set_error(-1);
        return -1;
    }

    if (!client->logout) {
        hive_set_error(-1);
        return 0;
    }

    rc = client_try_logout(client);
    switch(rc) {
    case 0:
        break;

    case 1:
        return 0;

    case -1:
    default:
        hive_set_error(-1);
        return -1;
    }

    rc = client->logout(client);
    __sync_val_compare_and_swap(&client->state, LOGOUTING, RAW);

    if (rc < 0) {
        hive_set_error(-1);
        return -1;
    }

    return 0;
}

int hive_client_get_info(HiveClient *client, char **result)
{
    if (!client || !result) {
        hive_set_error(-1);
        return -1;
    }

    if (!is_client_ready(client))  {
        hive_set_error(-1);
        return -1;
    }

    if (!client->get_info) {
        hive_set_error(-1);
        return -1;
    }

    return client->get_info(client, result);
}

int hive_client_list_drives(HiveClient *client, char **result)
{
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

    return client->list_drives(client, result);
}

HiveDrive *hive_drive_open(HiveClient *client)
{
    if (!client) {
        hive_set_error(-1);
        return NULL;
    }

    if (!is_client_ready(client))  {
        hive_set_error(-1);
        return NULL;
    }

    if  (!client->get_default_drive) {
        hive_set_error(-1);
        return NULL;
    }

    return client->get_default_drive(client);
}

int hive_client_invalidate_credential(HiveClient *client)
{
    if (!client) {
        hive_set_error(-1);
        return -1;
    }

    if (!is_client_ready(client))  {
        hive_set_error(-1);
        return -1;
    }

    return client->invalidate_credential(client);
}
