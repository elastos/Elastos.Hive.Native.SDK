#ifndef __HIVE_IMPL_H__
#define __HIVE_IMPL_H__

#include "hive.h"

HIVE_API
int hive_set_expired(hive_t *hive);

struct hive {
    int (*authorize)(hive_t *hive);
    int (*revoke)(hive_t *hive);
    int (*makedir)(hive_t *hive, const char *path);
    int (*list)(hive_t *hive, const char *path, char **result);
    int (*copy)(hive_t *hive, const char *src_path, const char *dest_path);
    int (*delete)(hive_t *hive, const char *path);
    int (*move)(hive_t *hive, const char *old, const char *new);
    int (*stat)(hive_t *hive, const char *path, char **result);
    int (*set_expired)(hive_t *hive);
};

#endif
