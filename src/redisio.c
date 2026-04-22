#define _POSIX_C_SOURCE 200809L
#define LOG_MODULE "redisio"

#include "botindex/redisio.h"

#include <hiredis/hiredis.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "botindex/log.h"

typedef struct redisio {
    redisContext *ctx;
} redisio_t;

static int is_unix_socket_path(const char *path) {
    struct stat st = {0};

    if (path == NULL || path[0] == '\0') {
        return 0;
    }
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISSOCK(st.st_mode) ? 1 : 0;
}

static int redis_expect_status(const redisReply *reply) {
    if (reply == NULL) {
        return -1;
    }
    if (reply->type == REDIS_REPLY_STATUS &&
        (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "QUEUED") == 0)) {
        return 0;
    }
    return -1;
}

static int redis_sentinel_check(redisio_t *r, uint64_t expected_chain_id) {
    redisReply *reply = NULL;
    redisReply *set_reply = NULL;
    char *end = NULL;
    unsigned long long parsed = 0;
    uint64_t got = 0;

    reply = redisCommand(r->ctx, "GET meta:chain_id");
    if (reply == NULL) {
        log_error("GET meta:chain_id failed");
        return -1;
    }

    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        set_reply = redisCommand(r->ctx, "SET meta:chain_id %" PRIu64 " NX", expected_chain_id);
        if (set_reply == NULL) {
            log_error("SET meta:chain_id NX failed");
            return -1;
        }
        freeReplyObject(set_reply);

        reply = redisCommand(r->ctx, "GET meta:chain_id");
        if (reply == NULL) {
            log_error("re-read meta:chain_id failed");
            return -1;
        }
    }

    if (reply->type != REDIS_REPLY_STRING) {
        log_error("unexpected meta:chain_id type=%d", reply->type);
        freeReplyObject(reply);
        return -1;
    }

    parsed = strtoull(reply->str, &end, 10);
    if (end == NULL || *end != '\0') {
        log_error("invalid meta:chain_id value=%s", reply->str);
        freeReplyObject(reply);
        return -1;
    }
    got = (uint64_t)parsed;

    if (got != expected_chain_id) {
        log_error("redis sentinel mismatch expected=%" PRIu64 " actual=%" PRIu64,
                  expected_chain_id, got);
        freeReplyObject(reply);
        return -1;
    }

    freeReplyObject(reply);
    log_info("sentinel check passed");
    return 0;
}

redisio_t *redisio_connect(const config_t *cfg, chain_t chain) {
    redisio_t *r = NULL;
    redisReply *reply = NULL;
    uint8_t db = 0;
    uint64_t expected_chain_id = 0;

    if (cfg == NULL) {
        return NULL;
    }

    r = calloc(1, sizeof(*r));
    if (r == NULL) {
        return NULL;
    }

    if (is_unix_socket_path(cfg->redis.unix_socket)) {
        r->ctx = redisConnectUnix(cfg->redis.unix_socket);
    } else {
        r->ctx = redisConnect(cfg->redis.host, cfg->redis.port);
    }

    if (r->ctx == NULL || r->ctx->err != 0) {
        log_error("redis connect failed err=%s", (r->ctx != NULL) ? r->ctx->errstr : "oom");
        redisio_close(r);
        return NULL;
    }

    db = config_redis_db(cfg, chain);
    reply = redisCommand(r->ctx, "SELECT %u", (unsigned)db);
    if (redis_expect_status(reply) != 0) {
        log_error("redis SELECT failed db=%u", (unsigned)db);
        if (reply != NULL) {
            freeReplyObject(reply);
        }
        redisio_close(r);
        return NULL;
    }
    freeReplyObject(reply);

    expected_chain_id = config_expected_chain_id(cfg, chain);
    if (redis_sentinel_check(r, expected_chain_id) != 0) {
        redisio_close(r);
        return NULL;
    }

    return r;
}

void redisio_close(redisio_t *r) {
    if (r == NULL) {
        return;
    }
    if (r->ctx != NULL) {
        redisFree(r->ctx);
    }
    free(r);
}

int redisio_hset_pool_head(redisio_t *r, uint64_t block, const char *hash, uint64_t ts) {
    redisReply *reply = NULL;

    if (r == NULL || hash == NULL) {
        return -1;
    }

    reply = redisCommand(r->ctx,
                         "HSET pool:head block %" PRIu64 " hash %s ts %" PRIu64,
                         block, hash, ts);
    if (reply == NULL) {
        log_error("HSET pool:head failed");
        return -1;
    }
    freeReplyObject(reply);
    return 0;
}
