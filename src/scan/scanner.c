#include "scan/scanner.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "platform/log.h"
#include "scan/filenames.h"
#include "scan/romdirs.h"

static ScanEntry *list_push(ScanList *list)
{
    if (list->count == list->cap) {
        list->cap = list->cap ? list->cap * 2 : 64;
        list->items = realloc(list->items, (size_t)list->cap * sizeof *list->items);
    }
    ScanEntry *e = &list->items[list->count++];
    memset(e, 0, sizeof *e);
    return e;
}

/* One <saves|states>/<core>/ folder. */
static void scan_core_dir(RomDirs *roms, const char *dir, const char *core, bool states,
                          ScanList *out)
{
    DIR *d = opendir(dir);
    if (!d)
        return;

    struct dirent *de;
    while ((de = readdir(d))) {
        char path[1024];
        snprintf(path, sizeof path, "%s/%s", dir, de->d_name);
        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
            continue;

        char stem[256];
        int slot = SLOT_NONE;
        SaveKind kind = KIND_STATE;
        if (states ? !parse_state_name(de->d_name, stem, sizeof stem, &slot)
                   : !parse_save_name(de->d_name, stem, sizeof stem, &kind))
            continue;

        ScanEntry *e = list_push(out);
        snprintf(e->core, sizeof e->core, "%s", core);
        snprintf(e->rom_name, sizeof e->rom_name, "%s", stem);
        snprintf(e->path, sizeof e->path, "%s", path);
        e->kind = kind;
        e->slot = slot;
        e->size = (long long)st.st_size;
        e->mtime = (long long)st.st_mtime;

        if (states) {
            char thumb[1040];
            struct stat ts;
            snprintf(thumb, sizeof thumb, "%s.png", path);
            e->has_thumb = stat(thumb, &ts) == 0;
        }

        romdirs_resolve_system(roms, core, stem, e->system, sizeof e->system);
        clean_title(stem, e->title, sizeof e->title);
        snprintf(e->game_key, sizeof e->game_key, "%s/%s", e->system, stem);
    }
    closedir(d);
}

int scan_saves(const char *profile_dir, const char *roms_dir, ScanList *out)
{
    RomDirs roms;
    romdirs_init(&roms, roms_dir);
    static const char *subdirs[] = {"saves", "states"};

    for (int s = 0; s < 2; s++) {
        char base[512];
        snprintf(base, sizeof base, "%s/%s", profile_dir, subdirs[s]);
        DIR *d = opendir(base);
        if (!d) {
            LOG_WARN("scan: cannot open %s", base);
            continue;
        }
        struct dirent *de;
        while ((de = readdir(d))) {
            if (de->d_name[0] == '.')
                continue;
            char dir[1024];
            struct stat st;
            snprintf(dir, sizeof dir, "%s/%s", base, de->d_name);
            if (stat(dir, &st) != 0 || !S_ISDIR(st.st_mode))
                continue;
            scan_core_dir(&roms, dir, de->d_name, s == 1, out);
        }
        closedir(d);
    }

    romdirs_free(&roms);
    LOG_INFO("scan: %d files", out->count);
    return 0;
}

void scan_list_free(ScanList *list)
{
    free(list->items);
    memset(list, 0, sizeof *list);
}
