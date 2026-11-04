#include "scan/romdirs.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "scan/cores.h"
#include "util/str.h"

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

void romdirs_init(RomDirs *rd, const char *roms_dir)
{
    memset(rd, 0, sizeof *rd);
    rd->roms_dir = roms_dir;
}

static RomDir *romdir_get(RomDirs *rd, const char *system)
{
    for (int i = 0; i < rd->count; i++) {
        if (strcmp(rd->dirs[i].system, system) == 0)
            return &rd->dirs[i];
    }
    if (rd->count == ROMDIRS_MAX)
        return NULL;

    RomDir *dir = &rd->dirs[rd->count++];
    memset(dir, 0, sizeof *dir);
    snprintf(dir->system, sizeof dir->system, "%s", system);

    char path[512];
    snprintf(path, sizeof path, "%s/%s", rd->roms_dir, system);
    DIR *d = opendir(path);
    if (!d)
        return dir;

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
        str_copy_n(stem, sizeof stem, e->d_name,
                   dot ? (size_t)(dot - e->d_name) : strlen(e->d_name));

        if (dir->count == cap) {
            cap = cap ? cap * 2 : 64;
            dir->stems = realloc(dir->stems, (size_t)cap * sizeof *dir->stems);
        }
        dir->stems[dir->count++] = strdup(stem);
    }
    closedir(d);
    qsort(dir->stems, (size_t)dir->count, sizeof *dir->stems, cmp_str);
    return dir;
}

bool romdirs_has(RomDirs *rd, const char *system, const char *stem)
{
    RomDir *dir = romdir_get(rd, system);
    return dir && dir->count &&
           bsearch(&stem, dir->stems, (size_t)dir->count, sizeof *dir->stems, cmp_str) != NULL;
}

void romdirs_resolve_system(RomDirs *rd, const char *core, const char *stem, char *out, size_t n)
{
    const char *list = cores_systems_for(core);
    if (!list) {
        snprintf(out, n, "OTHER");
        return;
    }
    char buf[128], first[16] = "";
    snprintf(buf, sizeof buf, "%s", list);
    for (char *save = NULL, *sys = strtok_r(buf, ",", &save); sys;
         sys = strtok_r(NULL, ",", &save)) {
        if (!first[0])
            snprintf(first, sizeof first, "%s", sys);
        if (romdirs_has(rd, sys, stem)) {
            snprintf(out, n, "%s", sys);
            return;
        }
    }
    snprintf(out, n, "%s", first);
}

void romdirs_free(RomDirs *rd)
{
    for (int i = 0; i < rd->count; i++) {
        for (int j = 0; j < rd->dirs[i].count; j++)
            free(rd->dirs[i].stems[j]);
        free(rd->dirs[i].stems);
    }
    rd->count = 0;
}
