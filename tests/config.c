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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
/*ifdef HAVE_DIRECT_H
#include <direct.h>
#endif*/

#include <libconfig.h>
#include <crystal.h>

#include "config.h"

TestConfig global_config;

static int get_str(config_t *cfg, const char *name, const char *default_value,
                    char *value, size_t len)
{
    const char *stropt;
    int rc;

    rc = config_lookup_string(cfg, name, &stropt);
    if (!rc) {
        if (!default_value) {
            fprintf(stderr, "Missing '%s' option.\n", name);
            config_destroy(cfg);
            exit(-1);
        } else {
            stropt = default_value;
        }
    }

    if (strlen(stropt) >= len) {
        fprintf(stderr, "Option '%s' too long.\n", name);
        config_destroy(cfg);
        exit(-1);
    }

    strcpy(value, stropt);
    return 0;
}

static int get_int(config_t *cfg, const char *name, int default_value)
{
    int intopt;
    int rc;

    rc = config_lookup_int(cfg, name, &intopt);
    return rc ? intopt : default_value;
}

void load_config(const char *config_file)
{
    config_t cfg;
    int rc;

    config_init(&cfg);
    rc = config_read_file(&cfg, config_file);
    if (!rc) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        exit(-1);
    }

    global_config.shuffle = (int)get_int(&cfg, "shuffle", 0);
    global_config.log2file = (int)get_int(&cfg, "log2file", 0);
    global_config.tests.loglevel = (int)get_int(&cfg, "tests.loglevel", 4);

    get_str(&cfg, "profile", NULL, global_config.profile, sizeof(global_config.profile));

    get_str(&cfg, "oauthinfo.client_id", NULL, global_config.oauthinfo.client_id, sizeof(global_config.oauthinfo.client_id));
    get_str(&cfg, "oauthinfo.scope", NULL, global_config.oauthinfo.scope, sizeof(global_config.oauthinfo.scope));
    get_str(&cfg, "oauthinfo.redirect_url", NULL, global_config.oauthinfo.redirect_url, sizeof(global_config.oauthinfo.redirect_url));

    get_str(&cfg, "files.file_name", NULL, global_config.files.file_name, sizeof(global_config.files.file_name));
    get_str(&cfg, "files.file_newname", NULL, global_config.files.file_newname, sizeof(global_config.files.file_newname));
    get_str(&cfg, "files.file_path", NULL, global_config.files.file_path, sizeof(global_config.files.file_path));
    get_str(&cfg, "files.file_newpath", NULL, global_config.files.file_newpath, sizeof(global_config.files.file_newpath));
    get_str(&cfg, "files.file_movepath", NULL, global_config.files.file_movepath, sizeof(global_config.files.file_movepath));

    config_destroy(&cfg);
}

