#ifndef RHSM_PLATFORM_LOG_H
#define RHSM_PLATFORM_LOG_H

/* Logs go to stderr. launch.sh redirects stderr to log.txt in the app folder,
 * so `make log` can pull them off the device. */
void log_msg(const char *level, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

#define LOG_INFO(...)  log_msg("INFO", __VA_ARGS__)
#define LOG_WARN(...)  log_msg("WARN", __VA_ARGS__)
#define LOG_ERROR(...) log_msg("ERROR", __VA_ARGS__)

/* Debug messages are compiled in only for debug builds (make debug / DEBUG=1). */
#ifdef DEBUG
#define LOG_DEBUG(...) log_msg("DEBUG", __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#endif
