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

#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef HAVE_PROCESS_H
#include <process.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <crystal.h>
#include <CUnit/Basic.h>

#include "config.h"
#include "suites.h"

#define CONFIG_NAME   "tests.conf"

static const char *default_config_files[] = {
        "./"CONFIG_NAME,
        "../etc/hive/"CONFIG_NAME,
#if !defined(_WIN32) && !defined(_WIN64)
        "/usr/local/etc/hive/"CONFIG_NAME,
        "/etc/hive/"CONFIG_NAME,
#endif
        NULL
};

static void signal_handler(int signum)
{
    printf("Got signal: %d, force exit.\n", signum);

    config_deinit();

    exit(-1);
}

const char *get_config_file(const char *config_file, const char *config_files[])
{
    const char **file = config_file ? &config_file : config_files;

    for (; *file; ) {
        int fd = open(*file, O_RDONLY);
        if (fd < 0) {
            if (*file == config_file)
                file = config_files;
            else
                file++;

            continue;
        }

        close(fd);

        return *file;
    }

    return NULL;
}

void config_update(int argc, char *argv[])
{
    char path[PATH_MAX];

    int opt;
    int idx;
    struct option cmd_options[] = {
            { "log-level",      required_argument,  NULL, 1 },
            { "log-file",       required_argument,  NULL, 2 },
            { "data-dir",       required_argument,  NULL, 3 },
            { NULL,             0,                  NULL, 0 }
    };

    optind = 0;
    opterr = 0;
    optopt = 0;

    while ((opt = getopt_long(argc, argv, "", cmd_options, &idx)) != -1) {
        switch (opt) {
        case 1:
            global_config.loglevel = atoi(optarg);
            break;

        case 2:
            if (global_config.logfile)
                free(global_config.logfile);

            qualified_path(optarg, NULL, path);
            global_config.logfile = strdup(path);
            break;

        case 3:
            if (global_config.data_dir)
                free(global_config.data_dir);

            qualified_path(optarg, NULL, path);
            global_config.data_dir = strdup(path);
            break;

        default:
            break;
        }
    }

    optind = 0;
    opterr = 0;
    optopt = 0;
}

void shuffle(int *order, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        int rnd = rand() % count;
        if (rnd == i)
            continue;

        int tmp = order[i];
        order[i] = order[rnd];
        order[rnd] = tmp;
    }
}

void wait_for_debugger_attach(void)
{
    fprintf(stderr, "\nWait for debugger attaching, process id is: %d.\n", getpid());
    fprintf(stderr, "After debugger attached, press any key to continue......\n");
    getchar();
}

int test_main(int argc, char *argv[])
{
    int rc;
    int retry_count = 1;
    int i, j;
    int debug = 0;
    CU_pSuite pSuite;
    CU_TestInfo *ti;
    int suites_cnt, cases_cnt, fail_cnt;
    int suites_order[64];
    int cases_order[64];

    const char *config_file = NULL;

    int opt;
    int idx;
    struct option options[] = {
        { "config",    required_argument, NULL, 'c' },
        { "log-level", required_argument, NULL, 1 },
        { "log-file",  required_argument, NULL, 2 },
        { "data-dir",  required_argument, NULL, 3 },
        { "debug",     no_argument,       NULL, 4 },
        { NULL,        0,                 NULL, 0 }
    };

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    while ((opt = getopt_long(argc, argv, "r:c:h", options, &idx)) != -1) {
        switch (opt) {
        case 1:
        case 2:
        case 3:
            break;

        case 4:
            debug = 1;
            break;

        case 'c':
            config_file = optarg;
            break;

        case 'r':
            retry_count = atoi(optarg);
            break;

        default:
            break;
        }
    }

    if (debug)
        wait_for_debugger_attach();

    // The primary job: load configuration file
    config_file = get_config_file(config_file, default_config_files);
    if (!config_file) {
        fprintf(stderr, "Error: Missing config file.\n");
        return -1;
    }

    if (!load_config(config_file)) {
        fprintf(stderr, "Loading configure failed !\n");
        return -1;
    }

    config_update(argc, argv);

    rc = mkdir(global_config.data_dir, S_IRWXU);
    if (rc < 0 && errno != EEXIST) {
        fprintf(stderr, "Creating data dir failed !\n");
        config_deinit();
        return -1;
    }

    ela_log_init(global_config.loglevel,
                 global_config.log2file ? global_config.logfile : NULL,
                 NULL);

    if (CUE_SUCCESS != CU_initialize_registry()) {
        config_deinit();
        return CU_get_error();
    }

    srand((unsigned int)time(NULL));

    for (suites_cnt = 0; suites[suites_cnt].fileName; suites_cnt++)
        suites_order[suites_cnt] = suites_cnt;

    if (suites_cnt > 1 && global_config.shuffle)
        shuffle(suites_order, suites_cnt);

    for (i = 0; i < suites_cnt; i++) {
        int suite_idx = suites_order[i];
        pSuite = CU_add_suite(suites[suite_idx].strName,
                              suites[suite_idx].pInit,
                              suites[suite_idx].pClean);
        if (NULL == pSuite) {
            config_deinit();
            CU_cleanup_registry();
            return CU_get_error();
        }

        ti = suites[suite_idx].pCases();
        for (cases_cnt = 0; ti[cases_cnt].pName; cases_cnt++)
            cases_order[cases_cnt] = cases_cnt;

        if (cases_cnt > 1 && global_config.shuffle)
            shuffle(cases_order, cases_cnt);

        for (j = 0; j < cases_cnt; j++) {
            if (CU_add_test(pSuite, ti[cases_order[j]].pName,
                            ti[cases_order[j]].pTestFunc) == NULL) {
                CU_cleanup_registry();
                config_deinit();
                return CU_get_error();
            }
        }
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_set_test_retry_count(retry_count);

    CU_basic_run_tests();

    fail_cnt = CU_get_number_of_tests_failed();
    if (fail_cnt > 0)
        fprintf(stderr, "Failure Case: %d\n", fail_cnt);

    CU_cleanup_registry();
    config_deinit();
    return fail_cnt;
}

