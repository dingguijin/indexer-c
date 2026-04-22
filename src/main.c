#define _POSIX_C_SOURCE 200809L
#define LOG_MODULE "main"

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "botindex/chain.h"
#include "botindex/config.h"
#include "botindex/head_follower.h"
#include "botindex/log.h"
#include "botindex/redisio.h"
#include "botindex/rpc.h"

static volatile sig_atomic_t g_running = 1;

static void handle_signal(int signo) {
    (void)signo;
    g_running = 0;
}

static int parse_args(int argc, char **argv, chain_t *chain_out, const char **config_out) {
    const char *config = "config/indexer.toml";
    chain_t chain = 0;
    int has_chain = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--chain") == 0) {
            if (i + 1 >= argc || chain_from_name(argv[i + 1], &chain) != 0) {
                fprintf(stderr, "invalid --chain value\n");
                return -1;
            }
            has_chain = 1;
            i++;
        } else if (strcmp(argv[i], "--config") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "missing --config value\n");
                return -1;
            }
            config = argv[i + 1];
            i++;
        } else {
            fprintf(stderr, "unknown arg: %s\n", argv[i]);
            return -1;
        }
    }

    if (!has_chain) {
        fprintf(stderr, "--chain is required\n");
        return -1;
    }

    *chain_out = chain;
    *config_out = config;
    return 0;
}

int main(int argc, char **argv) {
    chain_t chain = 0;
    const char *config_path = NULL;
    config_t *cfg = NULL;
    rpc_client_t *rpc = NULL;
    redisio_t *redis = NULL;
    uint64_t actual_chain_id = 0;
    uint64_t expected_chain_id = 0;
    const char *rpc_http = NULL;
    int head_poll_ms = 0;
    int rc = 1;

    log_init_from_env();

    if (parse_args(argc, argv, &chain, &config_path) != 0) {
        fprintf(stderr, "usage: %s --chain <eth|bsc|polygon|base|arb> [--config <path>]\n", argv[0]);
        return 1;
    }

    if (config_load(config_path, &cfg) != 0) {
        return 1;
    }

    rpc_http = config_rpc_http(cfg, chain);
    head_poll_ms = config_head_poll_ms(cfg, chain);
    expected_chain_id = config_expected_chain_id(cfg, chain);

    if (rpc_http == NULL || rpc_http[0] == '\0' || head_poll_ms <= 0 || expected_chain_id == 0) {
        log_error("missing required chain config chain=%s", chain_name(chain));
        goto done;
    }

    redis = redisio_connect(cfg, chain);
    if (redis == NULL) {
        goto done;
    }

    rpc = rpc_client_new(rpc_http);
    if (rpc == NULL) {
        log_error("failed to initialize rpc client");
        goto done;
    }

    if (rpc_eth_chain_id(rpc, &actual_chain_id) != 0) {
        log_error("eth_chainId probe failed");
        goto done;
    }

    if (actual_chain_id != expected_chain_id) {
        log_error("chain_id mismatch expected=%" PRIu64 " actual=%" PRIu64,
                  expected_chain_id, actual_chain_id);
        goto done;
    }

    log_info("eth_chainId ok chain_id=%" PRIu64, actual_chain_id);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    rc = head_follower_run(chain, head_poll_ms, rpc, redis, &g_running);

done:
    redisio_close(redis);
    rpc_client_free(rpc);
    config_free(cfg);
    return (rc == 0) ? 0 : 1;
}
