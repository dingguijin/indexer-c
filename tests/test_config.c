#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "botindex/config.h"

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__); return 1; } } while (0)

int main(void) {
    config_t *cfg = NULL;
    char ok_path[512];
    char bad_path[512];

    (void)snprintf(ok_path, sizeof(ok_path), "%s/tests/fixtures/indexer.ok.toml", BOTINDEX_SOURCE_DIR);
    (void)snprintf(bad_path, sizeof(bad_path), "%s/tests/fixtures/indexer.bad.toml", BOTINDEX_SOURCE_DIR);

    CHECK(config_load(ok_path, &cfg) == 0);
    CHECK(cfg != NULL);
    CHECK(config_redis_db(cfg, CHAIN_ETH) == 0);
    CHECK(config_redis_db(cfg, CHAIN_ARB) == 4);
    CHECK(config_expected_chain_id(cfg, CHAIN_ETH) == (uint64_t)1);
    CHECK(config_head_poll_ms(cfg, CHAIN_ETH) == 500);
    CHECK(config_rpc_http(cfg, CHAIN_ETH) != NULL);

    config_free(cfg);
    cfg = NULL;

    CHECK(config_load(bad_path, &cfg) != 0);
    CHECK(cfg == NULL);

    return 0;
}
