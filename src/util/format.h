#ifndef RHSM_UTIL_FORMAT_H
#define RHSM_UTIL_FORMAT_H

#include <stddef.h>

/* 512 -> "512 B", 8192 -> "8 KB", 1.6e6 -> "1.5 MB" */
void format_size(long long bytes, char *out, size_t n);

/* Unix time -> local "yy-mm-dd hh:mm" */
void format_time(long long t, char *out, size_t n);

#endif
