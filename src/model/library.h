#ifndef RHSM_MODEL_LIBRARY_H
#define RHSM_MODEL_LIBRARY_H

#include <stddef.h>

/* The save library as the rest of the app sees it: games, each with the save
 * files that belong to it. Filled by db/index.c, shown by ui/, printed by core/. */

typedef enum {
    KIND_SRAM,  /* .srm / .sav battery save */
    KIND_STATE, /* save state (.state, .stateN, .state.auto) */
    KIND_OTHER, /* other per-game files in saves/ (.ldci, .rtc, ...) */
} SaveKind;

#define SLOT_NONE -2 /* not a state */
#define SLOT_AUTO -1 /* .state.auto */

/* "sram" | "state" | "other" (also the value stored in the database) */
const char *save_kind_name(SaveKind kind);

/* One save file. */
typedef struct {
    int id;
    char core[64];
    char kind[8]; /* save_kind_name() */
    int slot;     /* SLOT_AUTO, 0..N for states; SLOT_NONE otherwise */
    char filename[256];
    long long size;
    long long mtime;
    int has_thumb;
} LibFile;

/* A game = all save files that share system + ROM name, across cores. */
typedef struct {
    int id;
    char key[280];
    char system[16];
    char title[160];
    char rom_name[256];
    int sram_count;
    int state_count;
    int file_count;
    long long last_mtime;
    LibFile *files;
} LibGame;

typedef struct {
    char db_path[512];
    int game_count;
    int file_count;
    LibGame *games;
} Library;

void library_free(Library *lib);

/* Short label for a file's type/slot: "SRAM", "FILE", "STATE auto", "STATE 1" */
void libfile_slot_label(const LibFile *f, char *out, size_t n);

#endif
