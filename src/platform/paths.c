#include "platform/paths.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const char *paths_sdcard(void)
{
    const char *env = getenv("RHSM_SDCARD");
    return (env && env[0]) ? env : "/mnt/SDCARD";
}

const char *paths_sd(char *buf, size_t n, const char *rel)
{
    snprintf(buf, n, "%s/%s", paths_sdcard(), rel);
    return buf;
}

const char *paths_boxart(char *buf, size_t n, const char *system, const char *rom_name)
{
    snprintf(buf, n, "%s/%s/%s/Imgs/%s.png", paths_sdcard(), PATH_ROMS, system, rom_name);
    return buf;
}

int paths_mkdirs(const char *dir)
{
    char tmp[512];
    snprintf(tmp, sizeof tmp, "%s", dir);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0777) != 0 && errno != EEXIST)
                return -1;
            *p = '/';
        }
    }
    if (mkdir(tmp, 0777) != 0 && errno != EEXIST)
        return -1;
    return 0;
}
