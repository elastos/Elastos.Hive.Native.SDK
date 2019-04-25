#include <stdlib.h>
#include <CUnit/Basic.h>

#include <hive.h>

static void asynctest_hive_new_1drv(void)
{
    return;
}

static void asynctest_hive_1drv_dir(void)
{
    return;
}

static int hive_asynctest_suite_init(void)
{
    return 0;
}

static int hive_asynctest_suite_cleanup(void)
{
    return 0;
}

static CU_TestInfo cases[] = {
    {   "asynctest_hive_new_1drv", asynctest_hive_new_1drv },
    {   "asynctest_hive_1drv_dir", asynctest_hive_1drv_dir },
    {   NULL, NULL }
};

static CU_SuiteInfo suite[] = {
    {   "hive async test", hive_asynctest_suite_init, hive_asynctest_suite_cleanup, NULL, NULL, cases },
    {    NULL,       NULL,                      NULL,                         NULL, NULL, NULL}
};

CU_SuiteInfo* hive_asynctest_suite_info(void)
{
    return suite;
}