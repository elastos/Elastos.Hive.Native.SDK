#include <crystal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <stdbool.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "drive.h"

int hive_drive_get_info(HiveDrive *drv, char **result)
{
    int rc;

    if (!drv || !result)
        return -1;

    ref(drv);
    rc = drv->get_info(drv, result);
    deref(drv);

    return rc;
}

int hive_drive_file_stat(HiveDrive *drv, const char *file_path, char **result)
{
    int rc;

    if (!drv || !file_path || !result || file_path[0] != '/')
        return -1;

    ref(drv);
    rc = drv->file_stat(drv, file_path, result);
    deref(drv);

    return rc;
}

int hive_drive_list_files(HiveDrive *drv, const char *dir_path, char **result)
{
    int rc;

    if (!drv || !dir_path || dir_path[0] != '/' || !result)
        return -1;

    ref(drv);
    rc = drv->list_files(drv, dir_path, result);
    deref(drv);

    return rc;
}

int hive_drive_mkdir(HiveDrive *drv, const char *path)
{
    int rc;

    if (!drv || !path || path[0] != '/' || path[1] == '\0')
        return -1;

    ref(drv);
    rc = drv->makedir(drv, path);
    deref(drv);

    return rc;
}

int hive_drive_move_file(HiveDrive *drv, const char *old, const char *new)
{
    int rc;

    if (!drv || !old || !new || old[0] != '/' ||
        new[0] == '\0' || strlen(old) > MAXPATHLEN ||
        strlen(new) > MAXPATHLEN || !strcmp(old, new))
        return -1;

    ref(drv);
    rc = drv->move_file(drv, old, new);
    deref(drv);

    return rc;
}

int hive_drive_copy_file(HiveDrive *drv, const char *src_path, const char *dest_path)
{
    int rc;

    if (!drv || !src_path || !dest_path || src_path[0] != '/' ||
        dest_path[0] == '\0' || strlen(src_path) > MAXPATHLEN ||
        strlen(dest_path) > MAXPATHLEN || !strcmp(src_path, dest_path))
        return -1;

    ref(drv);
    rc = drv->copy_file(drv, src_path, dest_path);
    deref(drv);

    return rc;
}

int hive_drive_delete_file(HiveDrive *drv, const char *path)
{
    int rc;

    if (!drv || !path || path[0] != '/' || path[1] == '\0')
        return -1;

    ref(drv);
    rc = drv->delete_file(drv, path);
    deref(drv);

    return rc;
}

void hive_drive_close(HiveDrive *drv)
{
    if (!drv)
        return;

    drv->close(drv);
}
