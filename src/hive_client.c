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

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include <crystal.h>

#include "hive_error.h"
#include "hive_client.h"
#include "ipfs.h"
#include "onedrive.h"
#include "mkdirs.h"

typedef struct FactoryMethod {
    int backendType;
    HiveConnect *(*connect)(HiveClient *, const HiveConnectOptions *);
} FactoryMethod;

static FactoryMethod factory_methods[] = {
    {HiveBackendType_IPFS,     ipfs_client_connect     },
    {HiveBackendType_OneDrive, onedrive_client_connect },
    {HiveDriveType_Butt,      NULL }
};

void hive_log_init(HiveLogLevel level, const char *log_file,
                   void (*log_printer)(const char *format, va_list args))
{
    vlog_init(level, log_file, log_printer);
}

HiveClient *hive_client_new(const HiveOptions *opts)
{
    HiveClient *client;
    struct stat st;
    int rc;

    if (!opts || !opts->data_location || !*opts->data_location) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return NULL;
    }

    rc = stat(opts->data_location, &st);
    if ((rc < 0 && errno != ENOENT) || (!rc && !S_ISDIR(st.st_mode))) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return NULL;
    }

    if (rc < 0) {
        rc = mkdirs(opts->data_location, S_IRWXU);
        if (rc < 0) {
            hive_set_error(HIVE_SYS_ERROR(errno));
            return NULL;
        }
    }

    client = rc_zalloc(sizeof(HiveClient) + strlen(opts->data_location) + 1, NULL);
    if (!client) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_OUT_OF_MEMORY));
        return NULL;
    }

    strcpy(client->data_location, opts->data_location);
    return client;
}

int hive_client_close(HiveClient *client)
{
    if (!client)
        return 0;

    deref(client);

    return 0;
}

HiveConnect *hive_client_connect(HiveClient *client, HiveConnectOptions *opts)
{
    FactoryMethod *method = &factory_methods[0];
    HiveConnect *connect;

    if (!client || !opts) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return NULL;
    }

    for (; method->connect; method++) {
        if (method->backendType == opts->backendType) {
            connect = method->connect(client, opts);
            break;
        }
    }

    if (!method->connect) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_NOT_SUPPORTED));
        return NULL;
    }

    return connect;
}

int hive_client_disconnect(HiveConnect *connect)
{
    if (!connect)
        return 0;

    if (connect->disconnect)
        connect->disconnect(connect);

    return 0;
}

int hive_set_encrypt_key(HiveConnect *connect, const char *encrypt_key)
{
    if (!connect || !encrypt_key) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    // TODO:
    return 0;
}

