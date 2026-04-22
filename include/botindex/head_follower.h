#ifndef BOTINDEX_HEAD_FOLLOWER_H
#define BOTINDEX_HEAD_FOLLOWER_H

#include <signal.h>

#include "botindex/chain.h"
#include "botindex/redisio.h"
#include "botindex/rpc.h"

int head_follower_run(chain_t chain, int head_poll_ms, rpc_client_t *rpc, redisio_t *redis,
                      volatile sig_atomic_t *running);

#endif
