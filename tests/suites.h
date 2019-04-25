#include <CUnit/Basic.h>

typedef CU_SuiteInfo* (*SuiteInfoFunc)(void);

typedef struct TestSuite {
    const char* fileName;
    SuiteInfoFunc getSuiteInfo;
} TestSuite;

CU_SuiteInfo* hive_new_test_suite_info(void);
CU_SuiteInfo* hive_client_getinfo_suite_info(void)
CU_SuiteInfo* hive_login_test_suite_info(void);
CU_SuiteInfo* hive_logining_test_suite_info(void);
CU_SuiteInfo* hive_mkidr_test_suite_info(void);
CU_SuiteInfo* hive_expired_token_test_suite_info(void);
CU_SuiteInfo* hive_no_file_test_suite_info(void);
CU_SuiteInfo* hive_file_test_suite_info(void);
//CU_SuiteInfo* hive_asynctest_suite_info(void);

TestSuite suites[] = {
    { "hive-new-test.c",            hive_new_test_suite_info            },
    { "hive-client-getinfo-test.c", hive_client_getinfo_suite_info      },
    { "hive-login-test.c",          hive_login_test_suite_info          },
    { "hive-logining-test.c",       hive_logining_test_suite_info       },
    { "hive-mkdir-test.c",          hive_mkidr_test_suite_info          },
    { "hive-expired-token-test.c",  hive_expired_token_test_suite_info  },
    { "hive-no_file-test.c",        hive_no_file_test_suite_info        },
    { "hive-file-test.c",           hive_file_test_suite_info           },
    //{ "hive-asynctest.c",        hive_asynctest_suite_info           },
    { NULL,                     NULL                                }
};
