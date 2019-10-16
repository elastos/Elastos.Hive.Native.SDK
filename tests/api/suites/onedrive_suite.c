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

#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#include <windows.h>
#include <shellapi.h>
#endif

#include <CUnit/Basic.h>
#include <ela_hive.h>

#include "../cases/case.h"
#include "../cases/file_apis_cases.h"
#include "api/cases/key_value_apis_cases.h"
#include "../test_context.h"

#define HIVETEST_REDIRECT_URL "http://localhost:12345"
#define HIVETEST_SCOPE "Files.ReadWrite.AppFolder offline_access"
#define HIVETEST_ONEDRIVE_CLIENT_ID "afd3d647-a8b7-4723-bf9d-1b832f43b881"

static CU_TestInfo cases[] = {
    DEFINE_FILE_APIS_CASES,
    DEFINE_KEY_APIS_CASES,
    DEFINE_TESTCASE_NULL
};

CU_TestInfo* onedrive_get_cases()
{
    return cases;
}

static int open_url(const char *url, void *context)
{
#if defined(_WIN32) || defined(_WIN64)
    ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    return 0;
#elif defined(__linux__)
    char cmd[strlen("xdg-open ") + strlen(url) + 3];
    sprintf(cmd, "xdg-open '%s'", url);
    system(cmd);
    return 0;
#elif defined(__APPLE__)
    char cmd[strlen("open ") + strlen(url) + 3];
    sprintf(cmd, "open '%s'", url);
    system(cmd);
    return 0;
#else
#   error "Unsupported Os."
#endif
}

int onedrive_suite_init()
{
    OneDriveConnectOptions opts = {
        .backendType  = HiveBackendType_OneDrive,
        .redirect_url = HIVETEST_REDIRECT_URL,
        .scope        = HIVETEST_SCOPE,
        .client_id    = HIVETEST_ONEDRIVE_CLIENT_ID,
        .callback     = open_url,
        .context      = NULL
    };

    test_ctx.connect = hive_client_connect(test_ctx.client, (HiveConnectOptions *)&opts);
    if (!test_ctx.connect) {
        CU_FAIL("Error: test suite initialize error");
        return -1;
    }

    return 0;
}

int onedrive_suite_cleanup()
{
    test_context_reset();

    return 0;
}
