#ifndef RHSM_DB_INDEX_H
#define RHSM_DB_INDEX_H

#include "model/library.h"
#include "scan/scanner.h"

/* The SQLite save index (Saves/RHSM/index.db): schema, storing a scan, and
 * loading it back as a Library. */

/* Schema version stored in PRAGMA user_version. The desktop companion checks
 * the same number and refuses to write a newer schema than it understands. */
#define DB_SCHEMA_VERSION 1

typedef struct sqlite3 sqlite3;

/* Opens (creating/migrating if needed) the index. Returns 0 on success. */
int db_open(const char *path, sqlite3 **out);
void db_close(sqlite3 *db);

/* Replaces the indexed file list with a fresh scan (games keep their ids). */
int db_store_scan(sqlite3 *db, const ScanList *scan);

/* Loads all games (sorted by title) with their files. Free with library_free(). */
int db_load_library(sqlite3 *db, Library *lib);

#endif
