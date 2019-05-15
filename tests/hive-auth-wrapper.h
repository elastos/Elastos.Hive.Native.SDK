#ifndef __HIVE_AUTH_WRAPPER_H__
#define __HIVE_AUTH_WRAPPER_H__

#include <hive.h>

int __hive_authorize(hive_t *hive);

int __hive_revoke(hive_t *hive);

#endif // __HIVE_AUTH_WRAPPER_H__
