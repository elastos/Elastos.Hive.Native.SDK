#include <stdbool.h>
#include <sys/time.h>
#include <unistd.h>

#include "hive-auth-wrapper.h"

static enum {
    unknown,
    not_authorized,
    authorized
} auth_state;

static struct timeval next_auth;

static void cool_down()
{
    struct timeval now;

    if (!next_auth.tv_sec)
        return;

    gettimeofday(&now, NULL);

    if (now.tv_sec >= next_auth.tv_sec)
        return;

    sleep(next_auth.tv_sec - now.tv_sec);
}

int __hive_authorize(hive_t *hive)
{
    int rc;
    struct timeval now;

    cool_down();

    rc = hive_authorize(hive);

    gettimeofday(&now, NULL);

    next_auth.tv_sec = auth_state == authorized ?
        now.tv_sec : now.tv_sec + 2;

    auth_state = rc ? unknown : authorized;

    return rc;
}

int __hive_revoke(hive_t *hive)
{
    int rc;

    rc = hive_revoke(hive);

    auth_state = rc ? unknown : not_authorized;

    return rc;
}
