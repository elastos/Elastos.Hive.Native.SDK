
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

#include "hive.h"
#include "version.h"

typedef struct cjson_t {
    //TODO;
} cjson_t;

typedef struct HiveOptions {
    //TODO;
} HiveOptions;

typedef struct Hive {
    //TODO;
} Hive;

const char *hive_get_version(void) {
    return hive_version;
}

Hive *hive_new(const HiveOptions *options)
{
    //TODO;
    return NULL;
}

void hive_kill(Hive *hive)
{
    //TODO;
}

void hive_free_json(cjson_t *json)
{
    //TODO;
}

int hive_stat(Hive *hive, const char *path, cjson_t **result)
{
    //TODO;
    return -1;
}

int hive_set_timestamp(Hive *hive, const char *path,
                       const struct timeval *new_timestamp)
{
    //TODO;
    return -1;
}

int hive_list(Hive *hive, const char *path, cjson_t **result)
{
    //TODO;
    return -1;
}

int hive_mkdir(Hive *hive, const char *path)
{
    //TODO;
    return -1;
}

int hive_move(Hive *hive, const char *old, const char *new)
{
    //TODO;
    return -1;
}

int hive_copy(Hive *hive, const char *src, const char *dest_path)
{
    //TODO;
    return -1;
}

int hive_delete(Hive *hive, const char *path)
{
    //TODO;
    return -1;
}

int hive_async_stat(Hive *hive, const char *path,
                    HiveResponseCallbacks *callbacks, void *context)
{
    //TODO;
    return -1;
}

int hive_async_set_timestamp(Hive *hive, const char *path,
                             const struct timeval *new_timestamp,
                             HiveResponseCallbacks *callbacks, void *context)
{
    //TODO;
    return -1;
}

int hive_async_list(Hive *hive, const char *path,
                    HiveResponseCallbacks *callbacks, void *context)
{
    //TODO;
    return -1;
}

int hive_async_mkdir(Hive *hive, const char *path,
                     HiveResponseCallbacks *callbacks, void *context)
{
    //TODO;
    return -1;
}

int hive_async_move(Hive *hive, const char *old, const char *new,
                    HiveResponseCallbacks *callbacks, void *context)
{
    //TODO;
    return -1;
}

int hive_async_copy(Hive *hive, const char *src_path, const char *dest_path,
                    HiveResponseCallbacks *callbacks, void *context)
{
    //TODO;
    return -1;
}

int hive_async_delete(Hive *hive, const char *path,
                      HiveResponseCallbacks *callbacks, void *context)
{
    //TODO;
    return -1;
}
