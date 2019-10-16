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

#include <string.h>

#include <ela_hive.h>

#include "test_context.h"
#include "../config.h"

test_context test_ctx;

void test_context_reset()
{
    HiveClient *client = test_ctx.client;

    if (test_ctx.connect)
        hive_client_disconnect(test_ctx.connect);

    memset(&test_ctx, 0, sizeof(test_ctx));

    test_ctx.client = client;
}

int test_context_init()
{
    HiveOptions opts = {
        .data_location = global_config.data_location
    };

    test_ctx.client = hive_client_new(&opts);

    return test_ctx.client ? 0 : -1;
}

void test_context_deinit()
{
    if (test_ctx.connect)
        hive_client_disconnect(test_ctx.connect);

    if (test_ctx.client)
        hive_client_close(test_ctx.client);
}
