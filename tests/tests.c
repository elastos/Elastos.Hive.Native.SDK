#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
//#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <crystal.h>

#include "suites.h"
#include "config.h"

#define CONFIG_NAME   "tests.conf"

static const char *config_files[] = {
    "./"CONFIG_NAME,
    "../etc/carrier/"CONFIG_NAME,
#if !defined(_WIN32) && !defined(_WIN64)
    "/usr/local/etc/carrier/"CONFIG_NAME,
    "/etc/carrier/"CONFIG_NAME,
#endif
    NULL
};

#ifdef HAVE_SETRLIMIT
int sys_coredump_set(bool enable)
{
    const struct rlimit rlim = {
        enable ? RLIM_INFINITY : 0,
        enable ? RLIM_INFINITY : 0
    };

    return setrlimit(RLIMIT_CORE, &rlim);
}
#else
int sys_coredump_set(bool enable)
{
    return 0;
}
#endif

static void usage(void)
{
    printf("Hive API unit tests.\n");
    printf("\n");
    printf("Usage: hivetest -c CONFIG\n");
    printf("\n");
}

const char *get_config_path(const char *config_file, const char *config_files[])
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

int main(int argc, char *argv[])
{
    TestSuite *ts;
    CU_ErrorCode rc;

    const char *config_file = NULL;
    int opt;
    int idx;

    getchar();
    //add by chenyu
    //printf("\nget argc = %d, argv = %s\n", argc, argv);

    struct option options[] = {
        { "debug",          no_argument,        NULL, 1   },
        { "config",         required_argument,  NULL, 'c' },
        { "help",           no_argument,        NULL, 'h' },
        { NULL,             0,                  NULL, 0 }
    };

    sys_coredump_set(true);

    while ((opt = getopt_long(argc, argv, "c:h:r:?", options, &idx)) != -1) {
        switch (opt) {
        case 1:
        case 'c':
            config_file = optarg;
            break;

        case 'h':
        case '?':
            usage();
            return -1;
        }
    }

    config_file = get_config_path(config_file, config_files);

    if (!config_file) {
        printf("Error: Missing config file.\n");
        usage();
        return -1;
    }

    load_config(config_file);

    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    for (ts = suites; ts->fileName != NULL; ts++) {
        CU_SuiteInfo *si = ts->getSuiteInfo();
        rc = CU_register_nsuites(1, si);

        if(rc != CUE_SUCCESS){
            CU_cleanup_registry();
            return rc;
        }
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);

    CU_basic_run_tests();

    CU_cleanup_registry();

    return CU_get_error();
}
