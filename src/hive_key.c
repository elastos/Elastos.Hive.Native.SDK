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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "hive_error.h"
#include "hive_client.h"

int hive_put_value(HiveConnect *connect, const char *key, const void *value,
                   size_t length, bool encrypt)
{
    int rc;

    if (!connect || !key || !*key || !value || !length ||
        length > HIVE_MAX_VALUE_LEN) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!connect->put_value) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = connect->put_value(connect, key, value, length, encrypt);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

int hive_set_value(HiveConnect *connect, const char *key, const void *value,
                   size_t length, bool encrypt)
{
    int rc;

    if (!connect || !key || !*key || !value || !length ||
        length > HIVE_MAX_VALUE_LEN) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!connect->set_value) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = connect->set_value(connect, key, value, length, encrypt);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

int hive_get_values(HiveConnect *connect, const char *key, bool decrypt,
                    HiveKeyValuesIterateCallback *callback, void *context)
{
    int rc;

    if (!connect || !key || !*key || !callback) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!connect->get_values) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = connect->get_values(connect, key, decrypt, callback, context);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

int hive_delete_key(HiveConnect *connect, const char *key)
{
    int rc;

    if (!connect || !key || !*key) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!connect->delete_key) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return -1;
    }

    rc = connect->delete_key(connect, key);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}

int hive_set_access_token_expired(HiveConnect *connect)
{
    int rc;

    if (!connect) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    if (!connect->expire_token)
        return 0;

    rc = connect->expire_token(connect);
    if (rc < 0) {
        hive_set_error(rc);
        return -1;
    }

    return 0;
}
