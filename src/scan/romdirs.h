#ifndef RHSM_SCAN_ROMDIRS_H
#define RHSM_SCAN_ROMDIRS_H

#include <stdbool.h>
#include <stddef.h>

/* Lazily loaded listings of Onion's Roms/<SYSTEM>/ folders, used to decide
 * which system a save belongs to when its core can run several. */

#define ROMDIRS_MAX 64

typedef struct {
    char system[16];
    char **stems; /* ROM names without extension, sorted */
    int count;
} RomDir;

typedef struct {
    const char *roms_dir;
    RomDir dirs[ROMDIRS_MAX];
    int count;
} RomDirs;

void romdirs_init(RomDirs *rd, const char *roms_dir);
void romdirs_free(RomDirs *rd);

/* true if Roms/<system>/ has a file "<stem>.<ext>" or a folder "<stem>" */
bool romdirs_has(RomDirs *rd, const char *system, const char *stem);

/* Picks the system for a save of `stem` made by `core`: the first of the core's
 * systems that has a matching ROM, else the core's first system, else "OTHER". */
void romdirs_resolve_system(RomDirs *rd, const char *core, const char *stem, char *out, size_t n);

#endif
