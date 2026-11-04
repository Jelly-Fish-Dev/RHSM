#ifndef RHSM_SCAN_FILENAMES_H
#define RHSM_SCAN_FILENAMES_H

#include <stdbool.h>
#include <stddef.h>

#include "model/library.h"

/* Parsing RetroArch save/state filenames. Pure string functions, no I/O. */

/* "Foo (USA).state.auto" -> stem "Foo (USA)", slot SLOT_AUTO
 * "Foo (USA).state1"     -> stem "Foo (USA)", slot 1
 * Returns false if the name is not a state (thumbnails *.png return false). */
bool parse_state_name(const char *fname, char *stem, size_t stem_n, int *slot);

/* "Foo (USA).srm" -> stem "Foo (USA)", kind KIND_SRAM
 * Returns false for hidden files and non-save files (.txt, .db, .png, ...). */
bool parse_save_name(const char *fname, char *stem, size_t stem_n, SaveKind *kind);

/* "Final Fantasy III (USA) (Rev 1)" -> "Final Fantasy III" */
void clean_title(const char *rom_name, char *out, size_t n);

#endif
