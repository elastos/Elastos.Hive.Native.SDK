#ifndef __HIVE_DRIVE_H__
#define __HIVE_DRIVE_H__

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
      #define HIVE_API __declspec(dllexport)
    #else
      #define HIVE_API __attribute__((visibility("default")))
    #endif
  #else
    #if defined(_WIN32) || defined(_WIN64)
      #define HIVE_API __declspec(dllimport)
    #else
      #define HIVE_API
    #endif
  #endif
#else
  #define HIVE_API
#endif

typedef struct HiveDrive HiveDrive;
typedef struct HiveDriveInfo HiveDriveInfo;

HIVE_API
int hive_drive_getinfo(HiveDrive *, HiveDriveInfo *);

HIVE_API
int hive_drive_file_stat(HiveDrive *, const char *file_path, char **result);

HIVE_API
int hive_drive_list_files(HiveDrive *, const char *dir_path, char **result);

HIVE_API
int hive_drive_mkdir(HiveDrive *, const char *path);

HIVE_API
int hive_drive_move_file(HiveDrive *, const char *old, const char *new);

HIVE_API
int hive_drive_copy_file(HiveDrive *, const char *src_path, const char *dest_path);

HIVE_API
int hive_drive_delete_file(HiveDrive *, const char *path);

HIVE_API
void hive_drive_close(HiveDrive *);

#ifdef __cplusplus
} // extern "C"
#endif

#if defined(__APPLE__)
#pragma GCC diagnostic pop
#endif

#endif // __HIVE_DRIVE_H__
