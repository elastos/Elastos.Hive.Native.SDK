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
#include <limits.h>
#include <pthread.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
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
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#include <windows.h>
#include <shellapi.h>
#endif

#include <ela_hive.h>

#include "config.h"
#include "constants.h"

typedef struct {
    cmd_cfg_t *cfg;
    HiveClient *client;
    HiveDrive *drive;
    HiveFile *file;
    bool stop;
} cmd_t;

static cmd_t cmd_ctx;
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

static void client_new(cmd_t *ctx, int argc, char *argv[])
{
    if (argc != 2) {
        console("Error: invalid command syntax.");
        return;
    }

    if (ctx->client) {
        console("Error: a client instance already exists.");
        return;
    }

    if (!strcmp(argv[1], "onedrive")) {
        OneDriveOptions opts = {
            .base.drive_type          = HiveDriveType_OneDrive,
            .base.persistent_location = ctx->cfg->persistent_location,
            .redirect_url             = HIVETEST_REDIRECT_URL,
            .scope                    = HIVETEST_SCOPE,
            .client_id                = HIVETEST_ONEDRIVE_CLIENT_ID
        };

        ctx->client = hive_client_new((HiveOptions *)&opts);
        if (!ctx->client) {
            console("create hive client instance failure.\n");
            return;
        }
    } else if (!strcmp(argv[1], "ipfs")) {
        cmd_cfg_t *cfg = ctx->cfg;
        int i;

        HiveRpcNode *nodes = calloc(1, sizeof(HiveRpcNode) * cfg->ipfs_rpc_nodes_sz);
        if (!nodes) {
            console("create hive client instance failure.\n");
            return;
        }

        for (i = 0; i < cfg->ipfs_rpc_nodes_sz; ++i) {
            HiveRpcNode *node = nodes + i;

            node->ipv4 = cfg->ipfs_rpc_nodes[i]->ipv4;
            node->ipv6 = cfg->ipfs_rpc_nodes[i]->ipv6;
            node->port = cfg->ipfs_rpc_nodes[i]->port;
        }

        IPFSOptions opts = {
            .base.persistent_location = cfg->persistent_location,
            .base.drive_type = HiveDriveType_IPFS,
            .uid = cfg->uid,
            .rpc_node_count = cfg->ipfs_rpc_nodes_sz,
            .rpcNodes = nodes
        };

        ctx->client = hive_client_new((HiveOptions *)&opts);
        free(nodes);
        if (!cmd_ctx.client) {
            console("create hive client instance failure.\n");
            return;
        }
    } else {
        console("Error: unsupported backend type.");
    }
}

static void client_close(cmd_t *ctx, int argc, char *argv[])
{
    if (argc != 1) {
        console("Error: invalid command syntax.");
        return;
    }

    if (ctx->file) {
        hive_file_close(ctx->file);
        ctx->file = NULL;
    }

    if (ctx->drive) {
        hive_drive_close(ctx->drive);
        ctx->drive = NULL;
    }

    if (ctx->client) {
        hive_client_close(ctx->client);
        ctx->client = NULL;
    }
}

static void login(cmd_t *ctx, int argc, char *argv[])
{
    int rc;

    if (argc != 1) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->client) {
        console("Error: No client instance created.");
        return;
    }

    rc = hive_client_login(ctx->client, open_url, NULL);
    if (rc < 0) {
        console("Error: login failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
        return;
    }

    ctx->drive = hive_drive_open(ctx->client);
    if (!ctx->drive)
        console("Error: create drive failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void logout(cmd_t *ctx, int argc, char *argv[])
{
   int rc;

    if (argc != 1) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->client) {
        console("Error: No client instance created.");
        return;
    }

    rc = hive_client_logout(ctx->client);
    if (rc < 0)
        console("Error: logout failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void client_info(cmd_t *ctx, int argc, char *argv[])
{
    HiveClientInfo info;
    int rc;

    if (argc != 1) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->client) {
        console("Error: No client instance created.");
        return;
    }

    rc = hive_client_get_info(ctx->client, &info);
    if (rc < 0) {
        console("Error: get client info failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
        return;
    }

    console("user id: %s", info.user_id);
    console("display name: %s", info.display_name);
    console("email: %s", info.email);
    console("phone number: %s", info.phone_number);
    console("region: %s", info.region);
}

static void drive_info(cmd_t *ctx, int argc, char *argv[])
{
    HiveDriveInfo info;
    int rc;

    if (argc != 1) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->drive) {
        console("Error: get drive info failed. Reason: not login.");
        return;
    }

    rc = hive_drive_get_info(ctx->drive, &info);
    if (rc < 0) {
        console("Error: get drive info failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
        return;
    }

    console("drive id: %s", info.driveid);
}

static void file_info(cmd_t *ctx, int argc, char *argv[])
{
    HiveFileInfo info;
    int rc;

    if (argc != 2) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->drive) {
        console("Error: get file info failed. Reason: not login.");
        return;
    }

    rc = hive_drive_file_stat(ctx->drive, argv[1], &info);
    if (rc < 0) {
        console("Error: get file info failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
        return;
    }

    console("file id: %s", info.fileid);
    console("type: %s", info.type);
    console("size: %zu", info.size);
}

static bool ls_cb(const KeyValue *info, size_t size, void *context)
{
    static int nth = 0;
    size_t i;

    if (nth && info)
        console("");

    for (i = 0; i < size; ++i)
        console("%s: %s", info[i].key, info[i].value);

    nth = info ? nth + 1 : 0;

    return true;
}

static void ls(cmd_t *ctx, int argc, char *argv[])
{
    int rc;

    if (argc != 2) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->drive) {
        console("Error: ls failed. Reason: not login.");
        return;
    }

    rc = hive_drive_list_files(ctx->drive, argv[1], ls_cb, NULL);
    if (rc < 0)
        console("Error: ls failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void makedir(cmd_t *ctx, int argc, char *argv[])
{
    int rc;

    if (argc != 2) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->drive) {
        console("Error: mkdir failed. Reason: not login.");
        return;
    }

    rc = hive_drive_mkdir(ctx->drive, argv[1]);
    if (rc < 0)
        console("Error: mkdir failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void mv(cmd_t *ctx, int argc, char *argv[])
{
    int rc;

    if (argc != 3) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->drive) {
        console("Error: mv failed. Reason: not login.");
        return;
    }

    rc = hive_drive_move_file(ctx->drive, argv[1], argv[2]);
    if (rc < 0)
        console("Error: mv failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void cp(cmd_t *ctx, int argc, char *argv[])
{
    int rc;

    if (argc != 3) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->drive) {
        console("Error: cp failed. Reason: not login.");
        return;
    }

    rc = hive_drive_copy_file(ctx->drive, argv[1], argv[2]);
    if (rc < 0)
        console("Error: cp failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void rm(cmd_t *ctx, int argc, char *argv[])
{
    int rc;

    if (argc != 2) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->drive) {
        console("Error: rm failed. Reason: not login.");
        return;
    }

    rc = hive_drive_delete_file(ctx->drive, argv[1]);
    if (rc < 0)
        console("Error: rm failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void file_open(cmd_t *ctx, int argc, char *argv[])
{
    if (argc != 3) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->drive) {
        console("Error: fopen failed. Reason: not login.");
        return;
    }

    if (ctx->file) {
        console("Error: fopen failed. Reason: a file is already opened.");
        return;
    }

    ctx->file = hive_file_open(ctx->drive, argv[1], argv[2]);
    if (!ctx->file)
        console("Error: fopen failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void file_close(cmd_t *ctx, int argc, char *argv[])
{
    int rc;

    if (argc != 1) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->drive) {
        console("Error: fclose failed. Reason: not login.");
        return;
    }

    if (!ctx->file) {
        console("Error: fclose failed. Reason: no file is opened.");
        return;
    }

    rc = hive_file_close(ctx->file);
    if (rc < 0) {
        console("Error: fclose failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
        return;
    }

    ctx->file = NULL;
}

static void file_seek(cmd_t *ctx, int argc, char *argv[])
{
    ssize_t rc;
    ssize_t offset;
    char *endptr;
    int whence;

    if (argc != 3) {
        console("Error: invalid command syntax.");
        return;
    }

    offset = strtol(argv[1], &endptr, 10);
    if (*endptr) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!strcmp(argv[2], "set"))
        whence = HiveSeek_Set;
    else if (!strcmp(argv[2], "cur"))
        whence = HiveSeek_Cur;
    else if (!strcmp(argv[2], "end"))
        whence = HiveSeek_End;
    else {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->drive) {
        console("Error: fseek failed. Reason: not login.");
        return;
    }

    if (!ctx->file) {
        console("Error: fseek failed. Reason: no file is opened.");
        return;
    }

    rc = hive_file_seek(ctx->file, offset, whence);
    if (rc < 0)
        console("Error: fseek failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void file_read(cmd_t *ctx, int argc, char *argv[])
{
    ssize_t rc;
    size_t size;
    char *endptr;
    char *buf;

    if (argc != 2) {
        console("Error: invalid command syntax.");
        return;
    }

    size = strtol(argv[1], &endptr, 10);
    if (*endptr || size <= 0) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->drive) {
        console("Error: fread failed. Reason: not login.");
        return;
    }

    if (!ctx->file) {
        console("Error: fread failed. Reason: no file is opened.");
        return;
    }

    buf = alloca(size + 1);

    rc = hive_file_read(ctx->file, buf, size);
    if (rc < 0) {
        console("Error: fread failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
        return;
    }

    buf[rc] = '\0';
    console("%s", buf);
}

static void file_write(cmd_t *ctx, int argc, char *argv[])
{
    ssize_t rc;

    if (argc != 2) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->drive) {
        console("Error: fwrite failed. Reason: not login.");
        return;
    }

    if (!ctx->file) {
        console("Error: fwrite failed. Reason: no file is opened.");
        return;
    }

    rc = hive_file_write(ctx->file, argv[1], strlen(argv[1]));
    if (rc < 0)
        console("Error: fwrite failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void file_commit(cmd_t *ctx, int argc, char *argv[])
{
    int rc;

    if (argc != 1) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->drive) {
        console("Error: fcommit failed. Reason: not login.");
        return;
    }

    if (!ctx->file) {
        console("Error: fcommit failed. Reason: no file is opened.");
        return;
    }

    rc = hive_file_commit(ctx->file);
    if (rc < 0)
        console("Error: fcommit failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void file_discard(cmd_t *ctx, int argc, char *argv[])
{
    int rc;

    if (argc != 1) {
        console("Error: invalid command syntax.");
        return;
    }

    if (!ctx->drive) {
        console("Error: fdiscard failed. Reason: not login.");
        return;
    }

    if (!ctx->file) {
        console("Error: fdiscard failed. Reason: no file is opened.");
        return;
    }

    rc = hive_file_discard(ctx->file);
    if (rc < 0)
        console("Error: fdiscard failed. Reason: %s.",
                hive_get_strerror(hive_get_error(), errbuf, sizeof(errbuf)));
}

static void exit_app(cmd_t *ctx, int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    ctx->stop = true;
}

static void help(cmd_t *ctx, int argc, char *argv[]);
static struct command {
    const char *name;
    void (*cmd_cb)(cmd_t *, int argc, char *argv[]);
    const char *help;
} commands[] = {
        { "help"        , help        , "help [cmd]"       },
        { "client_open" , client_new  , "client_open type" },
        { "client_close", client_close, "client_close"     },
        { "login"       , login       , "login"            },
        { "logout"      , logout      , "logout"           },
        { "client_info" , client_info , "client_info"      },
        { "drive_info"  , drive_info  , "drive_info"       },
        { "file_info"   , file_info   , "file_info path"   },
        { "ls"          , ls          , "ls path"          },
        { "mkdir"       , makedir     , "mkdir directory"  },
        { "mv"          , mv          , "mv source target" },
        { "cp"          , cp          , "cp source target" },
        { "rm"          , rm          , "rm path"          },
        { "fopen"       , file_open   , "fopen path mode"  },
        { "fclose"      , file_close  , "fclose"           },
        { "fseek"       , file_seek   , "fseek offset whence(set, cur, end)" },
        { "fread"       , file_read   , "fread size"       },
        { "fwrite"      , file_write  , "fwrite data"      },
        { "fcommit"     , file_commit , "fcommit"          },
        { "fdiscard"    , file_discard, "fdiscard"         },
        { "exit"        , exit_app    , "exit"             },
        { NULL          , NULL        , NULL               }
};

static void help(cmd_t *ctx, int argc, char *argv[])
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

    if (cmd_ctx.file)
        hive_file_close(cmd_ctx.file);

    if (cmd_ctx.drive)
        hive_drive_close(cmd_ctx.drive);

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
    cmd_cfg_t *cfg;
    char path[2048] = {0};
    struct stat st;
    int wait_for_attach = 0;
    int rc;
    char *cmd;

    int opt;
    struct option options[] = {
        {"config", required_argument, NULL, 'c'},
        {"debug",  no_argument,       NULL,  2 },
        {"help",   no_argument,       NULL, 'h'},
        {NULL,     0,                 NULL,  0 }
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

    ela_log_init(cfg->loglevel, cfg->logfile, logging);
    cmd_ctx.cfg = cfg;

    rc = mkdir(cfg->persistent_location, S_IRWXU);
    if (rc < 0 && errno != EEXIST) {
        fprintf(stderr, "failed to create directory %s\n", cfg->persistent_location);
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
