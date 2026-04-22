#define _POSIX_C_SOURCE 200809L
#define LOG_MODULE "rpc"

#include "botindex/rpc.h"

#include <curl/curl.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "botindex/log.h"
#include "botindex/util.h"

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} rpc_buf_t;

struct rpc_client {
    CURL *curl;
    struct curl_slist *headers;
    char *http_url;
    rpc_buf_t resp;
    unsigned long id;
};

static size_t rpc_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    rpc_buf_t *buf = (rpc_buf_t *)userdata;
    size_t n = size * nmemb;
    size_t need = 0;
    char *next = NULL;

    if (n == 0) {
        return 0;
    }

    need = buf->len + n + 1;
    if (need > buf->cap) {
        size_t cap = (buf->cap == 0) ? 1024 : buf->cap;
        while (cap < need) {
            cap *= 2;
        }
        next = realloc(buf->data, cap);
        if (next == NULL) {
            return 0;
        }
        buf->data = next;
        buf->cap = cap;
    }

    memcpy(buf->data + buf->len, ptr, n);
    buf->len += n;
    buf->data[buf->len] = '\0';
    return n;
}

static int rpc_parse_result_doc(const char *payload, yyjson_doc **out) {
    yyjson_doc *doc = yyjson_read(payload, strlen(payload), 0);
    yyjson_val *root = NULL;
    yyjson_val *error = NULL;

    if (doc == NULL) {
        log_error("json parse failed");
        return -1;
    }

    root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) {
        yyjson_doc_free(doc);
        log_error("json-rpc response root is not an object");
        return -1;
    }

    error = yyjson_obj_get(root, "error");
    if (error != NULL && !yyjson_is_null(error)) {
        yyjson_val *code = yyjson_obj_get(error, "code");
        yyjson_val *msg = yyjson_obj_get(error, "message");
        log_error("json-rpc error code=%" PRId64 " message=%s",
                  (int64_t)yyjson_get_sint(code), yyjson_get_str(msg));
        yyjson_doc_free(doc);
        return -1;
    }

    *out = doc;
    return 0;
}

rpc_client_t *rpc_client_new(const char *http_url) {
    rpc_client_t *c = NULL;

    if (http_url == NULL || http_url[0] == '\0') {
        return NULL;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        log_error("curl_global_init failed");
        return NULL;
    }

    c = calloc(1, sizeof(*c));
    if (c == NULL) {
        return NULL;
    }

    c->curl = curl_easy_init();
    if (c->curl == NULL) {
        free(c);
        return NULL;
    }

    c->http_url = strdup(http_url);
    if (c->http_url == NULL) {
        rpc_client_free(c);
        return NULL;
    }

    c->headers = curl_slist_append(c->headers, "Content-Type: application/json");
    if (c->headers == NULL) {
        rpc_client_free(c);
        return NULL;
    }

    curl_easy_setopt(c->curl, CURLOPT_URL, c->http_url);
    curl_easy_setopt(c->curl, CURLOPT_POST, 1L);
    curl_easy_setopt(c->curl, CURLOPT_HTTPHEADER, c->headers);
    curl_easy_setopt(c->curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(c->curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(c->curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(c->curl, CURLOPT_TCP_KEEPIDLE, 30L);
    curl_easy_setopt(c->curl, CURLOPT_TCP_KEEPINTVL, 30L);
    curl_easy_setopt(c->curl, CURLOPT_WRITEFUNCTION, rpc_write_cb);
    curl_easy_setopt(c->curl, CURLOPT_WRITEDATA, &c->resp);

    c->id = 1;
    return c;
}

void rpc_client_free(rpc_client_t *c) {
    if (c == NULL) {
        return;
    }

    curl_slist_free_all(c->headers);
    if (c->curl != NULL) {
        curl_easy_cleanup(c->curl);
    }
    free(c->resp.data);
    free(c->http_url);
    free(c);
}

int rpc_call(rpc_client_t *c, const char *method, const char *params_json, yyjson_doc **out) {
    yyjson_mut_doc *m_doc = NULL;
    yyjson_mut_val *root = NULL;
    yyjson_mut_val *params = NULL;
    yyjson_doc *p_doc = NULL;
    yyjson_val *p_root = NULL;
    yyjson_doc *result = NULL;
    char *body = NULL;
    size_t body_len = 0;
    CURLcode code;
    long http_status = 0;

    if (c == NULL || method == NULL || out == NULL) {
        return -1;
    }

    m_doc = yyjson_mut_doc_new(NULL);
    if (m_doc == NULL) {
        return -1;
    }

    root = yyjson_mut_obj(m_doc);
    yyjson_mut_doc_set_root(m_doc, root);
    yyjson_mut_obj_add_str(m_doc, root, "jsonrpc", "2.0");
    yyjson_mut_obj_add_uint(m_doc, root, "id", c->id++);
    yyjson_mut_obj_add_str(m_doc, root, "method", method);

    if (params_json == NULL || strcmp(params_json, "[]") == 0) {
        params = yyjson_mut_arr(m_doc);
    } else {
        p_doc = yyjson_read(params_json, strlen(params_json), 0);
        if (p_doc == NULL) {
            yyjson_mut_doc_free(m_doc);
            log_error("invalid params json method=%s", method);
            return -1;
        }
        p_root = yyjson_doc_get_root(p_doc);
        if (!yyjson_is_arr(p_root)) {
            yyjson_doc_free(p_doc);
            yyjson_mut_doc_free(m_doc);
            log_error("params is not array method=%s", method);
            return -1;
        }
        params = yyjson_val_mut_copy(m_doc, p_root);
        yyjson_doc_free(p_doc);
    }

    if (params == NULL || !yyjson_mut_obj_add_val(m_doc, root, "params", params)) {
        yyjson_mut_doc_free(m_doc);
        return -1;
    }

    body = yyjson_mut_write(m_doc, 0, &body_len);
    yyjson_mut_doc_free(m_doc);
    if (body == NULL) {
        return -1;
    }

    c->resp.len = 0;
    code = curl_easy_setopt(c->curl, CURLOPT_POSTFIELDS, body);
    if (code != CURLE_OK) {
        free(body);
        return -1;
    }
    code = curl_easy_setopt(c->curl, CURLOPT_POSTFIELDSIZE, (long)body_len);
    if (code != CURLE_OK) {
        free(body);
        return -1;
    }

    code = curl_easy_perform(c->curl);
    free(body);
    if (code != CURLE_OK) {
        log_error("rpc call failed method=%s err=%s", method, curl_easy_strerror(code));
        return -1;
    }

    code = curl_easy_getinfo(c->curl, CURLINFO_RESPONSE_CODE, &http_status);
    if (code != CURLE_OK || http_status < 200 || http_status >= 300) {
        log_error("rpc call bad http status method=%s status=%ld", method, http_status);
        return -1;
    }

    if (rpc_parse_result_doc(c->resp.data, &result) != 0) {
        return -1;
    }

    *out = result;
    return 0;
}

int rpc_eth_chain_id(rpc_client_t *c, uint64_t *out) {
    yyjson_doc *doc = NULL;
    yyjson_val *root = NULL;
    yyjson_val *result = NULL;
    int rc = -1;

    if (out == NULL) {
        return -1;
    }

    if (rpc_call(c, "eth_chainId", "[]", &doc) != 0) {
        return -1;
    }

    root = yyjson_doc_get_root(doc);
    result = yyjson_obj_get(root, "result");
    if (yyjson_is_str(result) && hex_to_u64(yyjson_get_str(result), out) == 0) {
        rc = 0;
    } else {
        log_error("eth_chainId missing/invalid result");
    }

    yyjson_doc_free(doc);
    return rc;
}

int rpc_eth_block_number(rpc_client_t *c, uint64_t *out) {
    yyjson_doc *doc = NULL;
    yyjson_val *root = NULL;
    yyjson_val *result = NULL;
    int rc = -1;

    if (out == NULL) {
        return -1;
    }

    if (rpc_call(c, "eth_blockNumber", "[]", &doc) != 0) {
        return -1;
    }

    root = yyjson_doc_get_root(doc);
    result = yyjson_obj_get(root, "result");
    if (yyjson_is_str(result) && hex_to_u64(yyjson_get_str(result), out) == 0) {
        rc = 0;
    } else {
        log_error("eth_blockNumber missing/invalid result");
    }

    yyjson_doc_free(doc);
    return rc;
}

int rpc_eth_get_block_by_number(rpc_client_t *c, uint64_t num, char hash_out[67], uint64_t *ts_out) {
    char params_json[64];
    yyjson_doc *doc = NULL;
    yyjson_val *root = NULL;
    yyjson_val *result = NULL;
    yyjson_val *hash = NULL;
    yyjson_val *ts = NULL;
    const char *hash_str = NULL;
    const char *ts_str = NULL;
    int rc = -1;

    if (hash_out == NULL || ts_out == NULL) {
        return -1;
    }

    (void)snprintf(params_json, sizeof(params_json), "[\"0x%" PRIx64 "\",false]", num);
    if (rpc_call(c, "eth_getBlockByNumber", params_json, &doc) != 0) {
        return -1;
    }

    root = yyjson_doc_get_root(doc);
    result = yyjson_obj_get(root, "result");
    if (!yyjson_is_obj(result)) {
        log_error("eth_getBlockByNumber result is not an object");
        yyjson_doc_free(doc);
        return -1;
    }

    hash = yyjson_obj_get(result, "hash");
    ts = yyjson_obj_get(result, "timestamp");
    hash_str = yyjson_get_str(hash);
    ts_str = yyjson_get_str(ts);

    if (hash_str == NULL || ts_str == NULL || strlen(hash_str) != 66 || hex_to_u64(ts_str, ts_out) != 0) {
        log_error("eth_getBlockByNumber missing hash/timestamp");
        yyjson_doc_free(doc);
        return -1;
    }

    memcpy(hash_out, hash_str, 67);
    rc = 0;

    yyjson_doc_free(doc);
    return rc;
}
