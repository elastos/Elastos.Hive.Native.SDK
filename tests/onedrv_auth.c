#include <hive.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void open_url(const char *url)
{
    char cmd[strlen("open ") + strlen(url) + 3];
    strcpy(cmd, "open ");
    strcat(cmd, "'");
    strcat(cmd, url);
    strcat(cmd, "'");
    system(cmd);
}

int main()
{
    hive_err_t rc;
    hive_1drv_opt_t opt;
    hive_t *hive;
    char *resp;

    getchar();

    rc = hive_global_init();
    if (rc)
        return -1;

    opt.base.type = HIVE_TYPE_ONEDRIVE;
    strcpy(opt.profile_path, "hive1drv.json");
    opt.open_oauth_url = open_url;

    hive = hive_new((hive_opt_t *)&opt);
    if (!hive)
        return -1;

    rc = hive_mkdir(hive, "/test/test");
    if (rc)
        return -1;
    while (1) ;
    return 0;
}