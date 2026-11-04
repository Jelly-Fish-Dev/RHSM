#include "scan/scanner.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "platform/log.h"
#include "scan/cores.h"

/* ------------------------------------------------------------ name parsing */

static bool ends_with(const char *s, const char *suffix)
{
    size_t ls = strlen(s), lx = strlen(suffix);
    return ls >= lx && strcmp(s + ls - lx, suffix) == 0;
}

static void copy_n(char *dst, size_t n, const char *src, size_t len)
{
    if (len >= n)
        len = n - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

bool parse_state_name(const char *fname, char *stem, size_t stem_n, int *slot)
{
    if (fname[0] == '.' || ends_with(fname, ".png"))
        return false;

    if (ends_with(fname, ".state.auto")) {
        copy_n(stem, stem_n, fname, strlen(fname) - strlen(".state.auto"));
        *slot = SLOT_AUTO;
        return true;
    }

    /* last ".state" followed by nothing or only digits */
    const char *found = NULL;
    for (const char *p = strstr(fname, ".state"); p; p = strstr(p + 1, ".state"))
        found = p;
    if (!found || found == fname)
        return false;

    const char *num = found + strlen(".state");
    for (const char *d = num; *d; d++) {
        if (!isdigit((unsigned char)*d))
            return false;
    }
    copy_n(stem, stem_n, fname, (size_t)(found - fname));
    *slot = *num ? atoi(num) : 0;
    return true;
}

bool parse_save_name(const char *fname, char *stem, size_t stem_n, SaveKind *kind)
{
    if (fname[0] == '.')
        return false;
    const char *dot = strrchr(fname, '.');
    if (!dot || dot == fname)
        return false;

    const char *ext = dot + 1;
    static const char *ignored[] = {"txt", "db", "png", "json", "cfg", "opt", "bak", NULL};
    for (int i = 0; ignored[i]; i++) {
        if (strcasecmp(ext, ignored[i]) == 0)
            return false;
    }

    copy_n(stem, stem_n, fname, (size_t)(dot - fname));
    *kind = (strcasecmp(ext, "srm") == 0 || strcasecmp(ext, "sav") == 0) ? KIND_SRAM : KIND_OTHER;
    return true;
}

void clean_title(const char *rom_name, char *out, size_t n)
{
    size_t o = 0;
    int depth = 0;
    for (const char *p = rom_name; *p && o + 1 < n; p++) {
        if (*p == '(' || *p == '[') {
            depth++;
        } else if ((*p == ')' || *p == ']') && depth > 0) {
            depth--;
        } else if (depth == 0) {
            out[o++] = *p;
        }
    }
    while (o > 0 && isspace((unsigned char)out[o - 1]))
        o--;
    out[o] = '\0';
    if (o == 0) /* name was all tags: keep it as is */
        snprintf(out, n, "%s", rom_name);
}

const char *save_kind_name(SaveKind kind)
{
    switch (kind) {
    case KIND_SRAM:  return "sram";
    case KIND_STATE: return "state";
    default:         return "other";
    }
}

/* ------------------------------------------- ROM lookup (decides system) */

#define MAX_ROM_DIRS 64

typedef struct {
    char system[16];
    char **stems; /* sorted */
    int count;
} RomDir;

typedef struct {
    const char *roms_dir;
    RomDir dirs[MAX_ROM_DIRS];
    int count;
} RomCache;

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

static RomDir *rom_dir_load(RomCache *cache, const char *system)
{
    for (int i = 0; i < cache->count; i++) {
        if (strcmp(cache->dirs[i].system, system) == 0)
            return &cache->dirs[i];
    }
    if (cache->count == MAX_ROM_DIRS)
        return NULL;

    RomDir *rd = &cache->dirs[cache->count++];
    memset(rd, 0, sizeof *rd);
    snprintf(rd->system, sizeof rd->system, "%s", system);

    char path[512];
    snprintf(path, sizeof path, "%s/%s", cache->roms_dir, system);
    DIR *d = opendir(path);
    if (!d)
        return rd;

    int cap = 0;
    struct dirent *e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.')
            continue;
        bool is_dir = e->d_type == DT_DIR;
        if (e->d_type == DT_UNKNOWN) {
            char full[1024];
            struct stat st;
            snprintf(full, sizeof full, "%s/%s", path, e->d_name);
            is_dir = stat(full, &st) == 0 && S_ISDIR(st.st_mode);
        }
        /* files: strip extension; folders (e.g. multi-disc PS games): keep name */
        char stem[256];
        const char *dot = is_dir ? NULL : strrchr(e->d_name, '.');
        copy_n(stem, sizeof stem, e->d_name, dot ? (size_t)(dot - e->d_name) : strlen(e->d_name));

        if (rd->count == cap) {
            cap = cap ? cap * 2 : 64;
            rd->stems = realloc(rd->stems, (size_t)cap * sizeof *rd->stems);
        }
        rd->stems[rd->count++] = strdup(stem);
    }
    closedir(d);
    qsort(rd->stems, (size_t)rd->count, sizeof *rd->stems, cmp_str);
    return rd;
}

static bool rom_exists(RomCache *cache, const char *system, const char *stem)
{
    RomDir *rd = rom_dir_load(cache, system);
    return rd && rd->count &&
           bsearch(&stem, rd->stems, (size_t)rd->count, sizeof *rd->stems, cmp_str) != NULL;
}

static void rom_cache_free(RomCache *cache)
{
    for (int i = 0; i < cache->count; i++) {
        for (int j = 0; j < cache->dirs[i].count; j++)
            free(cache->dirs[i].stems[j]);
        free(cache->dirs[i].stems);
    }
    cache->count = 0;
}

/* First candidate system that has a ROM with this stem; else the first candidate. */
static void resolve_system(RomCache *cache, const char *core, const char *stem,
                           char *out, size_t n)
{
    const char *list = cores_systems_for(core);
    if (!list) {
        snprintf(out, n, "OTHER");
        return;
    }
    char buf[128], first[16] = "";
    snprintf(buf, sizeof buf, "%s", list);
    for (char *save = NULL, *sys = strtok_r(buf, ",", &save); sys; sys = strtok_r(NULL, ",", &save)) {
        if (!first[0])
            snprintf(first, sizeof first, "%s", sys);
        if (rom_exists(cache, sys, stem)) {
            snprintf(out, n, "%s", sys);
            return;
        }
    }
    snprintf(out, n, "%s", first);
}

/* -------------------------------------------------------------- scanning */

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

static void scan_core_dir(RomCache *cache, const char *dir, const char *core,
                          bool states, ScanList *out)
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

        resolve_system(cache, core, stem, e->system, sizeof e->system);
        clean_title(stem, e->title, sizeof e->title);
        snprintf(e->game_key, sizeof e->game_key, "%s/%s", e->system, stem);
    }
    closedir(d);
}

int scan_saves(const char *profile_dir, const char *roms_dir, ScanList *out)
{
    RomCache cache = {.roms_dir = roms_dir};
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
            scan_core_dir(&cache, dir, de->d_name, s == 1, out);
        }
        closedir(d);
    }

    rom_cache_free(&cache);
    LOG_INFO("scan: %d files", out->count);
    return 0;
}

void scan_list_free(ScanList *list)
{
    free(list->items);
    memset(list, 0, sizeof *list);
}
