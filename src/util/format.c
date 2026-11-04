#include "util/format.h"

#include <stdio.h>
#include <time.h>

void format_size(long long bytes, char *out, size_t n)
{
    if (bytes < 1024)
        snprintf(out, n, "%lld B", bytes);
    else if (bytes < 1024 * 1024)
        snprintf(out, n, "%lld KB", (bytes + 1023) / 1024);
    else
        snprintf(out, n, "%.1f MB", (double)bytes / (1024.0 * 1024.0));
}

void format_time(long long t, char *out, size_t n)
{
    time_t tt = (time_t)t;
    struct tm tm;
    localtime_r(&tt, &tm);
    strftime(out, n, "%y-%m-%d %H:%M", &tm);
}
