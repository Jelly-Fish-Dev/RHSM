/* RHSM – Retro Handheld Save Manager
 *
 *   rhsm          run the on-device app (app/app.c)
 *   rhsm --dump   print the save library to stdout and exit (app/dump.c)
 */

#include <stdio.h>
#include <string.h>

#include "app/app.h"
#include "app/dump.h"
#include "mui/log.h"
#include "platform/paths.h"
#include "version.h"

int main(int argc, char **argv)
{
    LOG_INFO("RHSM %s starting (sdcard %s)", RHSM_VERSION, paths_sdcard());

    if (argc > 1 && strcmp(argv[1], "--dump") == 0)
        return dump_run();
    if (argc > 1) {
        fprintf(stderr, "usage: %s [--dump]\n", argv[0]);
        return 2;
    }
    return app_run();
}
