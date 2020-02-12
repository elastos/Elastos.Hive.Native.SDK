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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <crystal.h>
#include <pthread.h>
#include <sys/stat.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_PROCESS_H
#include <process.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#include <windows.h>
#include <shellapi.h>
#endif

#include <ela_hive.h>

#include "config.h"
#include "constants.h"

typedef struct ctx ctx_t;

typedef struct {
    int     (*put_file)             (HiveConnect *, const char *, bool, const char *);
    int     (*put_file_from_buffer) (HiveConnect *, const void *, size_t, bool, const char *);
    ssize_t (*get_file_length)      (HiveConnect *, const char *);
    ssize_t (*get_file)             (HiveConnect *, const char *, bool, const char *);
    ssize_t (*get_file_to_buffer)   (HiveConnect *, const char *, bool, void *, size_t);
    int     (*list_files)           (HiveConnect *, HiveFilesIterateCallback *, void *);
    int     (*delete_file)          (HiveConnect *, const char *);

    int     (*put_value)            (HiveConnect *, const char *, const void *, size_t, bool);
    int     (*set_value)            (HiveConnect *, const char *, const void *, size_t, bool);
    int     (*get_values)           (HiveConnect *, const char *, bool, HiveKeyValuesIterateCallback *, void *);
    int     (*delete_key)           (HiveConnect *, const char *);
} ops_t;

struct ctx {
    cmd_cfg_t *cfg;
    HiveClient *client;
    HiveConnect *connect;
    ops_t *ops;
    bool stop;
};

static ctx_t cmd_ctx;
static char errbuf[1024];

static void console(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fprintf(stdout, "\n");
}

static void console_nonl(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

static void console_prompt(void)
{
    fprintf(stdout, "# ");
    fflush(stdout);
}

static void logging(const char *fmt, va_list args)
{
    //DO NOTHING.
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

static ops_t general_ops = {
    .put_file             = hive_put_file,
    .put_file_from_buffer = hive_put_file_from_buffer,
    .get_file_length      = hive_get_file_length,
    .get_file             = hive_get_file,
    .get_file_to_buffer   = hive_get_file_to_buffer,
    .list_files           = hive_list_files,
    .delete_file          = hive_delete_file,
    .put_value            = hive_put_value,
    .set_value            = hive_set_value,
    .get_values           = hive_get_values,
    .delete_key           = hive_delete_key
};

static int ipfs_put_file(HiveConnect *connect, const char *from, bool encrypt, const char *filename)
{
    IPFSCid cid;
    int rc;

    rc = hive_ipfs_put_file(connect, from, encrypt, &cid);
    if (rc < 0) {
        console("Error: failed to put file.");
        return rc;
    }

    console("CID: %s.", cid.content);
    return 0;
}

static int ipfs_put_file_from_buffer(HiveConnect *connect, const void *from, size_t length,
                                     bool encrypt, const char *filename)
{
    IPFSCid cid;
    int rc;

    rc = hive_ipfs_put_file_from_buffer(connect, from, length, encrypt, &cid);
    if (rc < 0) {
        console("Error: failed to put file from buffer.");
        return rc;
    }

    console("CID: %s.", cid.content);
    return 0;
}

static ssize_t ipfs_get_file_length(HiveConnect *connect, const char *filename)
{
    return hive_ipfs_get_file_length(connect, (IPFSCid *)filename);
}

static ssize_t ipfs_get_file(HiveConnect *connect, const char *filename, bool decrypt, const char *to)
{
    return hive_ipfs_get_file(connect, (IPFSCid *)filename, decrypt, to);
}

static ssize_t ipfs_get_file_to_buffer(HiveConnect *connect, const char *filename, bool decrypt, void *to, size_t buflen)
{
    return hive_ipfs_get_file_to_buffer(connect, (IPFSCid *)filename, decrypt, to, buflen);
}

static ops_t ipfs_ops = {
    .put_file             = ipfs_put_file,
    .put_file_from_buffer = ipfs_put_file_from_buffer,
    .get_file_length      = ipfs_get_file_length,
    .get_file             = ipfs_get_file,
    .get_file_to_buffer   = ipfs_get_file_to_buffer,
};

static void client_connect(ctx_t *ctx, int argc, char **argv)
{
    if (argc != 2) {
        console("Error: invalid command syntax.");
        return;
    }

    if (ctx->connect) {
        console("Error: a HiveConnect instance already exists.");
        return;
    }

    if (!strcmp(argv[1], "onedrive")) {
        OneDriveConnectOptions opts = {
            .backendType  = HiveBackendType_OneDrive,
            .redirect_url = HIVETEST_REDIRECT_URL,
            .scope        = HIVETEST_SCOPE,
            .client_id    = HIVETEST_ONEDRIVE_CLIENT_ID,
            .callback     = open_url,
            .context      = NULL
        };

        ctx->connect = hive_client_connect(ctx->client, (HiveConnectOptions *)&opts);
        if (!ctx->connect) {
            console("create HiveConnect instance failure.\n");
            return;
        }

        ctx->ops = &general_ops;
    } else if (!strcmp(argv[1], "ipfs")) {
        cmd_cfg_t *cfg = ctx->cfg;
        int i;

        IPFSNode *nodes = calloc(1, sizeof(IPFSNode) * cfg->ipfs_rpc_nodes_sz);
        if (!nodes) {
            console("create HiveConnect instance failure.\n");
            return;
        }

        for (i = 0; i < cfg->ipfs_rpc_nodes_sz; ++i) {
            IPFSNode *node = nodes + i;

            node->ipv4 = cfg->ipfs_rpc_nodes[i]->ipv4;
            node->ipv6 = cfg->ipfs_rpc_nodes[i]->ipv6;
            node->port = cfg->ipfs_rpc_nodes[i]->port;
        }

        IPFSConnectOptions opts = {
            .backendType    = HiveBackendType_IPFS,
            .rpc_node_count = cfg->ipfs_rpc_nodes_sz,
            .rpcNodes       = nodes
        };

        ctx->connect = hive_client_connect(ctx->client, (HiveConnectOptions *)&opts);
        free(nodes);
        if (!ctx->connect) {
            console("create HiveConnect instance failure.\n");
            return;
        }

        ctx->ops = &ipfs_ops;
    } else {
        console("Error: unsupported backend type.");
    }
}

static void disconnect(ctx_t *ctx, int argc, char **argv)
{
    if (argc != 1) {
        console("Error: invalid command syntax.");
        return;
    }

    if (ctx->connect) {
        hive_client_disconnect(ctx->connect);
        ctx->connect = NULL;
    }
}

static void put_file(ctx_t *ctx, int argc, char *argv[])
{
    const char *from;
    const char *to;
    bool from_file;
    int rc;

    if (argc < 2 || argc > 4) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->connect) {
        console("Error: No HiveConnect instance created.");
        return;
    }

    switch (argc) {
    case 2:
        from_file = false;
        from = argv[1];
        to = NULL;
        break;
    case 3:
        if (!strcmp(argv[1], "<")) {
            from_file = true;
            from = argv[2];
            to = NULL;
        } else {
            from_file = false;
            from = argv[1];
            to = argv[2];
        }
        break;
    case 4:
        from_file = true;
        from = argv[2];
        to = argv[3];
        break;
    default:
        return;
    }

    rc = from_file ?
         ctx->ops->put_file(ctx->connect, from, true, to) :
         ctx->ops->put_file_from_buffer(ctx->connect, from, strlen(from), true, to);
    if (rc < 0)
        console("Error: put file failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void get_length(ctx_t *ctx, int argc, char *argv[])
{
    ssize_t len;

    if (argc != 2) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->connect) {
        console("Error: No HiveConnect instance created.");
        return;
    }

    len = ctx->ops->get_file_length(ctx->connect, argv[1]);
    if (len < 0) {
        console("Error: get length failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
        return;
    }

    console("file %s length is %zd bytes.", argv[1], len);
}

static void get_file(ctx_t *ctx, int argc, char *argv[])
{
    ssize_t len;

    if (argc != 2 && argc != 3) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->connect) {
        console("Error: No HiveConnect instance created.");
        return;
    }

    switch (argc) {
    case 2:
        len = ctx->ops->get_file_length(ctx->connect, argv[1]);
        if (len < 0) {
            console("Error: get length failed. Reason: %s.",
                    hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
            return;
        }

        if (!len) {
            console("file %s length is 0 bytes.", argv[1]);
            return;
        }

        char *buf = malloc(len + 1);
        if (!buf) {
            console("Error: OOM.");
            return;
        }

        len = ctx->ops->get_file_to_buffer(ctx->connect, argv[1], true, buf, len);
        if (len < 0) {
            console("Error: get file to buffer failed. Reason: %s.",
                    hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
            free(buf);
            return;
        }

        buf[len] = '\0';
        console("%s", buf);
        free(buf);
        return;
    case 3:
        len = ctx->ops->get_file(ctx->connect, argv[1], true, argv[2]);
        if (len < 0)
            console("Error: get file failed. Reason: %s.",
                    hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
        return;
    default:
        return;
    }
}

static void delete_file(ctx_t *ctx, int argc, char *argv[])
{
    int rc;

    if (argc != 2) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->connect) {
        console("Error: No HiveConnect instance created.");
        return;
    }

    if (!ctx->ops->delete_file) {
        console("Error: HiveConnect instance does not support this operation.");
        return;
    }

    rc = ctx->ops->delete_file(ctx->connect, argv[1]);
    if (rc < 0)
        console("Error: delete file failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static bool list_files_cb(const char *filename, void *context)
{
    (void)context;

    if (filename)
        console("%s", filename);

    return true;
}

static void list_files(ctx_t *ctx, int argc, char *argv[])
{
    int rc;

    if (argc != 1) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->connect) {
        console("Error: No HiveConnect instance created.");
        return;
    }

    if (!ctx->ops->list_files) {
        console("Error: HiveConnect instance does not support this operation.");
        return;
    }

    rc = ctx->ops->list_files(ctx->connect, list_files_cb, NULL);
    if (rc < 0)
        console("Error: list files failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void put_value(ctx_t *ctx, int argc, char *argv[])
{
    int rc;

    if (argc != 3) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->connect) {
        console("Error: No HiveConnect instance created.");
        return;
    }

    if (!ctx->ops->put_value) {
        console("Error: HiveConnect instance does not support this operation.");
        return;
    }

    rc = ctx->ops->put_value(ctx->connect, argv[1], argv[2], strlen(argv[2]) + 1, true);
    if (rc < 0)
        console("Error: put value failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void set_value(ctx_t *ctx, int argc, char *argv[])
{
    int rc;

    if (argc != 3) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->connect) {
        console("Error: No HiveConnect instance created.");
        return;
    }

    if (!ctx->ops->set_value) {
        console("Error: HiveConnect instance does not support this operation.");
        return;
    }

    rc = ctx->ops->set_value(ctx->connect, argv[1], argv[2], strlen(argv[2]) + 1, true);
    if (rc < 0)
        console("Error: set value failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static bool get_values_cb(const char *key, const void *value,
                          size_t length, void *context)
{
    console("%s", value);

    return true;
}

static void get_values(ctx_t *ctx, int argc, char **argv)
{
    int rc;

    if (argc != 2) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->connect) {
        console("Error: No HiveConnect instance created.");
        return;
    }

    if (!ctx->ops->get_values) {
        console("Error: HiveConnect instance does not support this operation.");
        return;
    }

    rc = ctx->ops->get_values(ctx->connect, argv[1], true, get_values_cb, NULL);
    if (rc < 0)
        console("Error: get values failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void delete_key(ctx_t *ctx, int argc, char *argv[])
{
    int rc;

    if (argc != 2) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->connect) {
        console("Error: No HiveConnect instance created.");
        return;
    }

    if (!ctx->ops->delete_key) {
        console("Error: HiveConnect instance does not support this operation.");
        return;
    }

    rc = ctx->ops->delete_key(ctx->connect, argv[1]);
    if (rc < 0)
        console("Error: delete key failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void exit_app(ctx_t *ctx, int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    ctx->stop = true;
}

static void help(ctx_t *ctx, int argc, char *argv[]);
static struct command {
    const char *name;
    void (*cmd_cb)(ctx_t *, int argc, char *argv[]);
    const char *help;
} commands[] = {
    { "help"        , help          , "help [cmd]"        },
    { "connect"     , client_connect, "connect type"      },
    { "disconnect"  , disconnect    , "disconnect"        },
    { "put_file"    , put_file      , "put_file {string | < file} [to]" },
    { "get_len"     , get_length    , "get_len file"      },
    { "get_file"    , get_file      , "get_file {file | cid} [to]" },
    { "del_file"    , delete_file   , "del_file file"     },
    { "ls"          , list_files    , "ls"                },
    { "put_val"     , put_value     , "put_val key value" },
    { "set_val"     , set_value     , "set_val key value" },
    { "get_val"     , get_values    , "get_val key"       },
    { "del_key"     , delete_key    , "del_key key"       },
    { "exit"        , exit_app      , "exit"              },
    { NULL          , NULL , NULL                }
};

static void help(ctx_t *ctx, int argc, char *argv[])
{
    char line[256] = {0};
    struct command *p;

    if (argc == 1) {
        console("available commands list:");

        for (p = commands; p->name; p++) {
            strcat(line, p->name);
            strcat(line, " ");
        }
        console("  %s", line);
        memset(line, 0, sizeof(line));
    } else {
        for (p = commands; p->name; p++) {
            if (strcmp(argv[1], p->name) == 0) {
                console("usage: %s", p->help);
                return;
            }
        }
        console("unknown command: %s\n", argv[1]);
    }
}

char *read_cmd(void)
{
    int ch = 0;
    char *p;

    static int  cmd_len = 0;
    static char cmd_line[1024];

    while ((ch = fgetc(stdin))) {
        if (ch == EOF)
            return NULL;

        if (isprint(ch)) {
            cmd_line[cmd_len++] = ch;
        } else if (ch == 10 || ch == 13) {
            cmd_line[cmd_len] = 0;
            // Trim trailing spaces;
            for (p = cmd_line + cmd_len -1; p > cmd_line && isspace(*p); p--);
            *(++p) = 0;

            // Trim leading spaces;
            for (p = cmd_line; *p && isspace(*p); p++);

            cmd_len = 0;
            if (strlen(p) > 0)
                return p;
            else
                console_prompt();
        } else {
            // ignored;
        }
    }
    return NULL;
}

static void do_cmd(char *line)
{
    char *args[64];
    int count = 0;
    char *p;
    int word = 0;

    for (p = line; *p != 0; p++) {
        if (isspace(*p)) {
            *p = 0;
            word = 0;
        } else {
            if (word == 0) {
                args[count] = p;
                count++;
            }
            word = 1;
        }
    }

    if (count > 0) {
        struct command *p;

        for (p = commands; p->name; p++) {
            if (strcmp(args[0], p->name) == 0) {
                p->cmd_cb(&cmd_ctx, count, args);
                return;
            }
        }
        console("unknown command: %s", args[0]);
    }
}

static void deinit()
{
    if (cmd_ctx.cfg)
        deref(cmd_ctx.cfg);

    if (cmd_ctx.connect)
        hive_client_disconnect(cmd_ctx.connect);

    if (cmd_ctx.client)
        hive_client_close(cmd_ctx.client);
}

static void usage(void)
{
    printf("hivecmd, a CLI application to demonstrate use of hive apis.\n");
    printf("Usage: hivecmd [OPTION]...\n");
    printf("\n");
    printf("First run options:\n");
    printf("  -c, --config=CONFIG_FILE      Set config file path.\n");
    printf("\n");
    printf("Debugging options:\n");
    printf("      --debug                   Wait for debugger attach after start.\n");
    printf("\n");
}

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>

int sys_coredump_set(bool enable)
{
    const struct rlimit rlim = {
        enable ? RLIM_INFINITY : 0,
        enable ? RLIM_INFINITY : 0
    };

    return setrlimit(RLIMIT_CORE, &rlim);
}
#endif

void signal_handler(int signum)
{
    exit(-1);
}

int main(int argc, char *argv[])
{
    int wait_for_attach = 0;
    char path[2048] = {0};
    HiveOptions opts;
    cmd_cfg_t *cfg;
    struct stat st;
    char *cmd;
    int rc;

    int opt;
    struct option options[] = {
        {"config", required_argument, NULL, 'c'},
        {"debug",  no_argument,       NULL,  2 },
        {"help",   no_argument,       NULL, 'h'},
        {NULL,     0,        NULL,  0 }
    };

#ifdef HAVE_SYS_RESOURCE_H
    sys_coredump_set(true);
#endif

    while ((opt = getopt_long(argc, argv, "c:t:s:h?", options, NULL)) != -1) {
        switch (opt) {
        case 'c':
            strcpy(path, optarg);
            break;
        case 2:
            wait_for_attach = 1;
            break;
        case 'h':
        case '?':
        default:
            usage();
            exit(-1);
        }
    }

    if (wait_for_attach) {
        printf("Wait for debugger attaching, process id is: %d.\n", getpid());
        printf("After debugger attached, press any key to continue......\n");
        getchar();
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGSEGV, signal_handler);
#if !defined(_WIN32) && !defined(_WIN64)
    signal(SIGKILL, signal_handler);
    signal(SIGHUP, signal_handler);
#endif

    srand((unsigned)time(NULL));

    if (!*path) {
        realpath(argv[0], path);
        strcat(path, ".conf");
    }

    rc = stat(path, &st);
    if (rc < 0) {
        fprintf(stderr, "config file (%s) not exist.\n", path);
        return -1;
    }

    cfg = load_config(path);
    if (!cfg) {
        fprintf(stderr, "loading configure failed !\n");
        return -1;
    }

    hive_log_init(cfg->loglevel, cfg->logfile, logging);
    cmd_ctx.cfg = cfg;

    rc = mkdir(cfg->data_location, S_IRWXU);
    if (rc < 0 && errno != EEXIST) {
        fprintf(stderr, "failed to create directory %s\n", cfg->data_location);
        return -1;
    }

    opts.data_location = cfg->data_location;
    cmd_ctx.client = hive_client_new(&opts);
    if (!cmd_ctx.client) {
        fprintf(stderr, "failed to create HiveClient instance\n");
        return -1;
    }

    console_prompt();
    while ((cmd = read_cmd())) {
        do_cmd(cmd);
        if (cmd_ctx.stop)
            break;
        console_prompt();
    }

    deinit();
    return 0;
}
