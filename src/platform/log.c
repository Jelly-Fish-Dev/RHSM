#include "platform/log.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void log_msg(const char *level, const char *fmt, ...)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    fprintf(stderr, "[%5ld.%03ld] %-5s ", (long)ts.tv_sec, ts.tv_nsec / 1000000L, level);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);
    fflush(stderr);
}
