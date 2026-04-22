#include <stdio.h>

#include "botindex/chain.h"

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__); return 1; } } while (0)

int main(void) {
    chain_t chain = 0;

    CHECK(chain_from_name("eth", &chain) == 0);
    CHECK(chain == CHAIN_ETH);
    CHECK(chain_name(chain)[0] == 'e');

    CHECK(chain_from_name("bsc", &chain) == 0);
    CHECK(chain == CHAIN_BSC);

    CHECK(chain_from_name("polygon", &chain) == 0);
    CHECK(chain == CHAIN_POLYGON);

    CHECK(chain_from_name("base", &chain) == 0);
    CHECK(chain == CHAIN_BASE);

    CHECK(chain_from_name("arb", &chain) == 0);
    CHECK(chain == CHAIN_ARB);

    CHECK(chain_from_name("unknown", &chain) != 0);
    CHECK(chain_index(CHAIN_ETH) == 0);
    CHECK(chain_index(CHAIN_ARB) == 4);
    return 0;
}
