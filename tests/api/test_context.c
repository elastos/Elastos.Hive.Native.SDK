#include <string.h>

#include "test_context.h"

test_context test_ctx;

void test_context_cleanup()
{
    if (test_ctx.file)
        hive_file_close(test_ctx.file);

    if (test_ctx.drive)
        hive_drive_close(test_ctx.drive);

    if (test_ctx.client)
        hive_client_close(test_ctx.client);

    memset(&test_ctx, 0, sizeof(test_ctx));
}

