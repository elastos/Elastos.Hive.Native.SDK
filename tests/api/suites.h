#ifndef __API_TEST_SUITES_H__
#define __API_TEST_SUITES_H__

#include <CUnit/Basic.h>

typedef CU_TestInfo* (*CU_CasesFunc)(void);

typedef struct TestSuite {
    const char* fileName;
    const char* strName;
    CU_CasesFunc pCases;
    CU_InitializeFunc pInit;
    CU_CleanupFunc pClean;
    CU_SetUpFunc pSetUp;
    CU_TearDownFunc pTearDown;
} TestSuite;

#define DECL_TESTSUITE(mod, backend) \
    int backend##_##mod##_suite_init(void); \
    int backend##_##mod##_suite_cleanup(void); \
    CU_TestInfo *mod##_get_cases(void);

#define DECL_TESTSUITE_PER_BACKEND(mod) \
    DECL_TESTSUITE(mod, onedrive) \
    DECL_TESTSUITE(mod, ipfs)

#define DEFINE_TESTSUIT(mod, backend) \
    { \
        .fileName = #mod".c", \
        .strName  = #backend"_"#mod, \
        .pCases   = mod##_get_cases, \
        .pInit    = backend##_##mod##_suite_init, \
        .pClean   = backend##_##mod##_suite_cleanup, \
        .pSetUp   = NULL, \
        .pTearDown= NULL \
    }

#define DEFINE_TESTSUITE_PER_BACKEND(mod) \
    DEFINE_TESTSUIT(mod, onedrive), \
    DEFINE_TESTSUIT(mod, ipfs)

#define DEFINE_TESTSUITE_NULL \
    { \
        .fileName = NULL, \
        .strName  = NULL, \
        .pCases   = NULL, \
        .pInit    = NULL, \
        .pClean   = NULL, \
        .pSetUp   = NULL, \
        .pTearDown= NULL  \
    }

#include "client/suites.h"
#include "drive/suites.h"
#include "file/suites.h"

TestSuite suites[] = {
    DEFINE_CLIENT_TESTSUITES,
    DEFINE_DRIVE_TESTSUITES,
    DEFINE_FILE_TESTSUITES,
    DEFINE_TESTSUITE_NULL
};

#endif /* __API_TEST_SUITES_H__ */
