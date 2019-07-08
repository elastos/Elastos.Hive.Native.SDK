#ifndef __IPFS_CONSTANTS_H__
#define __IPFS_CONSTANTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <curl/curl.h>

#include "ela_hive.h"

#define MAX_URL_LEN         (1024)

#define CLUSTER_API_PORT (9094)
#define NODE_API_PORT (9095)

#define HIVE_MAX_IPV4_ADDRESS_LEN (15)
#define HIVE_MAX_IPV6_ADDRESS_LEN (47)
#define HIVE_MAX_IPFS_UID_LEN     (127)

#define RC_NODE_UNREACHABLE(rc)                              \
    ((rc) == HIVE_CURL_ERROR(CURLE_COULDNT_CONNECT)      || \
     (rc) == HIVE_CURL_ERROR(CURLE_REMOTE_ACCESS_DENIED) || \
     (rc) == HIVE_CURL_ERROR(CURLE_OPERATION_TIMEDOUT))

#ifdef __cplusplus
}
#endif

#endif // __IPFS_CONSTANTS_H__
