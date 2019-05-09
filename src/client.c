#include <stdlib.h>

#include "client.h"
#include "local_client.h"
#include "onedrive_client.h"
#include "owncloud.h"
#include "hiveipfs.h"

typedef struct ClientFactoryMethod {
    int drive_type;
    HiveClient * (*factory_func)(const HiveOptions *);
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
    ClientFactoryMethod *method;
    HiveClient *client = NULL;

    if (!options || !options->persistent_location || !*options->persistent_location)
        return NULL;

    for (method = &client_factory_methods[0]; method->factory_func; method++) {
        if (method->drive_type == options->drive_type) {
            client = method->factory_func(options);
            break;
        }
    }

    if (!method->factory_func)
        return NULL;

    if (!client)
        return NULL;

    return client;
}

int hive_client_close(HiveClient *client)
{
    if (!client)
        return -1;

    client->destructor_func(client);
    return 0;
}

int hive_client_login(HiveClient *client)
{
    if (!client)
        return -1;

    return client->login(client);
}

int hive_client_logout(HiveClient *client)
{
    if (!client)
        return -1;

    return client->logout(client);
}

