#include <stdlib.h>
#include <assert.h>

#include "client.h"
#include "local_client.h"
#include "onedrive_client.h"
#include "owncloud.h"
#include "hiveipfs_client.h"

typedef struct ClientFactoryMethod {
    int drive_type;
    HiveClient * (*factory_cb)(const HiveOptions *);
} ClientFactoryMethod;

static ClientFactoryMethod client_factory_methods[] = {
    { HiveDriveType_Local,     localfs_client_new  },
    { HiveDriveType_OneDrive,  onedrive_client_new },
    { HiveDriveType_ownCloud,  owncloud_client_new },
    { HiveDriveType_HiveIPFS,  hiveipfs_client_new },
    { HiveDriveType_Butt,      NULL }
};

HiveClient *hive_client_new(const HiveOptions *options)
{
    ClientFactoryMethod *method = &client_factory_methods[0];
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
    if (!client) {
        hive_set_error(-1);
        return -1;
    }

    if (!client->login)
        return 0;

    return client->login(client);
}

int hive_client_logout(HiveClient *client)
{
    if (!client) {
        hive_set_error(-1);
        return -1;
    }

    if (!client->logout)
        return 0;

    return client->logout(client);
}

int hive_client_get_info(HiveClient *client, char **result)
{
    if (!client || !result) {
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

    return client->invalidate_credential(client);
}
