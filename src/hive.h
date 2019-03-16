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

#ifndef __ELA_HIVE_H__
#define __ELA_HIVE_H__

#if defined(__APPLE__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdocumentation"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(HIVE_STATIC)
  #define HIVE_API
#elif defined(HIVE_DYNAMIC)
  #ifdef HIVE_BUILD
    #if defined(_WIN32) || defined(_WIN64)
      #define HIVE_API        __declspec(dllexport)
    #else
      #define HIVE_API        __attribute__((visibility("default")))
    #endif
  #else
    #if defined(_WIN32) || defined(_WIN64)
      #define HIVE_API        __declspec(dllimport)
    #else
      #define HIVE_API        __attribute__((visibility("default")))
    #endif
  #endif
#else
  #define HIVE_API
#endif

#include <sys/time.h>

typedef struct cjson_t cjson_t;

typedef struct Hive Hive;

typedef struct HiveOptions HiveOptions;

/******************************************************************************
 * Global-wide APIs
 *****************************************************************************/
/**
 * \~English
 * Get the current version of hive sdk.
 */
const char *hive_get_version(void);

/******************************************************************************
 * Hive instance to create and kill.
 *****************************************************************************/
/**
 * \~English
 * Create a new hive instance.
 *
 * @param
 *      options     [in] The pointer to a valid HiveOptions structure
 *
 * @return
 *      If no error occurs, return the pointer of Hive instance.
 *      Otherwise, return NULL, and a specific error code can be
 *      retrieved by calling hive_get_error().
 */
HIVE_API
Hive *hive_new(const HiveOptions *options);

/**
 * \~English
 * Disconnect from hive network,and destroy all associated resources
 * with the Hive instance.
 *
 * After calling the function, the Hive pointer becomes invalid.
 * No other functions can be called.
 *
 * @param
 *      hive     [in] A handle identifying the Hive instance to kill
 */
HIVE_API
void hive_kill(Hive *hive);

/******************************************************************************
 * Hive file system APIs with synchronous mode.
 *****************************************************************************/
/**
 * \~English
 * Free the memory resource of json context.
 *
 * This API must be called after handling the json context retrieved from
 * some synchronous hive APIs as out parameter. Otherwise, there would be
 * memory leakage happening to applications.
 *
 * @param
 *     json         [in] The memory to json context.
 */
HIVE_API
void hive_free_json(cjson_t *json);

/**
 * \~English
 * Display file status.
 *
 * Application should specifically call hive_free_json() to free the
 * memory pointed by param 'result' after processing it.
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      path        [in] The target file path.
 * @param
 *      result      [out] The file information in json format.
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_stat(Hive *hive, const char *path, cjson_t **result);

/**
 * \~English
 * Update file's timestamp.
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      path        [in] The target file path.
 * @param
 *      new_timestamp   [in] The updated timestamp.
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_set_timestamp(Hive *hive, const char *path,
                       const struct timeval *new_timestamp);

/**
 * \~English
 * List all contents under the directory.
 *
 * Application should specifically call hive_free_json() to free the
 * memory pointed by param 'result' after processing it.
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      path        [in] The target file path
 * @param
 *      result      [out] The list of subdirectories and files in json format.
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_list(Hive *hive, const char *path, cjson_t **result);

/**
 * \~English
 * make a new directory.
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      path        [in] The directory name to be creaetd
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_mkdir(Hive *hive, const char *path);

/**
 * \~English
 * Change the name of the file or directory
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      old         [in] The pathname of the target file (or directory)
 * @param
 *      new         [in] The pathname to be renamed.
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_move(Hive *hive, const char *old, const char *new);

/**
 * \~English
 * Copy file
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      src_path    [in] The pathname of the target file (or directory)
 * @param
 *      dest_path   [in] The pathname to the file (or directory) would be
 *                       copied to
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_copy(Hive *hive, const char *src, const char *dest_path);

/**
 * \~English
 * Delete a file or directory
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      spath       [in] The pathname of the target file (or directory)
 *
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_delete(Hive *hive, const char *path);


//ssize_t hive_read(const char *path, svoid *buf, size_t length);

//ssize_t hive_write(const char *path, const void *buf, size_t length);

/******************************************************************************
 * Hive file system APIs with asynchronous mode.
 *****************************************************************************/
/**
 * \~English
 * Hive callbacks as a response to asynchronous APIs.
 */
typedef struct HiveResponseCallbacks {
    /**
     * \~English
     * Callback will be called when received a confirmative response with
     * extra information if possible.
     *
     * @param
     *      hive        [in] A handle to the Hive instance.
     * @param
     *      result      [in] The response information in json format or NULL
     *                       if confirmative response with no extra infos.
     * @param
     *      context     [in] The application defined context data.
     */
    void (*on_success)(Hive *hive, cjson_t *result, void *context);

    /**
     * \~English
     * Callback will be called when received a response with error information.
     *
     * @param
     *      hive        [in] A handle to the Hive instance.
     * @param
     *      json        [in] The response information in json format or NULL
     *                       if confirmative response with no extra infos.
     * @param
     *      context     [in] The application defined context data.
     */
    void (*on_error)(Hive *hive, int error_code, void *context);
} HiveResponseCallbacks;

/**
 * \~English
 * Display file status.
 *
 * This is a asynchronous API with same capability to 'hive_stat'.
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      path        [in] The target file path
 * @param
 *      callbacks   [in] The callbacks to handle the response
 * @param
 *      context     [in] The application defined context data.
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_async_stat(Hive *hive, const char *path,
                    HiveResponseCallbacks *callbacks, void *context);

/**
 * \~English
 * Update file's timestamp.
 *
 * This is a asynchronous API with same capability to 'hive_set_timestamp'.
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      path        [in] The target file path.
 * @param
 *      new_timestamp   [in] The updated timestamp.
 * @param
 *      callbacks   [in] The callbacks to handle the response
 * @param
 *      context     [in] The application defined context data
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_async_set_timestamp(Hive *hive, const char *path,
                             const struct timeval *new_timestamp,
                             HiveResponseCallbacks *callbacks, void *context);

/**
 * \~English
 * List all contents under the directory.
 *
 * This is a asynchronous API with same capability to 'hive_list'.
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      path        [in] The target file path
 * @param
 *      callbacks   [in] The callbacks to handle the response
 * @param
 *      context     [in] The application defined context data
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_async_list(Hive *hive, const char *path,
                    HiveResponseCallbacks *callbacks, void *context);

/**
 * \~English
 * make a new directory
 *
 * This is a asynchronous API with same capability to 'hive_mkdir'.
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      path        [in] The directory name to be created
 * @param
 *      callbacks   [in] The callbacks to handle the response
 * @param
 *      context     [in] The application defined context data
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_async_mkdir(Hive *hive, const char *path,
                     HiveResponseCallbacks *callbacks, void *context);

/**
 * \~English
 * Change the name of the file or directory
 *
 * This is a asynchronous API with same capability to 'hive_move'.
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      old         [in] The pathname of the target file (or directory)
 * @param
 *      new         [in] The pathname to be renamed.
 * @param
 *      callbacks   [in] The callbacks to handle the response
 * @param
 *      context     [in] The application defined context data
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_async_move(Hive *hive, const char *old, const char *new,
                    HiveResponseCallbacks *callbacks, void *context);

/**
 * \~English
 * Copy file.
 *
 * This is a asynchronous API with same capability to 'hive_copy'.
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      src_path    [in] The source file name
 * @param
 *      dest_path   [in] The dest file name would be copied to
 * @param
 *      callbacks   [in] The callbacks to handle the response
 * @param
 *      context     [in] The application defined context data
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_async_copy(Hive *hive, const char *src_path, const char *dest_path,
                    HiveResponseCallbacks *callbacks, void *context);

/**
 * \~English
 * Delete a file or directory
 *
 * This is a asynchronous API with same capability to 'hive_delete'.
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      path       [in] The pathname of the target file (or directory)
 *                      to delete
 * @param
 *      callbacks   [in] The callbacks to handle the response
 * @param
 *      context     [in] The application defined context data
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_async_delete(Hive *hive, const char *path,
                      HiveResponseCallbacks *callbacks, void *context);

#endif /* __ELA_HIVE_H__ */
