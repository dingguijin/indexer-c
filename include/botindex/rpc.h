#ifndef BOTINDEX_RPC_H
#define BOTINDEX_RPC_H

#include <stdint.h>

#include "yyjson.h"

typedef struct rpc_client rpc_client_t;

rpc_client_t *rpc_client_new(const char *http_url);
void rpc_client_free(rpc_client_t *c);
int rpc_call(rpc_client_t *c, const char *method, const char *params_json, yyjson_doc **out);
int rpc_eth_chain_id(rpc_client_t *c, uint64_t *out);
int rpc_eth_block_number(rpc_client_t *c, uint64_t *out);
int rpc_eth_get_block_by_number(rpc_client_t *c, uint64_t num, char hash_out[67], uint64_t *ts_out);

#endif
