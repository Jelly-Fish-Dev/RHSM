#ifndef RHSM_SCAN_SCANNER_H
#define RHSM_SCAN_SCANNER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    KIND_SRAM,  /* .srm / .sav battery save */
    KIND_STATE, /* save state (.state, .stateN, .state.auto) */
    KIND_OTHER, /* other per-game files in saves/ (.ldci, .rtc, ...) */
} SaveKind;

#define SLOT_NONE -2 /* not a state */
#define SLOT_AUTO -1 /* .state.auto */

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
 * roms_dir is used to decide which system a save belongs to.
 * Returns 0 on success. */
int scan_saves(const char *profile_dir, const char *roms_dir, ScanList *out);
void scan_list_free(ScanList *list);

const char *save_kind_name(SaveKind kind);

/* Filename parsing (exposed for tests).
 * parse_state_name("Foo (USA).state.auto") -> stem "Foo (USA)", slot SLOT_AUTO
 * Returns false if the name is not a state (thumbnails *.png return false). */
bool parse_state_name(const char *fname, char *stem, size_t stem_n, int *slot);
/* parse_save_name("Foo (USA).srm") -> stem "Foo (USA)", kind KIND_SRAM */
bool parse_save_name(const char *fname, char *stem, size_t stem_n, SaveKind *kind);
/* "Final Fantasy III (USA) (Rev 1)" -> "Final Fantasy III" */
void clean_title(const char *rom_name, char *out, size_t n);

#endif
