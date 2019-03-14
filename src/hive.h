#ifndef __HIVE_H__
#define __HIVE_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <crystal.h>

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

typedef struct hive hive_t;

typedef enum hive_type {
    HIVE_TYPE_ONEDRIVE,
    HIVE_TYPE_NONEXIST
} hive_type_t;

typedef struct hive_options {
    hive_type_t type;
} hive_opt_t;

typedef struct hive_onedrive_options {
    hive_opt_t base;
    char profile_path[PATH_MAX];
    void (*open_oauth_url)(const char *url);
} hive_1drv_opt_t;

typedef int hive_err_t;

HIVE_API
void hive_global_cleanup();

HIVE_API
hive_err_t hive_global_init();

HIVE_API
int hive_authorize(hive_t *hive);
/******************************************************************************
 * Global-wide APIs
 *****************************************************************************/
/**
 * \~English
 * Get the current version of hive sdk.
 */
HIVE_API
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
hive_t *hive_new(const hive_opt_t *options);

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
void hive_kill(hive_t *hive);

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
void hive_free_json(char *json);

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
int hive_stat(hive_t *hive, const char *path, char **result);

/**
 * \~English
 * Update file's timestamp.
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      path        [in] The target file path.
 * @param
 *      timeval     [in] The updated timestamp.
 *
 * @return
 *      0 on success, or -1 if an error occurred. The specific error code
 *      can be retrieved by calling hive_get_error().
 */
HIVE_API
int hive_set_timestamp(hive_t *hive, const char *path, const struct timeval);

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
int hive_list(hive_t *hive, const char *path, char **result);

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
int hive_mkdir(hive_t *hive, const char *path);

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
int hive_move(hive_t *hive, const char *old, const char *new);

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
int hive_copy(hive_t *hive, const char *src, const char *dest_path);

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
int hive_delete(hive_t *hive, const char *path);


//ssize_t hive_read(const char *path, svoid *buf, size_t length);

//ssize_t hive_write(const char *path, const void *buf, size_t length);

/******************************************************************************
 * Hive file system APIs with asynchronous mode.
 *****************************************************************************/
/**
 * \~English
 * Hive callbacks as a response to asynchronous APIs.
 */
typedef struct hive_response_callbacks {
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
    void (*on_success)(hive_t *hive, char *result, void *context);

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
    void (*on_error)(hive_t *hive, int error_code, void *context);
} hive_resp_cbs_t;

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
int hive_async_stat(hive_t *hive, const char *path,
                    hive_resp_cbs_t *callbacks, void *context);

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
 *      timeval     [in] The updated timestamp.
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
int hive_async_set_timestamp(hive_t *hive, const char *path,
                             const struct timeval,
                             hive_resp_cbs_t *callbacks, void *context);

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
int hive_async_list(hive_t *hive, const char *path,
                    hive_resp_cbs_t *callbacks, void *context);

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
int hive_async_mkdir(hive_t *hive, const char *path,
                     hive_resp_cbs_t *callbacks, void *context);

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
int hive_async_move(hive_t *hive, const char *old, const char *new,
                    hive_resp_cbs_t *callbacks, void *context);

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
int hive_async_copy(hive_t *hive, const char *src_path, const char *dest_path,
                    hive_resp_cbs_t *callbacks, void *context);

/**
 * \~English
 * Delete a file or directory
 *
 * This is a asynchronous API with same capability to 'hive_delete'.
 *
 * @param
 *      hive        [in] A handle to the Hive instance.
 * @param
 *      path        [in] The pathname of the target file (or directory)
 *                       to delete
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
int hive_async_delete(hive_t *hive, const char *path,
                      hive_resp_cbs_t *callbacks, void *context);
#ifdef __cplusplus
}
#endif

#if defined(__APPLE__)
#pragma GCC diagnostic pop
#endif

#endif /* __HIVE_H__ */
