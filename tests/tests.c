#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include <crystal.h>

//#include <CUnit/Basic.h>

#include "suites.h"

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

static void signal_handler(int signum)
{
    printf("Got signal: %d\n", signum);
    exit(-1);
}

int main(int argc, char *argv[])
{
    TestSuite *ts;
    CU_ErrorCode rc;

    sys_coredump_set(true);
    vlogD("**********");

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
