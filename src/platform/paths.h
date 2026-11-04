#ifndef RHSM_PLATFORM_PATHS_H
#define RHSM_PLATFORM_PATHS_H

#include <stddef.h>

/* Where things live on an OnionOS SD card. */

/* SD card root. Defaults to /mnt/SDCARD; override with the RHSM_SDCARD env var
 * (handy for testing against a copied SD card tree). */
const char *paths_sdcard(void);

/* Joins the SD card root with a relative path: paths_sd(buf, n, "Roms") */
const char *paths_sd(char *buf, size_t n, const char *rel);

/* Onion's box art for a ROM: <sdcard>/Roms/<system>/Imgs/<rom_name>.png */
const char *paths_boxart(char *buf, size_t n, const char *system, const char *rom_name);

/* mkdir -p. Returns 0 on success. */
int paths_mkdirs(const char *dir);

#define PATH_PROFILE "Saves/CurrentProfile"
#define PATH_ROMS    "Roms"
#define PATH_DATA    "Saves/RHSM"
#define PATH_DB      "Saves/RHSM/index.db"

#endif
