#include "app/dump.h"

#include <stdio.h>

#include "core/library_loader.h"
#include "core/library_print.h"

int dump_run(void)
{
    Library lib = {0};
    char err[160] = "";
    if (!library_load(&lib, err, sizeof err)) {
        fprintf(stderr, "%s\n", err);
        return 1;
    }
    library_print(stdout, &lib);
    library_free(&lib);
    return 0;
}
