#ifndef __ONEDRIVE_DRIVE_H__
#define __ONEDRIVE_DRIVE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "drive.h"
#include "oauth_token.h"

int onedrive_drive_open(oauth_token_t *token, const char *driveid,
                        HiveDrive **drive);

#ifdef __cplusplus
}
#endif

#endif // __ONEDRIVE_DRIVE_H__
