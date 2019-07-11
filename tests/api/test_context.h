#ifndef __TEST_CONTEXT_H__
#define __TEST_CONTEXT_H__

#include <ela_hive.h>

typedef struct {
    HiveClient *client;
    HiveDrive  *drive;
    HiveFile   *file;
    void       *ext;
} test_context;

extern test_context test_ctx;

void test_context_cleanup();

#endif // __TEST_CONTEXT_H__
