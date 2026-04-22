#include "botindex/chain.h"

#include <string.h>

typedef struct {
    chain_t chain;
    const char *name;
} chain_entry_t;

static const chain_entry_t CHAIN_ENTRIES[5] = {
    {CHAIN_ETH, "eth"},
    {CHAIN_BSC, "bsc"},
    {CHAIN_POLYGON, "polygon"},
    {CHAIN_BASE, "base"},
    {CHAIN_ARB, "arb"},
};

const char *chain_name(chain_t chain) {
    for (size_t i = 0; i < 5; i++) {
        if (CHAIN_ENTRIES[i].chain == chain) {
            return CHAIN_ENTRIES[i].name;
        }
    }
    return "unknown";
}

int chain_from_name(const char *s, chain_t *out) {
    if (s == NULL || out == NULL) {
        return -1;
    }
    for (size_t i = 0; i < 5; i++) {
        if (strcmp(CHAIN_ENTRIES[i].name, s) == 0) {
            *out = CHAIN_ENTRIES[i].chain;
            return 0;
        }
    }
    return -1;
}

int chain_index(chain_t chain) {
    for (int i = 0; i < 5; i++) {
        if (CHAIN_ENTRIES[i].chain == chain) {
            return i;
        }
    }
    return -1;
}
