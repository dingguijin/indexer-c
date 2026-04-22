#ifndef BOTINDEX_LOG_H
#define BOTINDEX_LOG_H

#include <stdbool.h>
#include <stdarg.h>

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3
} log_level_t;

void log_init_from_env(void);
int log_parse_level(const char *s, log_level_t *out);
bool log_should_log(log_level_t level);
void log_log(log_level_t level, const char *module, const char *fmt, ...);
void log_vlog(log_level_t level, const char *module, const char *fmt, va_list ap);

#ifndef LOG_MODULE
#define LOG_MODULE "app"
#endif

#define log_debug(...) do { if (log_should_log(LOG_LEVEL_DEBUG)) log_log(LOG_LEVEL_DEBUG, LOG_MODULE, __VA_ARGS__); } while (0)
#define log_info(...) do { if (log_should_log(LOG_LEVEL_INFO)) log_log(LOG_LEVEL_INFO, LOG_MODULE, __VA_ARGS__); } while (0)
#define log_warn(...) do { if (log_should_log(LOG_LEVEL_WARN)) log_log(LOG_LEVEL_WARN, LOG_MODULE, __VA_ARGS__); } while (0)
#define log_error(...) do { if (log_should_log(LOG_LEVEL_ERROR)) log_log(LOG_LEVEL_ERROR, LOG_MODULE, __VA_ARGS__); } while (0)

#endif
