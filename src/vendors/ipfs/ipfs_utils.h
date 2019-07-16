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

#ifndef __COMMON_OPS_H__
#define __COMMON_OPS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ipfs_rpc.h"

int ipfs_synchronize(ipfs_rpc_t *rpc);
int ipfs_publish(ipfs_rpc_t *rpc, const char *path);
int ipfs_stat_file(ipfs_rpc_t *rpc, const char *file_path, HiveFileInfo *info);

static inline bool is_string_item(cJSON *item)
{
    return cJSON_IsString(item) && item->valuestring && *item->valuestring;
}

int publish_root_hash(ipfs_rpc_t *rpc, char *buf, size_t length);

#ifdef __cplusplus
}
#endif

#endif // __COMMON_OPS_H__
