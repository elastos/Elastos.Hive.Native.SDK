#ifndef __TEST_HELPER_H__
#define __TEST_HELPER_H__

#include <ela_hive.h>

HiveClient *onedrive_client_new();
HiveClient *ipfs_client_new();
int open_authorization_url(const char *url, void *context);
#endif // __TEST_HELPER_H__
