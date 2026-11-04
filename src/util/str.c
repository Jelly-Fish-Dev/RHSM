#include "util/str.h"

#include <string.h>

bool str_ends_with(const char *s, const char *suffix)
{
    size_t ls = strlen(s), lx = strlen(suffix);
    return ls >= lx && strcmp(s + ls - lx, suffix) == 0;
}

void str_copy_n(char *dst, size_t n, const char *src, size_t len)
{
    if (n == 0)
        return;
    if (len >= n)
        len = n - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}
