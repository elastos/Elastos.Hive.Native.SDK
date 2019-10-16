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

#ifndef __FILE_APIS_CASES_H__
#define __FILE_APIS_CASES_H__

#include "case.h"

DECL_TESTCASE(put_file_test)
DECL_TESTCASE(double_put_file_test)
DECL_TESTCASE(put_file_from_buffer_test)
DECL_TESTCASE(double_put_file_from_buffer_test)
DECL_TESTCASE(get_nonexist_file_length_test)
DECL_TESTCASE(get_nonexist_file_to_buffer_test)
DECL_TESTCASE(get_file_test)
DECL_TESTCASE(get_nonexist_file_test)
DECL_TESTCASE(delete_nonexist_file_test)

#define DEFINE_FILE_APIS_CASES                         \
    DEFINE_TESTCASE(put_file_test),                    \
    DEFINE_TESTCASE(double_put_file_test),             \
    DEFINE_TESTCASE(put_file_from_buffer_test),        \
    DEFINE_TESTCASE(double_put_file_from_buffer_test), \
    DEFINE_TESTCASE(get_nonexist_file_length_test),    \
    DEFINE_TESTCASE(get_nonexist_file_to_buffer_test), \
    DEFINE_TESTCASE(get_file_test),                    \
    DEFINE_TESTCASE(get_nonexist_file_test),           \
    DEFINE_TESTCASE(delete_nonexist_file_test)

#endif /* __FILE_APIS_CASES_H__ */
