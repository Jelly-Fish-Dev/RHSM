#include "core/library_loader.h"

#include <stdio.h>
#include <time.h>

#include "core/library_print.h"
#include "db/index.h"
#include "platform/log.h"
#include "platform/paths.h"
#include "scan/scanner.h"

static long elapsed_ms(const struct timespec *t0)
{
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    return (long)((t1.tv_sec - t0->tv_sec) * 1000 + (t1.tv_nsec - t0->tv_nsec) / 1000000);
}

/* Called once the library is in memory. Kept separate and non-inlined so gdb
 * can hook it: scripts/rhsm.gdb prints every game and file from here. */
static __attribute__((noinline)) void on_library_loaded(const Library *lib)
{
    LOG_INFO("library: %d games, %d files", lib->game_count, lib->file_count);
#ifdef DEBUG
    library_print(stderr, lib);
#endif
}

bool library_load(Library *lib, char *err, size_t err_n)
{
    char profile[512], roms[512], data[512], db_path[512];
    paths_sd(profile, sizeof profile, PATH_PROFILE);
    paths_sd(roms, sizeof roms, PATH_ROMS);
    paths_sd(data, sizeof data, PATH_DATA);
    paths_sd(db_path, sizeof db_path, PATH_DB);

    if (paths_mkdirs(data) != 0) {
        snprintf(err, err_n, "Cannot create %s", data);
        return false;
    }

    struct timespec t0;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    ScanList scan = {0};
    scan_saves(profile, roms, &scan);

    sqlite3 *db = NULL;
    if (db_open(db_path, &db) != 0) {
        snprintf(err, err_n, "Cannot open database %s", db_path);
        scan_list_free(&scan);
        return false;
    }
    bool ok = db_store_scan(db, &scan) == 0 && db_load_library(db, lib) == 0;
    db_close(db);
    scan_list_free(&scan);

    if (!ok) {
        snprintf(err, err_n, "Database error (see log.txt)");
        return false;
    }
    LOG_INFO("library_load took %ld ms", elapsed_ms(&t0));
    on_library_loaded(lib);
    return true;
}
