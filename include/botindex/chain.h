#ifndef BOTINDEX_CHAIN_H
#define BOTINDEX_CHAIN_H

#include <stdint.h>

typedef enum {
    CHAIN_ETH = 1,
    CHAIN_BSC = 56,
    CHAIN_POLYGON = 137,
    CHAIN_BASE = 8453,
    CHAIN_ARB = 42161
} chain_t;

const char *chain_name(chain_t chain);
int chain_from_name(const char *s, chain_t *out);
int chain_index(chain_t chain);

#endif
