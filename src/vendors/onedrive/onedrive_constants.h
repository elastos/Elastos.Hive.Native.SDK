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

#ifndef __ONEDRIVE_CONSTANTS_H__
#define __ONEDRIVE_CONSTANTS_H__

#define URL_OAUTH   "https://login.microsoftonline.com/common/oauth2/v2.0/"

#define MAX_URL_LEN         (1024)

#define MY_DRIVE    "https://graph.microsoft.com/v1.0/me/drive"
#define APP_ROOT    MY_DRIVE"/special/approot"
#define FILES_DIR   "/files"
#define KEYS_DIR    "/keys"

#define METHOD_AUTHORIZE "authorize"
#define METHOD_TOKEN     "token"

#endif // __ONEDRIVE_CONSTANTS_H__
