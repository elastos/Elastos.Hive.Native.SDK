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

#include <CUnit/Basic.h>
#include <ela_hive.h>

#include "config.h"
#include "test_context.h"
#include "test_helper.h"

static void test_client_get_info_with_expired_token(void)
{
    int rc;
    HiveClientInfo info;

    rc = hive_client_login(test_ctx.client, open_authorization_url, NULL);
    CU_ASSERT_FATAL(rc == HIVEOK);

    rc = hive_set_access_token_expired(test_ctx.client);
    CU_ASSERT_FATAL(rc == HIVEOK);

    rc = hive_client_get_info(test_ctx.client, &info);
    CU_ASSERT_FATAL(rc == HIVEOK);
}

static CU_TestInfo cases[] = {
    { "test_client_get_info_with_expired_token", test_client_get_info_with_expired_token },
    { NULL, NULL }
};

CU_TestInfo *client_get_info_test_get_cases(void)
{
    return cases;
}

int onedrive_client_get_info_test_suite_init(void)
{
    test_ctx.client = onedrive_client_new();
    if (!test_ctx.client)
        return -1;

    return 0;
}

int onedrive_client_get_info_test_suite_cleanup(void)
{
    test_context_cleanup();

    return 0;
}

int ipfs_client_get_info_test_suite_init(void)
{
    test_ctx.client = ipfs_client_new();
    if (!test_ctx.client)
        return -1;

    return 0;
}

int ipfs_client_get_info_test_suite_cleanup(void)
{
    test_context_cleanup();

    return 0;
}
