#ifndef BOTINDEX_CONFIG_H
#define BOTINDEX_CONFIG_H

#include <stdint.h>

#include "botindex/chain.h"

typedef struct {
    struct {
        char *unix_socket;
        char *host;
        int port;
        uint8_t db_map[5];
    } redis;
    struct {
        uint64_t chain_id;
        char *rpc_http;
        int head_poll_ms;
    } chains[5];
} config_t;

int config_load(const char *path, config_t **out);
void config_free(config_t *cfg);
uint8_t config_redis_db(const config_t *cfg, chain_t chain);
const char *config_rpc_http(const config_t *cfg, chain_t chain);
int config_head_poll_ms(const config_t *cfg, chain_t chain);
uint64_t config_expected_chain_id(const config_t *cfg, chain_t chain);

#endif
