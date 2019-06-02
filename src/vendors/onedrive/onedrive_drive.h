#ifndef __ONEDRIVE_DRIVE_H__
#define __ONEDRIVE_DRIVE_H__

#include "drive.h"
#include "oauth_client.h"

HiveDrive *onedrive_drive_open(oauth_client_t *credential, const char *drive_id);

#endif // __ONEDRIVE_DRIVE_H__
