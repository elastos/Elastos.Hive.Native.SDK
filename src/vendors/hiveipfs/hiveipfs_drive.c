#include <stddef.h>

#include "hiveipfs_drive.h"

typedef struct ipfs_drive {
    HiveDrive base;
} ipfs_drv_t;



HiveDrive *ipfs_drive_open(HiveClient *client)
{
    (void)client;
    return NULL;
}
