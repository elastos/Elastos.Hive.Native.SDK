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
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#ifdef HAVE_PROCESS_H
#include <process.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include <crystal.h>

#include "http_client.h"
#include "http_status.h"

static void usage(void)
{
    printf("prober, a utility detecting connectivity of IPFS nodes.\n");
    printf("Usage: prober [OPTION]... [NODE_IP[:NODE_PORT]] ...\n");
    printf("Description: prober tests connectivity of each IPFS nodes provided as command line"
           " arguments and nodes listed in the file specified by -f option. Node address takes"
           " the form NODE_IP[:NODE_PORT] where node port can be omitted to take the default port 9095.");
    printf("\n");
    printf("First run options:\n");
    printf("  -f, --file=FILE_PATH          File containing addresses of nodes to be tested."
           " Nodes are separated by whitespaces or newlines.\n");
    printf("\n");
    printf("Debugging options:\n");
    printf("      --debug                   Wait for debugger attach after start.\n");
    printf("\n");
}

#ifdef HAVE_SYS_RESOURCE_H
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

void probe(const char *node)
{
    http_client_t *httpc;
    char url[1024];
    long resp_code;
    int rc;

    printf("probing %s...", node);

    httpc = http_client_new();
    if (!httpc) {
        printf("internal error.\n");
        return;
    }

    snprintf(url, sizeof(url), "http://%s:9095/version", node);
    rc = http_client_set_url(httpc, url);
    if (rc) {
        snprintf(url, sizeof(url), "http://%s/version", node);
        rc = http_client_set_url(httpc, url);
        if (rc) {
            printf("invalid node address.\n");
            return;
        }
    }

    http_client_set_method(httpc, HTTP_METHOD_POST);
    http_client_set_request_body_instant(httpc, NULL, 0);
    http_client_set_timeout(httpc, 5);

    rc = http_client_request(httpc);
    if (rc) {
        printf("unreachable.\n");
        return;
    }

    rc = http_client_get_response_code(httpc, &resp_code);
    http_client_close(httpc);
    if (rc)  {
        printf("unreachable.\n");
        return;
    }

    if (resp_code != HttpStatus_OK) {
        printf("unreachable.\n");
        return;
    }

    printf("ok.\n");
}

void logging(const char *fmt, va_list args)
{
    //DO NOTHING.
}

int main(int argc, char *argv[])
{
    char file[2048] = {0};
    char node[256];
    FILE *fp;
    int wait_for_attach = 0;
    int i;


    int opt;
    struct option options[] = {
        {"file",   required_argument, NULL, 'f'},
        {"debug",  no_argument,       NULL,  2 },
        {"help",   no_argument,       NULL, 'h'},
        {NULL,     0,                 NULL,  0 }
    };

#ifdef HAVE_SYS_RESOURCE_H
    sys_coredump_set(true);
#endif
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    while ((opt = getopt_long(argc, argv, "f:h?", options, NULL)) != -1) {
        switch (opt) {
        case 'f':
            strcpy(file, optarg);
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

    vlog_init(3, NULL, logging);

    for (i = optind; i < argc; ++i)
        probe(argv[i]);

    if (!*file)
        return 0;

    fp = fopen(file, "r");
    if (!fp) {
        printf("cannot open file (%s).\n", file);
        return 0;
    }

    while (fscanf(fp, " %s ", node) == 1)
        probe(node);

    fclose(fp);
    return 0;
}
