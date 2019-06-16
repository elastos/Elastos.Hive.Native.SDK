#ifndef __TOKEN_BASE_H__
#define __TOKEN_BASE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ela_hive.h"

typedef struct token_base token_base_t;

struct token_base {
    int state;

    int (*login)        (token_base_t *, HiveRequestAuthenticationCallback, void *);
    int (*logout)       (token_base_t *);
};

/*
 * convinient internal APIs to check for getting athorization or not.
 */
enum {
    RAW          = (int)0,    // The primitive state.
    LOGINING     = (int)1,    // Being in the middle of logining.
    LOGINED      = (int)2,    // Already being logined.
    LOGOUTING    = (int)3,    // Being in the middle of logout.
};

#if defined(_WIN32) || defined(_WIN64)
#include <winnt.h>

inline static
int __sync_val_compare_and_swap(int *ptr, int oldval, int newval)
{
    return (int)InterlockedCompareExchange((LONG *)ptr,
                                           (LONG)newval, (LONG)oldval);
}
#else
#define _test_and_swap __sync_val_compare_and_swap
#endif

inline static int check_and_login(token_base_t *token)
{
    int rc;

    rc = _test_and_swap(&token->state, RAW, LOGINING);
    switch(rc) {
    case RAW:
        rc = 0; // need to proceed the login routine.
        break;

    case LOGINING:
    case LOGOUTING:
    default:
        rc = -1;
        break;

    case LOGINED:
        rc = 1;
        break;
    }

    return rc;
}

inline static int check_and_logout(token_base_t *token)
{
    int rc;

    rc = _test_and_swap(&token->state, LOGINED, LOGOUTING);
    switch(rc) {
    case RAW:
        rc = 1;
        break;

    case LOGINING:
    case LOGOUTING:
    default:
        rc = -1;
        break;

    case LOGINED:
        rc = 0;
        break;
    }

    return rc;
}

inline static bool has_valid_token(token_base_t *token)
{
    return LOGINED == _test_and_swap(&token->state, LOGINED, LOGINED);
}

int token_login(token_base_t *token,
                HiveRequestAuthenticationCallback callback,
                void *context);
int token_logout(token_base_t *token);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TOKEN_BASE_H__
