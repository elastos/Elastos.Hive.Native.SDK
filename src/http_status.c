/*
 * Copyright (c) 2019 Elastos Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string.h>

#include "ela_hive.h"
#include "http_status.h"

typedef struct ErrorDesc {
    int errcode;
    const char *errdesc;
} ErrorDesc;

static const ErrorDesc status_codes[] = {
    { HttpStatus_Continue                      , "Continue"                       },
    { HttpStatus_SwitchingProtocols            , "Switching Protocols"            },
    { HttpStatus_Processing                    , "Processing"                     },
    { HttpStatus_EarlyHints                    , "Early Hints"                    },
    { HttpStatus_OK                            , "OK"                             },
    { HttpStatus_Created                       , "Created"                        },
    { HttpStatus_Accepted                      , "Accepted"                       },
    { HttpStatus_NonAuthoritativeInformation   , "Non-Authoritative Information"  },
    { HttpStatus_NoContent                     , "No Content"                     },
    { HttpStatus_ResetContent                  , "Reset Content"                  },
    { HttpStatus_PartialContent                , "Partial Content"                },
    { HttpStatus_MultiStatus                   , "Multi-Status"                   },
    { HttpStatus_AlreadyReported               , "Already Reported"               },
    { HttpStatus_IMUsed                        , "IM Used"                        },
    { HttpStatus_MultipleChoices               , "Multiple Choices"               },
    { HttpStatus_MovedPermanently              , "Moved Permanently"              },
    { HttpStatus_Found                         , "Found"                          },
    { HttpStatus_SeeOther                      , "See Other"                      },
    { HttpStatus_NotModified                   , "Not Modified"                   },
    { HttpStatus_UseProxy                      , "Use Proxy"                      },
    { HttpStatus_TemporaryRedirect             , "Temporary Redirect"             },
    { HttpStatus_PermanentRedirect             , "Permanent Redirect"             },
    { HttpStatus_BadRequest                    , "Bad Request"                    },
    { HttpStatus_Unauthorized                  , "Unauthorized"                   },
    { HttpStatus_PaymentRequired               , "Payment Required"               },
    { HttpStatus_Forbidden                     , "Forbidden"                      },
    { HttpStatus_NotFound                      , "Not Found"                      },
    { HttpStatus_MethodNotAllowed              , "Method Not Allowed"             },
    { HttpStatus_NotAcceptable                 , "Not Acceptable"                 },
    { HttpStatus_ProxyAuthenticationRequired   , "Proxy Authentication Required"  },
    { HttpStatus_RequestTimeout                , "Request Timeout"                },
    { HttpStatus_Conflict                      , "Conflict"                       },
    { HttpStatus_Gone                          , "Gone"                           },
    { HttpStatus_LengthRequired                , "Length Required"                },
    { HttpStatus_PreconditionFailed            , "Precondition Failed"            },
    { HttpStatus_PayloadTooLarge               , "Payload Too Large"              },
    { HttpStatus_URITooLong                    , "URI Too Long"                   },
    { HttpStatus_UnsupportedMediaType          , "Unsupported Media Type"         },
    { HttpStatus_RangeNotSatisfiable           , "Range Not Satisfiable"          },
    { HttpStatus_ExpectationFailed             , "Expectation Failed"             },
    { HttpStatus_ImATeapot                     , "I'm a teapot"                   },
    { HttpStatus_UnprocessableEntity           , "Unprocessable Entity"           },
    { HttpStatus_Locked                        , "Locked"                         },
    { HttpStatus_FailedDependency              , "Failed Dependency"              },
    { HttpStatus_UpgradeRequired               , "Upgrade Required"               },
    { HttpStatus_PreconditionRequired          , "Precondition Required"          },
    { HttpStatus_TooManyRequests               , "Too Many Requests"              },
    { HttpStatus_RequestHeaderFieldsTooLarge   , "Request Header Fields Too Large"},
    { HttpStatus_UnavailableForLegalReasons    , "Unavailable For Legal Reasons"  },
    { HttpStatus_InternalServerError           , "Internal Server Error"          },
    { HttpStatus_NotImplemented                , "Not Implemented"                },
    { HttpStatus_BadGateway                    , "Bad Gateway"                    },
    { HttpStatus_ServiceUnavailable            , "Service Unavailable"            },
    { HttpStatus_GatewayTimeout                , "Gateway Time-out"               },
    { HttpStatus_HTTPVersionNotSupported       , "HTTP Version Not Supported"     },
    { HttpStatus_VariantAlsoNegotiates         , "Variant Also Negotiates"        },
    { HttpStatus_InsufficientStorage           , "Insufficient Storage"           },
    { HttpStatus_LoopDetected                  , "Loop Detected"                  },
    { HttpStatus_NotExtended                   , "Not Extended"                   },
    { HttpStatus_NetworkAuthenticationRequired , "Network Authentication Required"}
};

int http_status_error(int errcode, char *buf, size_t len)
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
