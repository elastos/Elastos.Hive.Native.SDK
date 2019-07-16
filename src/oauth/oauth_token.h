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

#ifndef __OAUTH_TOKEN_H__
#define __OAUTH_TOKEN_H__

typedef struct oauth_token oauth_token_t;
typedef struct cJSON cJSON;

typedef struct oauth_options {
    const char *authorize_url;
    const char *token_url;

    const char *client_id;
    const char *scope;
    const char *redirect_url;

    const cJSON *store;
} oauth_options_t;

/*
 * The prototype of function to writeback token information.
 */
typedef int oauth_writeback_func_t(const cJSON *json, void *user_data);

/*
 * Create an oauth token instance.
 */
oauth_token_t *oauth_token_new(const oauth_options_t *opts, oauth_writeback_func_t *cb,
                               void *user_data);

/*
 *  Delete oauth token instance.
 */
int oauth_token_delete(oauth_token_t *token);

/*
 * Reset token
 */
int oauth_token_reset(oauth_token_t *token);

/*
 * The prototype of function to request authentication from user.
 */
typedef int oauth_request_func_t(const char *reqeust_url, void *user_data);

/*
 * Request and acquire the refresh and access tokens.
 */
int oauth_token_request(oauth_token_t *token, oauth_request_func_t *cb,
                        void *user_data);

/*
 * Make the access token expired.
 */
void oauth_token_set_expired(oauth_token_t *token);

/*
 * Check the access token expired or not. It the access token expired,
 * then refresh this access token.
 */
int oauth_token_check_expire(oauth_token_t *token);

/*
 * Get bearer type token.
 */
char *get_bearer_token(oauth_token_t *token);

#endif // __OAUTH_TOKEN_H__
