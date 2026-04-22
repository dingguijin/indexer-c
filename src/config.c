#define _POSIX_C_SOURCE 200809L
#define LOG_MODULE "config"

#include "botindex/config.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "botindex/log.h"
#include "toml.h"

static int chain_key_index(const char *name) {
    if (strcmp(name, "eth") == 0) {
        return 0;
    }
    if (strcmp(name, "bsc") == 0) {
        return 1;
    }
    if (strcmp(name, "polygon") == 0) {
        return 2;
    }
    if (strcmp(name, "base") == 0) {
        return 3;
    }
    if (strcmp(name, "arb") == 0) {
        return 4;
    }
    return -1;
}

static char *dup_or_empty(const char *s) {
    if (s == NULL) {
        return strdup("");
    }
    return strdup(s);
}

static int load_redis(toml_table_t *root, config_t *cfg) {
    toml_table_t *redis = toml_table_in(root, "redis");
    toml_table_t *db_map = NULL;
    toml_datum_t d = {0};

    if (redis == NULL) {
        log_error("missing required key redis");
        return -1;
    }

    d = toml_string_in(redis, "unix_socket");
    cfg->redis.unix_socket = dup_or_empty(d.ok ? d.u.s : "");
    if (d.ok) {
        free(d.u.s);
    }
    if (cfg->redis.unix_socket == NULL) {
        return -1;
    }

    d = toml_string_in(redis, "host");
    if (!d.ok) {
        log_error("missing required key redis.host");
        return -1;
    }
    cfg->redis.host = strdup(d.u.s);
    free(d.u.s);
    if (cfg->redis.host == NULL) {
        return -1;
    }

    d = toml_int_in(redis, "port");
    if (!d.ok || d.u.i <= 0 || d.u.i > INT_MAX) {
        log_error("invalid key redis.port");
        return -1;
    }
    cfg->redis.port = (int)d.u.i;

    db_map = toml_table_in(redis, "db_map");
    if (db_map == NULL) {
        log_error("missing required key redis.db_map");
        return -1;
    }

    for (int i = 0; i < 5; i++) {
        static const char *keys[5] = {"eth", "bsc", "polygon", "base", "arb"};
        d = toml_int_in(db_map, keys[i]);
        if (!d.ok || d.u.i < 0 || d.u.i > UCHAR_MAX) {
            log_error("invalid key redis.db_map.%s", keys[i]);
            return -1;
        }
        cfg->redis.db_map[i] = (uint8_t)d.u.i;
    }

    return 0;
}

static int load_chains(toml_table_t *root, config_t *cfg) {
    toml_table_t *chains = toml_table_in(root, "chains");

    if (chains == NULL) {
        log_error("missing required key chains");
        return -1;
    }

    for (int i = 0; i < 5; i++) {
        static const char *keys[5] = {"eth", "bsc", "polygon", "base", "arb"};
        toml_table_t *chain = toml_table_in(chains, keys[i]);
        toml_datum_t d = {0};

        if (chain == NULL) {
            continue;
        }

        d = toml_int_in(chain, "chain_id");
        if (d.ok && d.u.i >= 0) {
            cfg->chains[i].chain_id = (uint64_t)d.u.i;
        }

        d = toml_string_in(chain, "rpc_http");
        if (d.ok) {
            cfg->chains[i].rpc_http = strdup(d.u.s);
            free(d.u.s);
            if (cfg->chains[i].rpc_http == NULL) {
                return -1;
            }
        }

        d = toml_int_in(chain, "head_poll_ms");
        if (d.ok && d.u.i > 0 && d.u.i <= INT_MAX) {
            cfg->chains[i].head_poll_ms = (int)d.u.i;
        }
    }

    /* Optional sections are intentionally ignored in M1 but must parse cleanly. */
    for (int i = 0;; i++) {
        const char *key = toml_key_in(root, i);
        if (key == NULL) {
            break;
        }
        (void)chain_key_index(key);
    }

    return 0;
}

int config_load(const char *path, config_t **out) {
    FILE *fp = NULL;
    toml_table_t *root = NULL;
    config_t *cfg = NULL;
    char errbuf[200];

    if (path == NULL || out == NULL) {
        return -1;
    }

    fp = fopen(path, "r");
    if (fp == NULL) {
        log_error("failed to open config path=%s", path);
        return -1;
    }

    root = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);
    if (root == NULL) {
        log_error("toml parse error path=%s err=%s", path, errbuf);
        return -1;
    }

    cfg = calloc(1, sizeof(*cfg));
    if (cfg == NULL) {
        toml_free(root);
        return -1;
    }

    if (load_redis(root, cfg) != 0 || load_chains(root, cfg) != 0) {
        toml_free(root);
        config_free(cfg);
        return -1;
    }

    toml_free(root);
    *out = cfg;
    return 0;
}

void config_free(config_t *cfg) {
    if (cfg == NULL) {
        return;
    }

    free(cfg->redis.unix_socket);
    free(cfg->redis.host);
    for (int i = 0; i < 5; i++) {
        free(cfg->chains[i].rpc_http);
    }
    free(cfg);
}

uint8_t config_redis_db(const config_t *cfg, chain_t chain) {
    int idx = chain_index(chain);
    if (cfg == NULL || idx < 0) {
        return 0;
    }
    return cfg->redis.db_map[idx];
}

const char *config_rpc_http(const config_t *cfg, chain_t chain) {
    int idx = chain_index(chain);
    if (cfg == NULL || idx < 0) {
        return NULL;
    }
    return cfg->chains[idx].rpc_http;
}

int config_head_poll_ms(const config_t *cfg, chain_t chain) {
    int idx = chain_index(chain);
    if (cfg == NULL || idx < 0) {
        return 0;
    }
    return cfg->chains[idx].head_poll_ms;
}

uint64_t config_expected_chain_id(const config_t *cfg, chain_t chain) {
    int idx = chain_index(chain);
    if (cfg == NULL || idx < 0) {
        return 0;
    }
    return cfg->chains[idx].chain_id;
}
