#ifndef __ONEDRIVE_DRIVE_H__
#define __ONEDRIVE_DRIVE_H__

#include "drive.h"

HiveDrive *onedrive_drive_open(HiveClient *onedrv_client, const char *drive_id);

#endif // __ONEDRIVE_DRIVE_H__
