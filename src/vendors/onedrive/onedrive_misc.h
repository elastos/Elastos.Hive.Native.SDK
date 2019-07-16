/*
 * Copyright (c) 2019 Elastos Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
        return HIVE_GENERAL_ERROR(HIVEERR_BAD_JSON_FORMAT);

    if (cJSON_IsNull(item)) {
        buf[0] = '\0';
        return 0;
    }

    if (strlen(item->valuestring) >= len)
        return HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);

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
