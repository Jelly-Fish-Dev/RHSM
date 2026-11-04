#ifndef RHSM_UTIL_STR_H
#define RHSM_UTIL_STR_H

#include <stdbool.h>
#include <stddef.h>

/* true if s ends with suffix */
bool str_ends_with(const char *s, const char *suffix);

/* Copies the first len bytes of src into dst (size n), always NUL-terminated. */
void str_copy_n(char *dst, size_t n, const char *src, size_t len);

#endif
