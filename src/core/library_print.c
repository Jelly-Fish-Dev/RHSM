#include "core/library_print.h"

#include <sys/stat.h>

#include "platform/paths.h"
#include "util/format.h"

void library_print(FILE *out, const Library *lib)
{
    fprintf(out, "Library: %d games, %d files  (db %s)\n", lib->game_count, lib->file_count,
            lib->db_path);
    for (int i = 0; i < lib->game_count; i++) {
        const LibGame *g = &lib->games[i];
        char art[768];
        struct stat st;
        paths_boxart(art, sizeof art, g->system, g->rom_name);
        fprintf(out, "[%s] %s  -- %d sram, %d states  (%s)  art: %s\n", g->system, g->title,
                g->sram_count, g->state_count, g->rom_name, stat(art, &st) == 0 ? "yes" : "no");

        for (int j = 0; j < g->file_count; j++) {
            const LibFile *f = &g->files[j];
            char slot[24], size[24], when[24];
            libfile_slot_label(f, slot, sizeof slot);
            format_size(f->size, size, sizeof size);
            format_time(f->mtime, when, sizeof when);
            fprintf(out, "    %-11s %-18s %8s  %s%s\n", slot, f->core, size, when,
                    f->has_thumb ? "  [thumb]" : "");
        }
    }
}
