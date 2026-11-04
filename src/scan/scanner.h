#ifndef RHSM_SCAN_SCANNER_H
#define RHSM_SCAN_SCANNER_H

#include <stdbool.h>

#include "model/library.h"

/* Walks RetroArch's per-core save and state folders and produces one entry per
 * save file, already tagged with its system, title and game key. */

typedef struct {
    char core[64];      /* save folder name = RetroArch core */
    char system[16];    /* Onion Roms/<SYSTEM> */
    char rom_name[256]; /* filename stem shared with the ROM */
    char title[160];    /* rom_name without (region) [tags] */
    char game_key[280]; /* "<system>/<rom_name>" */
    char path[1024];    /* full path to the file */
    SaveKind kind;
    int slot;
    long long size;
    long long mtime;
    bool has_thumb; /* states: <file>.png exists */
} ScanEntry;

typedef struct {
    ScanEntry *items;
    int count;
    int cap;
} ScanList;

/* Scans <profile_dir>/saves/<core>/ and <profile_dir>/states/<core>/.
 * roms_dir (Onion's Roms/) decides which system each save belongs to.
 * Returns 0 on success. */
int scan_saves(const char *profile_dir, const char *roms_dir, ScanList *out);
void scan_list_free(ScanList *list);

#endif
