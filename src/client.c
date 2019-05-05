#include <stdlib.h>

#include "client.h"
#include "local_client.h"
#include "onedrive_client.h"
#include "owncloud.h"
#include "hiveipfs.h"

typedef struct HiveClient HiveClient;
struct HiveClient {
    // TODO;
    void (*destructor_func)(HiveClient *);
};

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

    int drive_type = options->drive_type;

    for (method = &client_factory_methods[0]; !method; method++) {
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
        return 0;

    client->destructor_func(client);
    return 0;
}
