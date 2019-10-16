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

#include <ela_hive.h>
#include <CUnit/Basic.h>

#include "../test_context.h"
#include "../../config.h"

bool key_value_cb(const char *key, const void *value, size_t length, void *context)
{

    bool *key_correct = (bool *)(((void **)context)[0]);
    bool *val1_present = (bool *)(((void **)context)[1]);
    bool *val2_present = (bool *)(((void **)context)[2]);

    if (strcmp(key, "key"))
        return true;

    *key_correct = true;

    if (length == strlen("value") + 1 && !strcmp(value, "value")) {
        *val1_present = true;
        return true;
    }

    if (length == strlen("value2") + 1 && !strcmp(value, "value2"))
        *val2_present = true;

    return true;
}

void key_value_apis_test(void)
{
    int rc;
    bool key_correct = false;
    bool val1_present = false;
    bool val2_present = false;
    void *args[] = {&key_correct, &val1_present, &val2_present};

    rc = hive_put_value(test_ctx.connect, "key", "value",
                        strlen("value") + 1, true);
    CU_ASSERT_TRUE_FATAL(rc == 0);

    rc = hive_get_values(test_ctx.connect, "key", true, key_value_cb, args);
    if (rc < 0 || !key_correct || !val1_present || val2_present) {
        hive_delete_key(test_ctx.connect, "key");
        CU_FAIL("put value failure.");
        return;
    }

    rc = hive_put_value(test_ctx.connect, "key", "value2",
                        strlen("value2") + 1, true);
    if (rc < 0) {
        hive_delete_key(test_ctx.connect, "key");
        CU_FAIL("put value failure.");
        return;
    }

    key_correct = false;
    val1_present = false;
    val2_present = false;
    rc = hive_get_values(test_ctx.connect, "key", true, key_value_cb, args);
    if (rc < 0 || !key_correct || !val1_present || !val2_present) {
        hive_delete_key(test_ctx.connect, "key");
        CU_FAIL("put value failure.");
        return;
    }

    rc = hive_set_value(test_ctx.connect, "key", "value",
                        strlen("value") + 1, true);
    if (rc < 0) {
        hive_delete_key(test_ctx.connect, "key");
        CU_FAIL("set value failure.");
        return;
    }

    key_correct = false;
    val1_present = false;
    val2_present = false;
    rc = hive_get_values(test_ctx.connect, "key", true, key_value_cb, args);
    if (rc < 0 || !key_correct || !val1_present || val2_present) {
        hive_delete_key(test_ctx.connect, "key");
        CU_FAIL("put value failure.");
        return;
    }

    rc = hive_delete_key(test_ctx.connect, "key");
    CU_ASSERT_TRUE(rc == 0);
}
