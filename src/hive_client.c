#include <stdlib.h>
#include <sys/stat.h>
#include <crystal.h>

#include "hive_error.h"
#include "hive_client.h"

#include "native_client.h"
#include "ipfs_client.h"
#include "onedrive.h"
#include "owncloud.h"

typedef struct FactoryMethod {
    int drive_type;
    HiveClient *(*create_client)(const HiveOptions *);
} FactoryMethod;

static FactoryMethod factory_methods[] = {
    { HiveDriveType_Native,    native_client_new   },
    { HiveDriveType_IPFS,      ipfs_client_new     },
    { HiveDriveType_OneDrive,  onedrive_client_new },
    { HiveDriveType_ownCloud,  owncloud_client_new },
    { HiveDriveType_Bott,      NULL }
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
    if (rc < 0|| !S_ISDIR(st.st_mode)) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return NULL;
    }

    for (; method->create_client; method++) {
        if (method->drive_type == options->drive_type) {
            client = method->create_client(options);
            break;
        }
    }

    if (!method->create_client) {
        hive_set_error(-1);
        return NULL;
    }

    return client;
}

int hive_client_close(HiveClient *client)
{
    if (!client)
        return 0;

    // TODO: token;

    if (client->close)
        client->close(client);

    return 0;
}

int hive_client_login(HiveClient *client,
                      HiveRequestAuthenticationCallback callback,
                      void *context)
{
    int rc;

    if (!client || !callback) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (client->login) {
        rc = client->login(client, callback, context);
        if (rc < 0) {
            hive_set_error(rc);
            return -1;
        }
    }

    return 0;
}

int hive_client_logout(HiveClient *client)
{
    if (!client) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (client->logout)
        client->logout(client);

    return 0;
}

int hive_client_get_info(HiveClient *client, HiveClientInfo *info)
{
    int rc;

    if (!client || !info) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!has_valid_token(client->token))  {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_READY));
        return -1;
    }

    if (!client->get_info) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = client->get_info(client, info);
    if (rc < 0) {
        hive_set_error(-1);
        return -1;
    }

    return 0;
}

HiveDrive *hive_drive_open(HiveClient *client)
{
    HiveDrive *drive;
    int rc;

    if (!client) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return NULL;
    }

    if (!has_valid_token(client->token))  {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_READY));
        return NULL;
    }

    if (!client->get_drive) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return NULL;
    }

    rc = client->get_drive(client, &drive);
    if (rc < 0) {
        hive_set_error(rc);
        return NULL;
    }

    return drive;
}
