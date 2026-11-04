#ifndef RHSM_PLATFORM_PATHS_H
#define RHSM_PLATFORM_PATHS_H

#include <stddef.h>

/* SD card root. Defaults to /mnt/SDCARD; override with the RHSM_SDCARD env var
 * (handy for testing against a copied SD card tree). */
const char *paths_sdcard(void);

/* Joins the SD card root with a relative path: paths_sd(buf, n, "Roms") */
const char *paths_sd(char *buf, size_t n, const char *rel);

/* mkdir -p */
int paths_mkdirs(const char *dir);

#define PATH_PROFILE "Saves/CurrentProfile"
#define PATH_ROMS    "Roms"
#define PATH_DATA    "Saves/RHSM"
#define PATH_DB      "Saves/RHSM/index.db"

#endif
