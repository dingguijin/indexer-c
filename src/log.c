#define _POSIX_C_SOURCE 200809L

#include "botindex/log.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static pthread_mutex_t g_log_lock = PTHREAD_MUTEX_INITIALIZER;
static log_level_t g_log_level = LOG_LEVEL_INFO;

static const char *level_name(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO: return "INFO ";
        case LOG_LEVEL_WARN: return "WARN ";
        case LOG_LEVEL_ERROR: return "ERROR";
        default: return "INFO ";
    }
}

int log_parse_level(const char *s, log_level_t *out) {
    if (s == NULL || out == NULL) {
        return -1;
    }
    if (strcmp(s, "debug") == 0) {
        *out = LOG_LEVEL_DEBUG;
        return 0;
    }
    if (strcmp(s, "info") == 0) {
        *out = LOG_LEVEL_INFO;
        return 0;
    }
    if (strcmp(s, "warn") == 0) {
        *out = LOG_LEVEL_WARN;
        return 0;
    }
    if (strcmp(s, "error") == 0) {
        *out = LOG_LEVEL_ERROR;
        return 0;
    }
    return -1;
}

void log_init_from_env(void) {
    const char *env = getenv("BOTINDEX_LOG");
    log_level_t parsed = LOG_LEVEL_INFO;

    if (env != NULL && log_parse_level(env, &parsed) == 0) {
        g_log_level = parsed;
        return;
    }
    g_log_level = LOG_LEVEL_INFO;
}

bool log_should_log(log_level_t level) {
    return level >= g_log_level;
}

void log_vlog(log_level_t level, const char *module, const char *fmt, va_list ap) {
    struct timespec ts = {0};
    struct tm tm_utc = {0};
    char stamp[40];

    (void)clock_gettime(CLOCK_REALTIME, &ts);
    gmtime_r(&ts.tv_sec, &tm_utc);
    (void)strftime(stamp, sizeof(stamp), "%Y-%m-%dT%H:%M:%S", &tm_utc);

    pthread_mutex_lock(&g_log_lock);
    fprintf(stderr, "%s.%03ldZ %s [%s] ", stamp, ts.tv_nsec / 1000000L, level_name(level), module);
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wformat-nonliteral"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
    vfprintf(stderr, fmt, ap);
#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif
    fputc('\n', stderr);
    pthread_mutex_unlock(&g_log_lock);
}

void log_log(log_level_t level, const char *module, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_vlog(level, module, fmt, ap);
    va_end(ap);
}

