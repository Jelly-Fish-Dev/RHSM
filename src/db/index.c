#include "db/index.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/log.h"
#include "sqlite3.h"

#define DB_CHECK(rc, db, what)                                              \
    do {                                                                    \
        if ((rc) != SQLITE_OK && (rc) != SQLITE_DONE && (rc) != SQLITE_ROW) { \
            LOG_ERROR("db: %s: %s", what, sqlite3_errmsg(db));              \
            goto fail;                                                      \
        }                                                                   \
    } while (0)

static const char *SCHEMA_V1 =
    "CREATE TABLE IF NOT EXISTS games ("
    "  id        INTEGER PRIMARY KEY,"
    "  game_key  TEXT NOT NULL UNIQUE,"   /* <system>/<rom_name> */
    "  system    TEXT NOT NULL,"
    "  rom_name  TEXT NOT NULL,"
    "  title     TEXT NOT NULL"
    ");"
    "CREATE TABLE IF NOT EXISTS files ("
    "  id        INTEGER PRIMARY KEY,"
    "  game_id   INTEGER NOT NULL REFERENCES games(id) ON DELETE CASCADE,"
    "  path      TEXT NOT NULL UNIQUE,"
    "  core      TEXT NOT NULL,"
    "  kind      TEXT NOT NULL,"          /* sram | state | other */
    "  slot      INTEGER NOT NULL,"       /* -1 auto, 0..N state slot, -2 n/a */
    "  size      INTEGER NOT NULL,"
    "  mtime     INTEGER NOT NULL,"
    "  has_thumb INTEGER NOT NULL DEFAULT 0,"
    "  seen      INTEGER NOT NULL DEFAULT 1"
    ");"
    "CREATE INDEX IF NOT EXISTS files_game ON files(game_id);";

static int exec(sqlite3 *db, const char *sql)
{
    char *err = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        LOG_ERROR("db: %s", err ? err : sqlite3_errmsg(db));
        sqlite3_free(err);
    }
    return rc;
}

int db_open(const char *path, sqlite3 **out)
{
    sqlite3 *db = NULL;
    int rc = sqlite3_open(path, &db);
    if (rc != SQLITE_OK) {
        LOG_ERROR("db: open %s: %s", path, db ? sqlite3_errmsg(db) : "out of memory");
        sqlite3_close(db);
        return -1;
    }
    exec(db, "PRAGMA foreign_keys = ON; PRAGMA synchronous = NORMAL;");

    sqlite3_stmt *st = NULL;
    int version = 0;
    if (sqlite3_prepare_v2(db, "PRAGMA user_version", -1, &st, NULL) == SQLITE_OK &&
        sqlite3_step(st) == SQLITE_ROW)
        version = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);

    if (version > DB_SCHEMA_VERSION) {
        LOG_ERROR("db: schema v%d is newer than this app (v%d); update RHSM", version,
                  DB_SCHEMA_VERSION);
        sqlite3_close(db);
        return -1;
    }
    if (version < DB_SCHEMA_VERSION) {
        if (exec(db, SCHEMA_V1) != SQLITE_OK || exec(db, "PRAGMA user_version = 1") != SQLITE_OK) {
            sqlite3_close(db);
            return -1;
        }
        LOG_INFO("db: created schema v%d at %s", DB_SCHEMA_VERSION, path);
    }

    *out = db;
    return 0;
}

void db_close(sqlite3 *db)
{
    sqlite3_close(db);
}

int db_store_scan(sqlite3 *db, const ScanList *scan)
{
    sqlite3_stmt *game_st = NULL, *file_st = NULL;
    int rc;

    if (exec(db, "BEGIN; UPDATE files SET seen = 0;") != SQLITE_OK)
        return -1;

    rc = sqlite3_prepare_v2(db,
        "INSERT INTO games (game_key, system, rom_name, title) VALUES (?1, ?2, ?3, ?4) "
        "ON CONFLICT(game_key) DO UPDATE SET title = excluded.title "
        "RETURNING id", -1, &game_st, NULL);
    DB_CHECK(rc, db, "prepare games");

    rc = sqlite3_prepare_v2(db,
        "INSERT INTO files (game_id, path, core, kind, slot, size, mtime, has_thumb, seen) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, 1) "
        "ON CONFLICT(path) DO UPDATE SET game_id = excluded.game_id, core = excluded.core, "
        "kind = excluded.kind, slot = excluded.slot, size = excluded.size, "
        "mtime = excluded.mtime, has_thumb = excluded.has_thumb, seen = 1",
        -1, &file_st, NULL);
    DB_CHECK(rc, db, "prepare files");

    for (int i = 0; i < scan->count; i++) {
        const ScanEntry *e = &scan->items[i];

        sqlite3_bind_text(game_st, 1, e->game_key, -1, SQLITE_STATIC);
        sqlite3_bind_text(game_st, 2, e->system, -1, SQLITE_STATIC);
        sqlite3_bind_text(game_st, 3, e->rom_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(game_st, 4, e->title, -1, SQLITE_STATIC);
        rc = sqlite3_step(game_st);
        DB_CHECK(rc, db, "upsert game");
        sqlite3_int64 game_id = sqlite3_column_int64(game_st, 0);
        sqlite3_reset(game_st);

        sqlite3_bind_int64(file_st, 1, game_id);
        sqlite3_bind_text(file_st, 2, e->path, -1, SQLITE_STATIC);
        sqlite3_bind_text(file_st, 3, e->core, -1, SQLITE_STATIC);
        sqlite3_bind_text(file_st, 4, save_kind_name(e->kind), -1, SQLITE_STATIC);
        sqlite3_bind_int(file_st, 5, e->slot);
        sqlite3_bind_int64(file_st, 6, e->size);
        sqlite3_bind_int64(file_st, 7, e->mtime);
        sqlite3_bind_int(file_st, 8, e->has_thumb);
        rc = sqlite3_step(file_st);
        DB_CHECK(rc, db, "upsert file");
        sqlite3_reset(file_st);
    }

    sqlite3_finalize(game_st);
    sqlite3_finalize(file_st);

    if (exec(db, "DELETE FROM files WHERE seen = 0;"
                 "DELETE FROM games WHERE id NOT IN (SELECT game_id FROM files);"
                 "COMMIT;") != SQLITE_OK)
        goto fail_nofinal;
    return 0;

fail:
    sqlite3_finalize(game_st);
    sqlite3_finalize(file_st);
fail_nofinal:
    exec(db, "ROLLBACK");
    return -1;
}

int db_load_library(sqlite3 *db, Library *lib)
{
    sqlite3_stmt *st = NULL;
    int rc;
    const char *path = sqlite3_db_filename(db, "main");

    memset(lib, 0, sizeof *lib);
    snprintf(lib->db_path, sizeof lib->db_path, "%s", path ? path : "");

    /* Games with counts. Order must match the files query below. */
    rc = sqlite3_prepare_v2(db,
        "SELECT g.id, g.game_key, g.system, g.title, g.rom_name, "
        "  SUM(f.kind = 'sram'), SUM(f.kind = 'state'), COUNT(f.id), MAX(f.mtime) "
        "FROM games g JOIN files f ON f.game_id = g.id "
        "GROUP BY g.id ORDER BY g.title COLLATE NOCASE, g.id", -1, &st, NULL);
    DB_CHECK(rc, db, "prepare games");

    int cap = 0;
    while ((rc = sqlite3_step(st)) == SQLITE_ROW) {
        if (lib->game_count == cap) {
            cap = cap ? cap * 2 : 32;
            lib->games = realloc(lib->games, (size_t)cap * sizeof *lib->games);
        }
        LibGame *g = &lib->games[lib->game_count++];
        memset(g, 0, sizeof *g);
        g->id = sqlite3_column_int(st, 0);
        snprintf(g->key, sizeof g->key, "%s", (const char *)sqlite3_column_text(st, 1));
        snprintf(g->system, sizeof g->system, "%s", (const char *)sqlite3_column_text(st, 2));
        snprintf(g->title, sizeof g->title, "%s", (const char *)sqlite3_column_text(st, 3));
        snprintf(g->rom_name, sizeof g->rom_name, "%s", (const char *)sqlite3_column_text(st, 4));
        g->sram_count = sqlite3_column_int(st, 5);
        g->state_count = sqlite3_column_int(st, 6);
        g->file_count = sqlite3_column_int(st, 7);
        g->last_mtime = sqlite3_column_int64(st, 8);
        g->files = calloc((size_t)g->file_count, sizeof *g->files);
    }
    DB_CHECK(rc, db, "read games");
    sqlite3_finalize(st);
    st = NULL;

    /* Files, in the same game order: sram first, then states (auto, 0, 1, ...) */
    rc = sqlite3_prepare_v2(db,
        "SELECT f.game_id, f.id, f.core, f.kind, f.slot, f.path, f.size, f.mtime, f.has_thumb "
        "FROM files f JOIN games g ON g.id = f.game_id "
        "ORDER BY g.title COLLATE NOCASE, g.id, "
        "  CASE f.kind WHEN 'sram' THEN 0 WHEN 'state' THEN 1 ELSE 2 END, f.slot, f.core",
        -1, &st, NULL);
    DB_CHECK(rc, db, "prepare files");

    int gi = 0, fi = 0;
    while ((rc = sqlite3_step(st)) == SQLITE_ROW) {
        int game_id = sqlite3_column_int(st, 0);
        while (gi < lib->game_count && lib->games[gi].id != game_id) {
            gi++;
            fi = 0;
        }
        if (gi == lib->game_count || fi >= lib->games[gi].file_count)
            break; /* should not happen: orders match */

        LibFile *f = &lib->games[gi].files[fi++];
        f->id = sqlite3_column_int(st, 1);
        snprintf(f->core, sizeof f->core, "%s", (const char *)sqlite3_column_text(st, 2));
        snprintf(f->kind, sizeof f->kind, "%s", (const char *)sqlite3_column_text(st, 3));
        f->slot = sqlite3_column_int(st, 4);
        const char *p = (const char *)sqlite3_column_text(st, 5);
        const char *slash = strrchr(p, '/');
        snprintf(f->filename, sizeof f->filename, "%s", slash ? slash + 1 : p);
        f->size = sqlite3_column_int64(st, 6);
        f->mtime = sqlite3_column_int64(st, 7);
        f->has_thumb = sqlite3_column_int(st, 8);
        lib->file_count++;
    }
    sqlite3_finalize(st);

    LOG_INFO("db: loaded %d games, %d files", lib->game_count, lib->file_count);
    return 0;

fail:
    sqlite3_finalize(st);
    library_free(lib);
    return -1;
}

void library_free(Library *lib)
{
    for (int i = 0; i < lib->game_count; i++)
        free(lib->games[i].files);
    free(lib->games);
    lib->games = NULL;
    lib->game_count = lib->file_count = 0;
}
