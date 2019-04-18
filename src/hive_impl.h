#ifndef __HIVE_IMPL_H__
#define __HIVE_IMPL_H__

typedef struct hive hive_t;

struct hive {
    int (*authorize)(hive_t *hive);
    int (*mkdir)(hive_t *hive, const char *path);
    int (*list)(hive_t *hive, const char *path, char **result);
    int (*copy)(hive_t *hive, const char *src_path, const char *dest_path);
};

#endif
