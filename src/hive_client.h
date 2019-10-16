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

#ifndef __HIVE_CLIENT_H__
#define __HIVE_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ela_hive.h"

struct HiveClient {
    char data_location[0];
};

struct HiveConnect {
    int state;  // login state.

    int     (*put_file_from_buffer)     (HiveConnect *, const void *, size_t, bool, const char *);
    ssize_t (*get_file_length)          (HiveConnect *, const char *);
    ssize_t (*get_file_to_buffer)       (HiveConnect *, const char *, bool, void *, size_t);
    int     (*list_files)               (HiveConnect *, HiveFilesIterateCallback *, void *);
    int     (*delete_file)              (HiveConnect *, const char *);

    int     (*ipfs_put_file_from_buffer)(HiveConnect *, const void *, size_t, bool, IPFSCid *);
    ssize_t (*ipfs_get_file_length)     (HiveConnect *, const IPFSCid *cid);
    ssize_t (*ipfs_get_file_to_buffer)  (HiveConnect *, const IPFSCid *, bool, void *, size_t);

    int     (*put_value)                (HiveConnect *, const char *, const void *, size_t, bool);
    int     (*set_value)                (HiveConnect *, const char *, const void *, size_t, bool);
    int     (*get_values)               (HiveConnect *, const char *, bool, HiveKeyValuesIterateCallback *, void *);
    int     (*delete_key)               (HiveConnect *, const char *);

    int     (*disconnect)               (HiveConnect *);
    int     (*expire_token)             (HiveConnect *);
};

HIVE_API
int hive_set_access_token_expired(HiveConnect *connect);

#ifdef __cplusplus
}
#endif

#endif // __HIVE_CLIENT_H__
