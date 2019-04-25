/*
 * Copyright (c) 2018 Elastos Foundation
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

#ifndef __TEST_CONFIG_H__
#define __TEST_CONFIG_H__

#include <limits.h>
#include <crystal.h>

#define HIVE_CONFIG_LEN 512

typedef struct TestConfig {
    int shuffle;
    int log2file;
    char profile[PATH_MAX+1];

    struct {
        int loglevel;
    } tests;

    struct {
        char client_id[HIVE_CONFIG_LEN];
        char scope[HIVE_CONFIG_LEN];
        char redirect_url[HIVE_CONFIG_LEN];
    } oauthinfo;

    struct {
        char file_name[PATH_MAX + 1];
        char file_newname[PATH_MAX + 1];
        char file_path[PATH_MAX + 1];
        char file_newpath[PATH_MAX + 1];
        char file_movepath[PATH_MAX + 1];
    } files;
} TestConfig;

extern TestConfig global_config;

void load_config(const char *config_file);

#endif /* __TEST_CONFIG_H__ */
