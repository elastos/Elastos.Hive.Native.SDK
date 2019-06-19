#include <crystal.h>

#include "token_base.h"
#include "hive_error.h"

int token_login(token_base_t *token,
                HiveRequestAuthenticationCallback *callback,
                void *context)
{
    int rc;

    /*
     * Check login state.
     * 1. If already logined, return OK immediately, else
     * 2. if being in progress of logining, then return error. else
     * 3. It's in raw state, conduct the login process.
     */
    rc = check_and_login(token);
    switch(rc) {
    case 0:
        break;

    case 1:
        vlogD("Hive: This token logined already");
        return 0;

    case -1:
    default:
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_WRONG_STATE));
        return -1;
    }

    if (token->login)
        rc = token->login(token, callback, context);

    if (rc < 0) {
        // recover back to 'RAW' state.
        _test_and_swap(&token->state, LOGINING, RAW);
        hive_set_error(rc);
        return -1;
    }

    // When conducting all login stuffs successfully, then change to be
    // 'LOGINED'.
    _test_and_swap(&token->state, LOGINING, LOGINED);
    return 0;
}

int token_logout(token_base_t *token)
{
    int rc;

    rc = check_and_logout(token);
    switch(rc) {
    case 0:
        break;

    case 1:
        return 0;

    case -1:
    default:
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_WRONG_STATE));
        return -1;
    }

    if (token->logout)
        token->logout(token);

    _test_and_swap(&token->state, LOGOUTING, RAW);

    return 0;
}
