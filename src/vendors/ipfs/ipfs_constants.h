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

#ifndef __IPFS_CONSTANTS_H__
#define __IPFS_CONSTANTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <curl/curl.h>

#include "ela_hive.h"

#define MAX_URL_LEN         (1024)

#define HIVE_MAX_IPV4_ADDRESS_LEN (15)
#define HIVE_MAX_IPV6_ADDRESS_LEN (47)
#define HIVE_MAX_IPFS_UID_LEN     (127)

#define RC_NODE_UNREACHABLE(rc)                             \
    ((rc) == HIVE_CURL_ERROR(CURLE_COULDNT_CONNECT)      || \
     (rc) == HIVE_CURL_ERROR(CURLE_REMOTE_ACCESS_DENIED) || \
     (rc) == HIVE_CURL_ERROR(CURLE_OPERATION_TIMEDOUT))

#ifdef __cplusplus
}
#endif

#endif // __IPFS_CONSTANTS_H__
