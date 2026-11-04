#include "model/library.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *save_kind_name(SaveKind kind)
{
    switch (kind) {
    case KIND_SRAM:  return "sram";
    case KIND_STATE: return "state";
    default:         return "other";
    }
}

void library_free(Library *lib)
{
    for (int i = 0; i < lib->game_count; i++)
        free(lib->games[i].files);
    free(lib->games);
    lib->games = NULL;
    lib->game_count = lib->file_count = 0;
}

void libfile_slot_label(const LibFile *f, char *out, size_t n)
{
    if (strcmp(f->kind, "state") != 0)
        snprintf(out, n, "%s", strcmp(f->kind, "sram") == 0 ? "SRAM" : "FILE");
    else if (f->slot == SLOT_AUTO)
        snprintf(out, n, "STATE auto");
    else
        snprintf(out, n, "STATE %d", f->slot);
}
