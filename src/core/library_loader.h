#ifndef RHSM_CORE_LIBRARY_LOADER_H
#define RHSM_CORE_LIBRARY_LOADER_H

#include <stdbool.h>
#include <stddef.h>

#include "model/library.h"

/* The full pipeline: scan the SD card's saves -> store in the SQLite index ->
 * load the index back as a Library. Returns false and fills err on failure.
 * On success the caller owns lib (free with library_free). */
bool library_load(Library *lib, char *err, size_t err_n);

#endif
