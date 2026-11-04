#ifndef RHSM_APP_DUMP_H
#define RHSM_APP_DUMP_H

/* `rhsm --dump`: scan + index the saves and print the library to stdout,
 * without touching the display (useful over SSH). Returns the exit code. */
int dump_run(void);

#endif
