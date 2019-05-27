#include <CUnit/Basic.h>

typedef CU_SuiteInfo* (*SuiteInfoFunc)(void);

typedef struct TestSuite {
    const char* fileName;
    SuiteInfoFunc getSuiteInfo;
} TestSuite;

CU_SuiteInfo* hive_client_new_test_suite_info(void);
CU_SuiteInfo* hive_client_getinfo_suite_info(void);
CU_SuiteInfo* hive_client_login_test_suite_info(void);
CU_SuiteInfo* hive_client_logining_test_suite_info(void);
CU_SuiteInfo* hive_client_opendrive_suite_info(void);
CU_SuiteInfo* hive_client_drivelist_suite_info(void);
CU_SuiteInfo* hive_drive_getinfo_suite_info(void);
CU_SuiteInfo* hive_drive_mkidr_test_suite_info(void);
CU_SuiteInfo* hive_drive_filestat_test_suite_info(void);
CU_SuiteInfo* hive_drive_filelist_test_suite_info(void);
CU_SuiteInfo* hive_drive_copy_test_suite_info(void);
CU_SuiteInfo* hive_drive_move_test_suite_info(void);
CU_SuiteInfo* hive_expired_token_test_suite_info(void);

TestSuite suites[] = {
    { "hive-client-new-test.c",         hive_client_new_test_suite_info        },
    { "hive-client-getinfo-test.c",     hive_client_getinfo_suite_info         },
    { "hive-client-login-test.c",       hive_client_login_test_suite_info      },
    { "hive-client-logining-test.c",    hive_client_logining_test_suite_info   },
    { "hive-client-opendrive-test.c",   hive_client_opendrive_suite_info       },
    { "hive-client-drivelist-test.c",   hive_client_drivelist_suite_info       },
    { "hive-drive-getinfo-test.c",      hive_drive_getinfo_suite_info          },
    { "hive-drive_mkdir-test.c",        hive_drive_mkidr_test_suite_info       },
    { "hive-drive-filestat-test.c",     hive_drive_filestat_test_suite_info    },
    { "hive-drive-filelist-test.c",     hive_drive_filelist_test_suite_info    },
    { "hive-drive-copy-test.c",         hive_drive_copy_test_suite_info        },
    { "hive-drive-move-test.c",         hive_drive_move_test_suite_info        },
    { "hive-expired-token-test.c",      hive_expired_token_test_suite_info     },
    { NULL,                             NULL                                   }
};
