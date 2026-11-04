#include "scan/filenames.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "util/str.h"

bool parse_state_name(const char *fname, char *stem, size_t stem_n, int *slot)
{
    if (fname[0] == '.' || str_ends_with(fname, ".png"))
        return false;

    if (str_ends_with(fname, ".state.auto")) {
        str_copy_n(stem, stem_n, fname, strlen(fname) - strlen(".state.auto"));
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
    str_copy_n(stem, stem_n, fname, (size_t)(found - fname));
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

    str_copy_n(stem, stem_n, fname, (size_t)(dot - fname));
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
