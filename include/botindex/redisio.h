#ifndef BOTINDEX_REDISIO_H
#define BOTINDEX_REDISIO_H

#include <stdint.h>

#include "botindex/chain.h"
#include "botindex/config.h"

typedef struct redisio redisio_t;

redisio_t *redisio_connect(const config_t *cfg, chain_t chain);
void redisio_close(redisio_t *r);
int redisio_hset_pool_head(redisio_t *r, uint64_t block, const char *hash, uint64_t ts);

#endif
