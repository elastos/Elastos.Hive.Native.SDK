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

#ifndef __HTTP_STATUS_H__
#define __HTTP_STATUS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define HttpStatus_Continue                          100
#define HttpStatus_SwitchingProtocols                101
#define HttpStatus_Processing                        102
#define HttpStatus_EarlyHints                        103
#define HttpStatus_OK                                200
#define HttpStatus_Created                           201
#define HttpStatus_Accepted                          202
#define HttpStatus_NonAuthoritativeInformation       203
#define HttpStatus_NoContent                         204
#define HttpStatus_ResetContent                      205
#define HttpStatus_PartialContent                    206
#define HttpStatus_MultiStatus                       207
#define HttpStatus_AlreadyReported                   208
#define HttpStatus_IMUsed                            226
#define HttpStatus_MultipleChoices                   300
#define HttpStatus_MovedPermanently                  301
#define HttpStatus_Found                             302
#define HttpStatus_SeeOther                          303
#define HttpStatus_NotModified                       304
#define HttpStatus_UseProxy                          305
#define HttpStatus_TemporaryRedirect                 307
#define HttpStatus_PermanentRedirect                 308
#define HttpStatus_BadRequest                        400
#define HttpStatus_Unauthorized                      401
#define HttpStatus_PaymentRequired                   402
#define HttpStatus_Forbidden                         403
#define HttpStatus_NotFound                          404
#define HttpStatus_MethodNotAllowed                  405
#define HttpStatus_NotAcceptable                     406
#define HttpStatus_ProxyAuthenticationRequired       407
#define HttpStatus_RequestTimeout                    408
#define HttpStatus_Conflict                          409
#define HttpStatus_Gone                              410
#define HttpStatus_LengthRequired                    411
#define HttpStatus_PreconditionFailed                412
#define HttpStatus_PayloadTooLarge                   413
#define HttpStatus_URITooLong                        414
#define HttpStatus_UnsupportedMediaType              415
#define HttpStatus_RangeNotSatisfiable               416
#define HttpStatus_ExpectationFailed                 417
#define HttpStatus_ImATeapot                         418
#define HttpStatus_UnprocessableEntity               422
#define HttpStatus_Locked                            423
#define HttpStatus_FailedDependency                  424
#define HttpStatus_UpgradeRequired                   426
#define HttpStatus_PreconditionRequired              428
#define HttpStatus_TooManyRequests                   429
#define HttpStatus_RequestHeaderFieldsTooLarge       431
#define HttpStatus_UnavailableForLegalReasons        451
#define HttpStatus_InternalServerError               500
#define HttpStatus_NotImplemented                    501
#define HttpStatus_BadGateway                        502
#define HttpStatus_ServiceUnavailable                503
#define HttpStatus_GatewayTimeout                    504
#define HttpStatus_HTTPVersionNotSupported           505
#define HttpStatus_VariantAlsoNegotiates             506
#define HttpStatus_InsufficientStorage               507
#define HttpStatus_LoopDetected                      508
#define HttpStatus_NotExtended                       510
#define HttpStatus_NetworkAuthenticationRequired     511

int http_status_error(int errcode, char *buf, size_t len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __HTTP_STATUS_H__
