#include <stdint.h>
#include <stdio.h>

#include "botindex/log.h"
#include "botindex/util.h"

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__); return 1; } } while (0)

int main(void) {
    uint64_t out = 0;
    log_level_t lvl = LOG_LEVEL_INFO;

    CHECK(hex_to_u64("0x17c71e9", &out) == 0);
    CHECK(out == (uint64_t)24932841);

    CHECK(hex_to_u64("17c9a29", &out) != 0);
    CHECK(hex_to_u64("0xz", &out) != 0);

    CHECK(log_parse_level("debug", &lvl) == 0);
    CHECK(lvl == LOG_LEVEL_DEBUG);
    CHECK(log_parse_level("info", &lvl) == 0);
    CHECK(lvl == LOG_LEVEL_INFO);
    CHECK(log_parse_level("warn", &lvl) == 0);
    CHECK(lvl == LOG_LEVEL_WARN);
    CHECK(log_parse_level("error", &lvl) == 0);
    CHECK(lvl == LOG_LEVEL_ERROR);
    CHECK(log_parse_level("nope", &lvl) != 0);

    return 0;
}
