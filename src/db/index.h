#ifndef RHSM_DB_INDEX_H
#define RHSM_DB_INDEX_H

#include "scan/scanner.h"

/* Schema version stored in PRAGMA user_version. The desktop companion checks
 * the same number and refuses to write a newer schema than it understands. */
#define DB_SCHEMA_VERSION 1

typedef struct sqlite3 sqlite3;

/* One save file as loaded back from the database. */
typedef struct {
    int id;
    char core[64];
    char kind[8]; /* "sram" | "state" | "other" */
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

int db_open(const char *path, sqlite3 **out);
void db_close(sqlite3 *db);

/* Replaces the indexed file list with a fresh scan (games keep their ids). */
int db_store_scan(sqlite3 *db, const ScanList *scan);

/* Loads all games (sorted by title) with their files. */
int db_load_library(sqlite3 *db, Library *lib);
void library_free(Library *lib);

#endif
