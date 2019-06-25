#ifndef __ONEDRIVE_MISC_H__
#define __ONEDRIVE_MISC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ela_hive.h"
#include "oauth_token.h"
#include "hive_client.h"

static inline
int decode_info_field(cJSON *json, const char *name, char *buf, size_t len)
{
    cJSON *item;

    item = cJSON_GetObjectItemCaseSensitive(json, name);
    if (!item || (!cJSON_IsNull(item) &&
                  !(cJSON_IsString(item) && item->valuestring && *item->valuestring)))
        return -1;

    if (cJSON_IsNull(item)) {
        buf[0] = '\0';
        return 0;
    }

    if (strlen(item->valuestring) >= len)
        return -2;

    strcpy(buf, item->valuestring);
    return 0;
}

int onedrive_drive_open(oauth_token_t *token, const char *driveid,
                        const char *tmp_template, HiveDrive **);

int onedrive_file_open(oauth_token_t *token, const char *path,
                       int flags, const char *tmp_template, HiveFile **file);

#ifdef __cplusplus
}
#endif

#endif // __ONEDRIVE_MISC_H__
