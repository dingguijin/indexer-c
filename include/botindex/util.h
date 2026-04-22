#ifndef BOTINDEX_UTIL_H
#define BOTINDEX_UTIL_H

#include <stdint.h>

int hex_to_u64(const char *s, uint64_t *out);
void sleep_ms(int ms);

#endif
