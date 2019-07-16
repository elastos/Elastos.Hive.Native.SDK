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

#ifndef __TEST_HELPER_H__
#define __TEST_HELPER_H__

#include <ela_hive.h>

typedef struct {
    char *name;
    char *type;
} dir_entry;

#define ONEDRIVE_DIR_ENTRY(n, t) \
    { \
        .name = n, \
        .type = t  \
    }

#define IPFS_DIR_ENTRY(n, t) \
    { \
        .name = n, \
        .type = NULL  \
    }

HiveClient *onedrive_client_new();
HiveClient *ipfs_client_new();
int open_authorization_url(const char *url, void *context);
char *get_random_file_name();
int list_files_test_scheme(HiveDrive *drive, const char *dir,
                           dir_entry *expected_entries, int size);
#endif // __TEST_HELPER_H__
