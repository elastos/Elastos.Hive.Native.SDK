#ifndef __ONEDRIVE_CONSTANTS_H__
#define __ONEDRIVE_CONSTANTS_H__

#define URL_OAUTH   "https://login.microsoftonline.com/common/oauth2/v2.0/"
#define URL_API     "https://graph.microsoft.com/v1.0/me"

#define MAX_URL_LEN         (1024)
#define MAX_URL_PARAM_LEN   (1024 - strlen(URL_API) - 32)

#define MAX_CTAG_LEN        (1024)

#define MY_DRIVE    "https://graph.microsoft.com/v1.0/me/drive"

#define METHOD_AUTHORIZE "authorize"
#define METHOD_TOKEN     "token"

#endif // __ONEDRIVE_CONSTANTS_H__
