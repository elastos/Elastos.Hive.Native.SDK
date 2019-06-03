#include <stdlib.h>
#include <assert.h>

#include "native_client.h"

int native_client_new(const HiveOptions *options, HiveClient **client)
{
    assert(options);
    assert(client);

    (void)options;
    (void)client;

    // TODO;
    return -1;
}
