#define LOG_MODULE "head_follower"

#include "botindex/head_follower.h"

#include <inttypes.h>

#include "botindex/chain.h"
#include "botindex/log.h"
#include "botindex/util.h"

int head_follower_run(chain_t chain, int head_poll_ms, rpc_client_t *rpc, redisio_t *redis,
                      volatile sig_atomic_t *running) {
    uint64_t last = 0;

    if (rpc == NULL || redis == NULL || running == NULL) {
        return -1;
    }

    while (*running) {
        uint64_t head = 0;
        char hash[67] = {0};
        uint64_t ts = 0;

        if (rpc_eth_block_number(rpc, &head) != 0) {
            sleep_ms(head_poll_ms);
            continue;
        }

        if (head <= last) {
            sleep_ms(head_poll_ms);
            continue;
        }

        if (rpc_eth_get_block_by_number(rpc, head, hash, &ts) != 0) {
            sleep_ms(head_poll_ms);
            continue;
        }

        log_info("new head chain=%s block=%" PRIu64 " hash=%s ts=%" PRIu64,
                 chain_name(chain), head, hash, ts);

        if (redisio_hset_pool_head(redis, head, hash, ts) != 0) {
            log_warn("failed to persist pool:head block=%" PRIu64, head);
            sleep_ms(head_poll_ms);
            continue;
        }

        last = head;
        sleep_ms(head_poll_ms);
    }

    return 0;
}
