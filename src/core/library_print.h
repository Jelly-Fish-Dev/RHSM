#ifndef RHSM_CORE_LIBRARY_PRINT_H
#define RHSM_CORE_LIBRARY_PRINT_H

#include <stdio.h>

#include "model/library.h"

/* Plain-text listing of every game and save file (used by --dump and the
 * debug log). Also reports whether Onion box art exists for each game. */
void library_print(FILE *out, const Library *lib);

#endif
