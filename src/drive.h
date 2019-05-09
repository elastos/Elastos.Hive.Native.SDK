#ifndef __HIVE_DRIVE_H__
#define __HIVE_DRIVE_H__

typedef struct HiveDrive HiveDrive;
typedef struct HiveDriveOptions HiveDriveOptions;
typedef struct HiveDriveInfo HiveDriveInfo;

int hive_drive_getinfo(HiveDrive *drive, HiveDriveInfo *info);

#endif // __HIVE_DRIVE_H__
