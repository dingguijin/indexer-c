#define _POSIX_C_SOURCE 200809L

#include "botindex/util.h"

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

int hex_to_u64(const char *s, uint64_t *out) {
    char *end = NULL;
    unsigned long long value = 0;

    if (s == NULL || out == NULL) {
        return -1;
    }
    if (s[0] != '0' || s[1] != 'x') {
        return -1;
    }

    errno = 0;
    value = strtoull(s + 2, &end, 16);
    if (errno != 0 || end == s + 2 || *end != '\0') {
        return -1;
    }

    *out = (uint64_t)value;
    return 0;
}

void sleep_ms(int ms) {
    struct timespec req = {0};
    struct timespec rem = {0};

    if (ms <= 0) {
        return;
    }

    req.tv_sec = ms / 1000;
    req.tv_nsec = (long)(ms % 1000) * 1000000L;

    while (nanosleep(&req, &rem) != 0 && errno == EINTR) {
        req = rem;
    }
}
