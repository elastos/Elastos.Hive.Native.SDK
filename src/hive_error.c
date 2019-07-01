#include <string.h>
#include <pthread.h>

#include "ela_hive.h"
#include "hive_error.h"
#include "http_client.h"

#if defined(_WIN32) || defined(_WIN64)
#define __thread        __declspec(thread)
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
static __thread int hive_error;
#elif defined(__APPLE__)
#include <pthread.h>
static pthread_once_t hive_key_once = PTHREAD_ONCE_INIT;
static pthread_key_t hive_error;
static void hive_setup_error(void)
{
    (void)pthread_key_create(&hive_error, NULL);
}
#else
#error "Unsupported OS yet"
#endif

int hive_get_error(void)
{
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
    return hive_error;
#elif defined(__APPLE__)
    return (int)pthread_getspecific(hive_error);
#else
#error "Unsupported OS yet"
#endif
}

void hive_clear_error(void)
{
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
    hive_error = HIVEOK;
#elif defined(__APPLE__)
    (void)pthread_setspecific(hive_error, 0);
#else
#error "Unsupported OS yet"
#endif
}

void hive_set_error(int err)
{
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
    hive_error = err;
#elif defined(__APPLE__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
    (void)pthread_once(&hive_key_once, hive_setup_error);
    (void)pthread_setspecific(hive_error, (void*)err);
#pragma GCC diagnostic pop
#else
#error "Unsupported OS yet"
#endif
}

typedef struct ErrorDesc {
    int errcode;
    const char *errdesc;
} ErrorDesc;

static
const ErrorDesc error_codes[] = {
    { HIVEERR_INVALID_ARGS,                "Invalid argument(s)"     },
    { HIVEERR_OUT_OF_MEMORY,               "Out of memory"           },
    { HIVEERR_BUFFER_TOO_SMALL,            "Too small buffer size"   },
    { HIVEERR_BAD_PERSISTENT_DATA,         "Bad persistent data"     },
    { HIVEERR_INVALID_PERSISTENCE_FILE,    "Invalid persistent file" },
    { HIVEERR_INVALID_CREDENTIAL,          "Invalid credential"      },
    { HIVEERR_NOT_READY,                   "SDK not ready"           },
    { HIVEERR_NOT_EXIST,                   "Entity not exists"       },
    { HIVEERR_ALREADY_EXIST,               "Entity already exists"   },
    { HIVEERR_INVALID_USERID,              "Invalid user id"         },
    { HIVEERR_WRONG_STATE,                 "Being in wrong state"    },
    { HIVEERR_BUSY,                        "Instance is being busy"  },
    { HIVEERR_LANGUAGE_BINDING,            "Language binding error"  },
    { HIVEERR_ENCRYPT,                     "Encrypt error"           },
    { HIVEERR_NOT_IMPLEMENTED,             "Not implemented yet"     },
    { HIVEERR_NOT_SUPPORTED,               "Not supported"           },
    { HIVEERR_LIMIT_EXCEEDED,              "Exceeding the limit"     },
    { HIVEERR_ENCRYPTED_PERSISTENT_DATA,   "Load encrypted persistent data error"},
    { HIVEERR_BAD_BOOTSTRAP_HOST,          "Bad bootstrap host"      },
    { HIVEERR_BAD_BOOTSTRAP_PORT,          "Bad bootstrap port"      },
    { HIVEERR_BAD_ADDRESS,                 "Bad carrier node address"},
    { HIVEERR_BAD_JSON_FORMAT,             "Bad json format"         },
    { HIVEERR_TRY_AGAIN,                   "Try again the operation" },
    { HIVEERR_UNKNOWN,                     "Unknown error"           }
};

static int general_error(int errcode, char *buf, size_t len)
{
    int size = sizeof(error_codes)/sizeof(ErrorDesc);
    int i;

    for (i = 0; i < size; i++) {
        if (errcode == error_codes[i].errcode)
            break;
    }

    if (i >= size || len <= strlen(error_codes[i].errdesc))
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    strcpy(buf, error_codes[i].errdesc);
    return 0;
}

static int system_error(int errcode, char *buf, size_t len)
{
    int rc;
#if defined(_WIN32) || defined(_WIN64)
    rc = strerror_s(buf, len, errcode);
#else
    rc = strerror_r(errcode, buf, len);
#endif
    if (rc < 0)
        return HIVE_SYS_ERROR(HIVEERR_INVALID_ARGS);

    return 0;
}

static int http_client_error(int errcode, char *buf, size_t len)
{
    const char *errstr;

    errstr = http_client_strerror(errcode);

    if (strlen(errstr) >= len)
        return HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL);

    strcpy(buf, errstr);
    return 0;
}
static
const ErrorDesc status_codes[] = {
    {HttpStatus_Continue                      , "Continue"                       },
    {HttpStatus_SwitchingProtocols            , "Switching Protocols"            },
    {HttpStatus_Processing                    , "Processing"                     },
    {HttpStatus_EarlyHints                    , "Early Hints"                    },
    {HttpStatus_OK                            , "OK"                             },
    {HttpStatus_Created                       , "Created"                        },
    {HttpStatus_Accepted                      , "Accepted"                       },
    {HttpStatus_NonAuthoritativeInformation   , "Non-Authoritative Information"  },
    {HttpStatus_NoContent                     , "No Content"                     },
    {HttpStatus_ResetContent                  , "Reset Content"                  },
    {HttpStatus_PartialContent                , "Partial Content"                },
    {HttpStatus_MultiStatus                   , "Multi-Status"                   },
    {HttpStatus_AlreadyReported               , "Already Reported"               },
    {HttpStatus_IMUsed                        , "IM Used"                        },
    {HttpStatus_MultipleChoices               , "Multiple Choices"               },
    {HttpStatus_MovedPermanently              , "Moved Permanently"              },
    {HttpStatus_Found                         , "Found"                          },
    {HttpStatus_SeeOther                      , "See Other"                      },
    {HttpStatus_NotModified                   , "Not Modified"                   },
    {HttpStatus_UseProxy                      , "Use Proxy"                      },
    {HttpStatus_TemporaryRedirect             , "Temporary Redirect"             },
    {HttpStatus_PermanentRedirect             , "Permanent Redirect"             },
    {HttpStatus_BadRequest                    , "Bad Request"                    },
    {HttpStatus_Unauthorized                  , "Unauthorized"                   },
    {HttpStatus_PaymentRequired               , "Payment Required"               },
    {HttpStatus_Forbidden                     , "Forbidden"                      },
    {HttpStatus_NotFound                      , "Not Found"                      },
    {HttpStatus_MethodNotAllowed              , "Method Not Allowed"             },
    {HttpStatus_NotAcceptable                 , "Not Acceptable"                 },
    {HttpStatus_ProxyAuthenticationRequired   , "Proxy Authentication Required"  },
    {HttpStatus_RequestTimeout                , "Request Timeout"                },
    {HttpStatus_Conflict                      , "Conflict"                       },
    {HttpStatus_Gone                          , "Gone"                           },
    {HttpStatus_LengthRequired                , "Length Required"                },
    {HttpStatus_PreconditionFailed            , "Precondition Failed"            },
    {HttpStatus_PayloadTooLarge               , "Payload Too Large"              },
    {HttpStatus_URITooLong                    , "URI Too Long"                   },
    {HttpStatus_UnsupportedMediaType          , "Unsupported Media Type"         },
    {HttpStatus_RangeNotSatisfiable           , "Range Not Satisfiable"          },
    {HttpStatus_ExpectationFailed             , "Expectation Failed"             },
    {HttpStatus_ImATeapot                     , "I'm a teapot"                   },
    {HttpStatus_UnprocessableEntity           , "Unprocessable Entity"           },
    {HttpStatus_Locked                        , "Locked"                         },
    {HttpStatus_FailedDependency              , "Failed Dependency"              },
    {HttpStatus_UpgradeRequired               , "Upgrade Required"               },
    {HttpStatus_PreconditionRequired          , "Precondition Required"          },
    {HttpStatus_TooManyRequests               , "Too Many Requests"              },
    {HttpStatus_RequestHeaderFieldsTooLarge   , "Request Header Fields Too Large"},
    {HttpStatus_UnavailableForLegalReasons    , "Unavailable For Legal Reasons"  },
    {HttpStatus_InternalServerError           , "Internal Server Error"          },
    {HttpStatus_NotImplemented                , "Not Implemented"                },
    {HttpStatus_BadGateway                    , "Bad Gateway"                    },
    {HttpStatus_ServiceUnavailable            , "Service Unavailable"            },
    {HttpStatus_GatewayTimeout                , "Gateway Time-out"               },
    {HttpStatus_HTTPVersionNotSupported       , "HTTP Version Not Supported"     },
    {HttpStatus_VariantAlsoNegotiates         , "Variant Also Negotiates"        },
    {HttpStatus_InsufficientStorage           , "Insufficient Storage"           },
    {HttpStatus_LoopDetected                  , "Loop Detected"                  },
    {HttpStatus_NotExtended                   , "Not Extended"                   },
    {HttpStatus_NetworkAuthenticationRequired , "Network Authentication Required"}
};

static int http_status_error(int errcode, char *buf, size_t len)
{
    int size = sizeof(status_codes)/sizeof(ErrorDesc);
    int i;

    for (i = 0; i < size; i++) {
        if (errcode == status_codes[i].errcode)
            break;
    }

    if (i >= size || len <= strlen(status_codes[i].errdesc))
        return HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS);

    strcpy(buf, status_codes[i].errdesc);
    return 0;
}

typedef struct FacilityDesc {
    const char *desc;
    strerror_func_t *errstring;
} FacilityDesc;

static FacilityDesc facility_codes[] = {
    { "[General] ",         general_error },     //ELAF_GENERAL
    { "[System] ",          system_error },      //ELAF_SYS
    { "Reserved facility",  NULL },              //ELAF_RESERVED1
    { "Reserved facility",  NULL },              //ELAF_RESERVED2
    { "[httpc] ",           http_client_error }, //ELAF_HTTP_CLIENT
    { "[httpstat] ",        http_status_error }, //ELAF_HTTP_STATUS
};

char *hive_get_strerror(int errnum, char *buf, size_t len)
{
    FacilityDesc *faci_desc;
    bool negative;
    int facility;
    int errcode;
    int rc = 0;
    size_t desc_len;
    char *p = buf;

    negative = !!(errnum & 0x80000000);
    facility = (errnum >> 24) & 0x0F;
    errcode  = errnum & 0x00FFFFFF;

    if (!buf || !negative || facility <= 0 ||
        facility > sizeof(facility_codes)/sizeof(FacilityDesc)) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return NULL;
    }

    faci_desc = (FacilityDesc*)&facility_codes[facility - 1];
    desc_len = strlen(faci_desc->desc);
    if (len < desc_len) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_BUFFER_TOO_SMALL));
        return NULL;
    }

    strcpy(p, faci_desc->desc);
    p += desc_len;
    len -= desc_len;

    if (faci_desc->errstring)
        rc = faci_desc->errstring(errcode, p, len);

    if (rc < 0) {
        hive_set_error(rc);
        return NULL;
    }

    return buf;
}

int hive_register_strerror(int facility, strerror_func_t *strerr)
{
    FacilityDesc *faci_desc;

    if (facility <= 0 || facility > HIVEF_HTTP_STATUS) {
        hive_set_error(HIVE_GENERAL_ERROR(HIVEERR_INVALID_ARGS));
        return -1;
    }

    faci_desc = (FacilityDesc*)&facility_codes[facility - 1];
    if (!faci_desc->errstring)
        faci_desc->errstring = strerr;

    return 0;
}
