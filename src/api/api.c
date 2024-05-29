//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//   Błażej Sowa, <blazej@fictionlab.pl>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/net/config.h"
#include "zenoh-pico/net/filtering.h"
#include "zenoh-pico/net/logger.h"
#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/net/sample.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/multicast.h"
#include "zenoh-pico/transport/unicast.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"
#include "zenoh-pico/utils/uuid.h"

/********* Data Types Handlers *********/

int8_t z_view_str_wrap(z_view_string_t *str, const char *value) {
    str->_val = _z_string_wrap((char *)value);
    return _Z_RES_OK;
}

const z_loaned_string_t *z_string_array_get(const z_loaned_string_array_t *a, size_t k) {
    return _z_string_vec_get(a, k);
}

size_t z_string_array_len(const z_loaned_string_array_t *a) { return _z_string_vec_len(a); }

_Bool z_string_array_is_empty(const z_loaned_string_array_t *a) { return _z_string_vec_is_empty(a); }

int8_t z_view_keyexpr_from_string(z_view_keyexpr_t *keyexpr, const char *name) {
    keyexpr->_val = _z_rname(name);
    return _Z_RES_OK;
}

int8_t z_view_keyexpr_from_string_unchecked(z_view_keyexpr_t *keyexpr, const char *name) {
    keyexpr->_val = _z_rname(name);
    return _Z_RES_OK;
}

int8_t z_keyexpr_to_string(const z_loaned_keyexpr_t *keyexpr, z_owned_string_t *s) {
    int8_t ret = _Z_RES_OK;
    if (keyexpr->_id == Z_RESOURCE_ID_NONE) {
        s->_val = (_z_string_t *)z_malloc(sizeof(_z_string_t));
        if (s->_val != NULL) {
            s->_val->val = _z_str_clone(keyexpr->_suffix);
            s->_val->len = strlen(keyexpr->_suffix);
        } else {
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

int8_t zp_keyexpr_resolve(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, z_owned_string_t *str) {
    _z_keyexpr_t ekey = _z_get_expanded_key_from_key(&_Z_RC_IN_VAL(zs), keyexpr);
    *str->_val = _z_string_make((char *)ekey._suffix);  // ekey will be out of scope so
                                                        //  - suffix can be safely casted as non-const
                                                        //  - suffix does not need to be copied
    return _Z_RES_OK;
}

_Bool z_keyexpr_is_initialized(const z_loaned_keyexpr_t *keyexpr) {
    _Bool ret = false;

    if ((keyexpr->_id != Z_RESOURCE_ID_NONE) || (keyexpr->_suffix != NULL)) {
        ret = true;
    }

    return ret;
}

int8_t z_keyexpr_is_canon(const char *start, size_t len) { return _z_keyexpr_is_canon(start, len); }

int8_t zp_keyexpr_is_canon_null_terminated(const char *start) { return _z_keyexpr_is_canon(start, strlen(start)); }

int8_t z_keyexpr_canonize(char *start, size_t *len) { return _z_keyexpr_canonize(start, len); }

int8_t zp_keyexpr_canonize_null_terminated(char *start) {
    zp_keyexpr_canon_status_t ret = Z_KEYEXPR_CANON_SUCCESS;

    size_t len = strlen(start);
    size_t newlen = len;
    ret = _z_keyexpr_canonize(start, &newlen);
    if (newlen < len) {
        start[newlen] = '\0';
    }

    return ret;
}

_Bool z_keyexpr_includes(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r) {
    if ((l->_id == Z_RESOURCE_ID_NONE) && (r->_id == Z_RESOURCE_ID_NONE)) {
        return zp_keyexpr_includes_null_terminated(l->_suffix, r->_suffix);
    }
    return false;
}

_Bool zp_keyexpr_includes_null_terminated(const char *l, const char *r) {
    if (l != NULL && r != NULL) {
        return _z_keyexpr_includes(l, strlen(l), r, strlen(r));
    }
    return false;
}

_Bool z_keyexpr_intersects(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r) {
    if ((l->_id == Z_RESOURCE_ID_NONE) && (r->_id == Z_RESOURCE_ID_NONE)) {
        return zp_keyexpr_intersect_null_terminated(l->_suffix, r->_suffix);
    }
    return false;
}

_Bool zp_keyexpr_intersect_null_terminated(const char *l, const char *r) {
    if (l != NULL && r != NULL) {
        return _z_keyexpr_intersects(l, strlen(l), r, strlen(r));
    }
    return false;
}

_Bool z_keyexpr_equals(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r) {
    if ((l->_id == Z_RESOURCE_ID_NONE) && (r->_id == Z_RESOURCE_ID_NONE)) {
        return zp_keyexpr_equals_null_terminated(l->_suffix, r->_suffix);
    }
    return false;
}

_Bool zp_keyexpr_equals_null_terminated(const char *l, const char *r) {
    if (l != NULL && r != NULL) {
        size_t llen = strlen(l);
        if (llen == strlen(r)) {
            if (strncmp(l, r, llen) == 0) {
                return true;
            }
        }
    }
    return false;
}

void z_config_new(z_owned_config_t *config) { config->_val = _z_config_empty(); }

void z_config_default(z_owned_config_t *config) { config->_val = _z_config_default(); }

const char *zp_config_get(const z_loaned_config_t *config, uint8_t key) { return _z_config_get(config, key); }

int8_t zp_config_insert(z_loaned_config_t *config, uint8_t key, const char *value) {
    return _zp_config_insert(config, key, value);
}

void z_scouting_config_default(z_owned_scouting_config_t *sc) {
    sc->_val = _z_config_empty();

    _zp_config_insert(sc->_val, Z_CONFIG_MULTICAST_LOCATOR_KEY, Z_CONFIG_MULTICAST_LOCATOR_DEFAULT);
    _zp_config_insert(sc->_val, Z_CONFIG_SCOUTING_TIMEOUT_KEY, Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT);
    _zp_config_insert(sc->_val, Z_CONFIG_SCOUTING_WHAT_KEY, Z_CONFIG_SCOUTING_WHAT_DEFAULT);
}

int8_t z_scouting_config_from(z_owned_scouting_config_t *sc, const z_loaned_config_t *c) {
    sc->_val = _z_config_empty();

    char *opt;
    opt = _z_config_get(c, Z_CONFIG_MULTICAST_LOCATOR_KEY);
    if (opt == NULL) {
        _zp_config_insert(sc->_val, Z_CONFIG_MULTICAST_LOCATOR_KEY, Z_CONFIG_MULTICAST_LOCATOR_DEFAULT);
    } else {
        _zp_config_insert(sc->_val, Z_CONFIG_MULTICAST_LOCATOR_KEY, opt);
    }

    opt = _z_config_get(c, Z_CONFIG_SCOUTING_TIMEOUT_KEY);
    if (opt == NULL) {
        _zp_config_insert(sc->_val, Z_CONFIG_SCOUTING_TIMEOUT_KEY, Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT);
    } else {
        _zp_config_insert(sc->_val, Z_CONFIG_SCOUTING_TIMEOUT_KEY, opt);
    }

    opt = _z_config_get(c, Z_CONFIG_SCOUTING_WHAT_KEY);
    if (opt == NULL) {
        _zp_config_insert(sc->_val, Z_CONFIG_SCOUTING_WHAT_KEY, Z_CONFIG_SCOUTING_WHAT_DEFAULT);
    } else {
        _zp_config_insert(sc->_val, Z_CONFIG_SCOUTING_WHAT_KEY, opt);
    }

    return _Z_RES_OK;
}

const char *zp_scouting_config_get(const z_loaned_scouting_config_t *sc, uint8_t key) { return _z_config_get(sc, key); }

int8_t zp_scouting_config_insert(z_loaned_scouting_config_t *sc, uint8_t key, const char *value) {
    return _zp_config_insert(sc, key, value);
}

z_encoding_t z_encoding(z_encoding_prefix_t prefix, const char *suffix) {
    return (_z_encoding_t){
        .id = prefix, .schema = _z_bytes_wrap((const uint8_t *)suffix, (suffix == NULL) ? (size_t)0 : strlen(suffix))};
}

z_encoding_t z_encoding_default(void) { return z_encoding(Z_ENCODING_PREFIX_DEFAULT, NULL); }

const z_loaned_bytes_t *z_value_payload(const z_loaned_value_t *value) { return &value->payload; }

size_t z_bytes_len(const z_loaned_bytes_t *bytes) { return bytes->len; }

int8_t z_bytes_decode_into_string(const z_loaned_bytes_t *bytes, z_owned_string_t *s) {
    // Init owned string
    z_string_null(s);
    s->_val = (_z_string_t *)z_malloc(sizeof(_z_string_t));
    if (s->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Convert bytes to string
    *s->_val = _z_string_from_bytes(bytes);
    return _Z_RES_OK;
}

int8_t z_bytes_encode_from_string(z_owned_bytes_t *buffer, const z_loaned_string_t *s) {
    // Init owned bytes
    z_bytes_null(buffer);
    buffer->_val = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
    if (buffer->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Encode data
    *buffer->_val = _z_bytes_wrap((uint8_t *)s->val, s->len);
    return _Z_RES_OK;
}

_Bool z_timestamp_check(z_timestamp_t ts) { return _z_timestamp_check(&ts); }

z_query_target_t z_query_target_default(void) { return Z_QUERY_TARGET_DEFAULT; }

z_query_consolidation_t z_query_consolidation_auto(void) {
    return (z_query_consolidation_t){.mode = Z_CONSOLIDATION_MODE_AUTO};
}

z_query_consolidation_t z_query_consolidation_latest(void) {
    return (z_query_consolidation_t){.mode = Z_CONSOLIDATION_MODE_LATEST};
}

z_query_consolidation_t z_query_consolidation_monotonic(void) {
    return (z_query_consolidation_t){.mode = Z_CONSOLIDATION_MODE_MONOTONIC};
}

z_query_consolidation_t z_query_consolidation_none(void) {
    return (z_query_consolidation_t){.mode = Z_CONSOLIDATION_MODE_NONE};
}

z_query_consolidation_t z_query_consolidation_default(void) { return z_query_consolidation_auto(); }

void z_query_parameters(const z_loaned_query_t *query, z_view_string_t *parameters) {
    parameters->_val.val = query->in->val._parameters;
    parameters->_val.len = strlen(query->in->val._parameters);
}

const z_loaned_value_t *z_query_value(const z_loaned_query_t *query) { return &query->in->val._value; }

#if Z_FEATURE_ATTACHMENT == 1
z_attachment_t z_query_attachment(const z_loaned_query_t *query) { return query->in->val.attachment; }
#endif

const z_loaned_keyexpr_t *z_query_keyexpr(const z_loaned_query_t *query) { return &query->in->val._key; }

void z_closure_sample_call(const z_owned_closure_sample_t *closure, const z_loaned_sample_t *sample) {
    if (closure->call != NULL) {
        (closure->call)(sample, closure->context);
    }
}

void z_closure_owned_sample_call(const z_owned_closure_owned_sample_t *closure, z_owned_sample_t *sample) {
    if (closure->call != NULL) {
        (closure->call)(sample, closure->context);
    }
}

void z_closure_query_call(const z_owned_closure_query_t *closure, const z_loaned_query_t *query) {
    if (closure->call != NULL) {
        (closure->call)(query, closure->context);
    }
}

void z_closure_owned_query_call(const z_owned_closure_owned_query_t *closure, z_owned_query_t *query) {
    if (closure->call != NULL) {
        (closure->call)(query, closure->context);
    }
}

void z_closure_reply_call(const z_owned_closure_reply_t *closure, const z_loaned_reply_t *reply) {
    if (closure->call != NULL) {
        (closure->call)(reply, closure->context);
    }
}

void z_closure_owned_reply_call(const z_owned_closure_owned_reply_t *closure, z_owned_reply_t *reply) {
    if (closure->call != NULL) {
        (closure->call)(reply, closure->context);
    }
}

void z_closure_hello_call(const z_owned_closure_hello_t *closure, z_owned_hello_t *hello) {
    if (closure->call != NULL) {
        (closure->call)(hello, closure->context);
    }
}

void z_closure_zid_call(const z_owned_closure_zid_t *closure, const z_id_t *id) {
    if (closure->call != NULL) {
        (closure->call)(id, closure->context);
    }
}

#define OWNED_FUNCTIONS_PTR(type, name, f_copy, f_free)                                             \
    _Bool z_##name##_check(const z_owned_##name##_t *obj) { return obj->_val != NULL; }             \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *obj) { return obj->_val; } \
    z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *obj) { return obj->_val; }         \
    void z_##name##_null(z_owned_##name##_t *obj) { obj->_val = NULL; }                             \
    z_owned_##name##_t *z_##name##_move(z_owned_##name##_t *obj) { return obj; }                    \
    int8_t z_##name##_clone(z_owned_##name##_t *obj, const z_loaned_##name##_t *src) {              \
        int8_t ret = _Z_RES_OK;                                                                     \
        obj->_val = (type *)z_malloc(sizeof(type));                                                 \
        if (obj->_val != NULL) {                                                                    \
            f_copy(obj->_val, src);                                                                 \
        } else {                                                                                    \
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;                                                      \
        }                                                                                           \
        return ret;                                                                                 \
    }                                                                                               \
    void z_##name##_drop(z_owned_##name##_t *obj) {                                                 \
        if (obj->_val != NULL) {                                                                    \
            f_free(&obj->_val);                                                                     \
        }                                                                                           \
    }

#define OWNED_FUNCTIONS_RC(name)                                                                    \
    _Bool z_##name##_check(const z_owned_##name##_t *val) { return val->_rc.in != NULL; }           \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *val) { return &val->_rc; } \
    z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *val) { return &val->_rc; }         \
    void z_##name##_null(z_owned_##name##_t *val) { val->_rc.in = NULL; }                           \
    z_owned_##name##_t *z_##name##_move(z_owned_##name##_t *val) { return val; }                    \
    int8_t z_##name##_clone(z_owned_##name##_t *obj, const z_loaned_##name##_t *src) {              \
        int8_t ret = _Z_RES_OK;                                                                     \
        obj->_rc = _z_##name##_rc_clone((z_loaned_##name##_t *)src);                                \
        if (obj->_rc.in == NULL) {                                                                  \
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;                                                      \
        }                                                                                           \
        return ret;                                                                                 \
    }                                                                                               \
    void z_##name##_drop(z_owned_##name##_t *val) {                                                 \
        if (val->_rc.in != NULL) {                                                                  \
            if (_z_##name##_rc_drop(&val->_rc)) {                                                   \
                val->_rc.in = NULL;                                                                 \
            }                                                                                       \
        }                                                                                           \
    }

#define VIEW_FUNCTIONS_PTR(type, name)                                                                   \
    const z_loaned_##name##_t *z_view_##name##_loan(const z_view_##name##_t *obj) { return &obj->_val; } \
    z_loaned_##name##_t *z_view_##name##_loan_mut(z_view_##name##_t *obj) { return &obj->_val; }

static inline void _z_owner_noop_copy(void *dst, const void *src) {
    (void)(dst);
    (void)(src);
}

OWNED_FUNCTIONS_PTR(_z_config_t, config, _z_owner_noop_copy, _z_config_free)
OWNED_FUNCTIONS_PTR(_z_scouting_config_t, scouting_config, _z_owner_noop_copy, _z_scouting_config_free)
OWNED_FUNCTIONS_PTR(_z_string_t, string, _z_string_copy, _z_string_free)
OWNED_FUNCTIONS_PTR(_z_value_t, value, _z_value_copy, _z_value_free)

OWNED_FUNCTIONS_PTR(_z_keyexpr_t, keyexpr, _z_keyexpr_copy, _z_keyexpr_free)
VIEW_FUNCTIONS_PTR(_z_keyexpr_t, keyexpr)
VIEW_FUNCTIONS_PTR(_z_string_t, string)

OWNED_FUNCTIONS_PTR(_z_hello_t, hello, _z_owner_noop_copy, _z_hello_free)
OWNED_FUNCTIONS_PTR(_z_string_vec_t, string_array, _z_owner_noop_copy, _z_string_vec_free)
VIEW_FUNCTIONS_PTR(_z_string_vec_t, string_array)
OWNED_FUNCTIONS_PTR(_z_bytes_t, bytes, _z_bytes_copy, _z_bytes_free)

static _z_bytes_t _z_bytes_from_owned_bytes(z_owned_bytes_t *bytes) {
    _z_bytes_t b = _z_bytes_empty();
    if ((bytes != NULL) && (bytes->_val != NULL)) {
        b = _z_bytes_wrap(bytes->_val->start, bytes->_val->len);
    }
    return b;
}

OWNED_FUNCTIONS_RC(sample)
OWNED_FUNCTIONS_RC(session)

#define OWNED_FUNCTIONS_CLOSURE(ownedtype, name, f_call, f_drop)                   \
    _Bool z_##name##_check(const ownedtype *val) { return val->call != NULL; }     \
    ownedtype *z_##name##_move(ownedtype *val) { return val; }                     \
    void z_##name##_drop(ownedtype *val) {                                         \
        if (val->drop != NULL) {                                                   \
            (val->drop)(val->context);                                             \
            val->drop = NULL;                                                      \
        }                                                                          \
        val->call = NULL;                                                          \
        val->context = NULL;                                                       \
    }                                                                              \
    void z_##name##_null(ownedtype *val) {                                         \
        val->call = NULL;                                                          \
        val->drop = NULL;                                                          \
        val->context = NULL;                                                       \
    }                                                                              \
    int8_t z_##name(ownedtype *closure, f_call call, f_drop drop, void *context) { \
        closure->call = call;                                                      \
        closure->drop = drop;                                                      \
        closure->context = context;                                                \
                                                                                   \
        return _Z_RES_OK;                                                          \
    }

OWNED_FUNCTIONS_CLOSURE(z_owned_closure_sample_t, closure_sample, _z_data_handler_t, z_dropper_handler_t)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_owned_sample_t, closure_owned_sample, z_owned_sample_handler_t,
                        z_dropper_handler_t)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_query_t, closure_query, _z_queryable_handler_t, z_dropper_handler_t)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_owned_query_t, closure_owned_query, z_owned_query_handler_t,
                        z_dropper_handler_t)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_reply_t, closure_reply, _z_reply_handler_t, z_dropper_handler_t)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_owned_reply_t, closure_owned_reply, z_owned_reply_handler_t,
                        z_dropper_handler_t)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_hello_t, closure_hello, z_owned_hello_handler_t, z_dropper_handler_t)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_zid_t, closure_zid, z_id_handler_t, z_dropper_handler_t)

/************* Primitives **************/
typedef struct __z_hello_handler_wrapper_t {
    z_owned_hello_handler_t user_call;
    void *ctx;
} __z_hello_handler_wrapper_t;

void __z_hello_handler(_z_hello_t *hello, __z_hello_handler_wrapper_t *wrapped_ctx) {
    z_owned_hello_t ohello = {._val = hello};
    wrapped_ctx->user_call(&ohello, wrapped_ctx->ctx);
}

int8_t z_scout(z_owned_scouting_config_t *config, z_owned_closure_hello_t *callback) {
    int8_t ret = _Z_RES_OK;

    void *ctx = callback->context;
    callback->context = NULL;

    // TODO[API-NET]: When API and NET are a single layer, there is no wrap the user callback and args
    //                to enclose the z_reply_t into a z_owned_reply_t.
    __z_hello_handler_wrapper_t *wrapped_ctx =
        (__z_hello_handler_wrapper_t *)z_malloc(sizeof(__z_hello_handler_wrapper_t));
    if (wrapped_ctx != NULL) {
        wrapped_ctx->user_call = callback->call;
        wrapped_ctx->ctx = ctx;

        char *opt_as_str = _z_config_get(config->_val, Z_CONFIG_SCOUTING_WHAT_KEY);
        if (opt_as_str == NULL) {
            opt_as_str = (char *)Z_CONFIG_SCOUTING_WHAT_DEFAULT;
        }
        z_what_t what = strtol(opt_as_str, NULL, 10);

        opt_as_str = _z_config_get(config->_val, Z_CONFIG_MULTICAST_LOCATOR_KEY);
        if (opt_as_str == NULL) {
            opt_as_str = (char *)Z_CONFIG_MULTICAST_LOCATOR_DEFAULT;
        }
        char *mcast_locator = opt_as_str;

        opt_as_str = _z_config_get(config->_val, Z_CONFIG_SCOUTING_TIMEOUT_KEY);
        if (opt_as_str == NULL) {
            opt_as_str = (char *)Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT;
        }
        uint32_t timeout = strtoul(opt_as_str, NULL, 10);

        _z_id_t zid = _z_id_empty();
        char *zid_str = _z_config_get(config->_val, Z_CONFIG_SESSION_ZID_KEY);
        if (zid_str != NULL) {
            _z_uuid_to_bytes(zid.id, zid_str);
        }

        _z_scout(what, zid, mcast_locator, timeout, __z_hello_handler, wrapped_ctx, callback->drop, ctx);

        z_free(wrapped_ctx);
        z_scouting_config_drop(config);
    } else {
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    return ret;
}

int8_t z_open(z_owned_session_t *zs, z_owned_config_t *config) {
    z_session_null(zs);

    // Create rc
    _z_session_rc_t zsrc = _z_session_rc_new();
    if (zsrc.in == NULL) {
        z_config_drop(config);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Open session
    int8_t ret = _z_open(&zsrc.in->val, config->_val);
    if (ret != _Z_RES_OK) {
        _z_session_rc_drop(&zsrc);
        z_config_drop(config);
        return ret;
    }
    // Store rc in session
    zs->_rc = zsrc;
    z_config_drop(config);
    return _Z_RES_OK;
}

int8_t z_close(z_owned_session_t *zs) {
    if (zs == NULL || !z_session_check(zs)) {
        return _Z_RES_OK;
    }
    _z_close(&_Z_OWNED_RC_IN_VAL(zs));
    z_session_drop(zs);
    return _Z_RES_OK;
}

int8_t z_info_peers_zid(const z_loaned_session_t *zs, z_owned_closure_zid_t *callback) {
    // Call transport function
    switch (_Z_RC_IN_VAL(zs)._tp._type) {
        case _Z_TRANSPORT_MULTICAST_TYPE:
        case _Z_TRANSPORT_RAWETH_TYPE:
            _zp_multicast_fetch_zid(&(_Z_RC_IN_VAL(zs)._tp), callback);
            break;
        default:
            break;
    }
    // Note and clear context
    void *ctx = callback->context;
    callback->context = NULL;
    // Drop if needed
    if (callback->drop != NULL) {
        callback->drop(ctx);
    }
    return 0;
}

int8_t z_info_routers_zid(const z_loaned_session_t *zs, z_owned_closure_zid_t *callback) {
    // Call transport function
    switch (_Z_RC_IN_VAL(zs)._tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            _zp_unicast_fetch_zid(&(_Z_RC_IN_VAL(zs)._tp), callback);
            break;
        default:
            break;
    }
    // Note and clear context
    void *ctx = callback->context;
    callback->context = NULL;
    // Drop if needed
    if (callback->drop != NULL) {
        callback->drop(ctx);
    }
    return 0;
}

z_id_t z_info_zid(const z_loaned_session_t *zs) { return _Z_RC_IN_VAL(zs)._local_zid; }

const z_loaned_keyexpr_t *z_sample_keyexpr(const z_loaned_sample_t *sample) { return &_Z_RC_IN_VAL(sample).keyexpr; }
z_sample_kind_t z_sample_kind(const z_loaned_sample_t *sample) { return _Z_RC_IN_VAL(sample).kind; }
const z_loaned_bytes_t *z_sample_payload(const z_loaned_sample_t *sample) { return &_Z_RC_IN_VAL(sample).payload; }
z_timestamp_t z_sample_timestamp(const z_loaned_sample_t *sample) { return _Z_RC_IN_VAL(sample).timestamp; }
z_encoding_t z_sample_encoding(const z_loaned_sample_t *sample) { return _Z_RC_IN_VAL(sample).encoding; }
z_qos_t z_sample_qos(const z_loaned_sample_t *sample) { return _Z_RC_IN_VAL(sample).qos; }
#if Z_FEATURE_ATTACHMENT == 1
z_attachment_t z_sample_attachment(const z_loaned_sample_t *sample) { return _Z_RC_IN_VAL(sample).attachment; }
#endif

const char *z_str_data(const z_loaned_string_t *str) { return str->val; }

#if Z_FEATURE_PUBLICATION == 1
int8_t _z_publisher_drop(_z_publisher_t **pub) {
    int8_t ret = _Z_RES_OK;

    ret = _z_undeclare_publisher(*pub);
    _z_publisher_free(pub);

    return ret;
}

OWNED_FUNCTIONS_PTR(_z_publisher_t, publisher, _z_owner_noop_copy, _z_publisher_drop)

void z_put_options_default(z_put_options_t *options) {
    options->congestion_control = Z_CONGESTION_CONTROL_DEFAULT;
    options->priority = Z_PRIORITY_DEFAULT;
    options->encoding = z_encoding_default();
#if Z_FEATURE_ATTACHMENT == 1
    options->attachment = z_attachment_null();
#endif
}

void z_delete_options_default(z_delete_options_t *options) {
    options->congestion_control = Z_CONGESTION_CONTROL_DEFAULT;
    options->priority = Z_PRIORITY_DEFAULT;
}

int8_t z_put(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, const uint8_t *payload,
             z_zint_t payload_len, const z_put_options_t *options) {
    int8_t ret = 0;

    z_put_options_t opt;
    z_put_options_default(&opt);
    if (options != NULL) {
        opt.congestion_control = options->congestion_control;
        opt.encoding = options->encoding;
        opt.priority = options->priority;
#if Z_FEATURE_ATTACHMENT == 1
        opt.attachment = options->attachment;
#endif
    }
    ret = _z_write(&_Z_RC_IN_VAL(zs), *keyexpr, (const uint8_t *)payload, payload_len, opt.encoding, Z_SAMPLE_KIND_PUT,
                   opt.congestion_control, opt.priority
#if Z_FEATURE_ATTACHMENT == 1
                   ,
                   opt.attachment
#endif
    );

    // Trigger local subscriptions
#if Z_FEATURE_ATTACHMENT == 1
    z_attachment_t att = opt.attachment;
#else
    z_attachment_t att = z_attachment_null();
#endif
    _z_trigger_local_subscriptions(&_Z_RC_IN_VAL(zs), *keyexpr, payload, payload_len,
                                   _z_n_qos_make(0, opt.congestion_control == Z_CONGESTION_CONTROL_BLOCK, opt.priority),
                                   att

    );

    return ret;
}

int8_t z_delete(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, const z_delete_options_t *options) {
    int8_t ret = 0;

    z_delete_options_t opt;
    z_delete_options_default(&opt);
    if (options != NULL) {
        opt.congestion_control = options->congestion_control;
        opt.priority = options->priority;
    }
    ret = _z_write(&_Z_RC_IN_VAL(zs), *keyexpr, NULL, 0, z_encoding_default(), Z_SAMPLE_KIND_DELETE,
                   opt.congestion_control, opt.priority
#if Z_FEATURE_ATTACHMENT == 1
                   ,
                   z_attachment_null()
#endif
    );

    return ret;
}

void z_publisher_options_default(z_publisher_options_t *options) {
    options->congestion_control = Z_CONGESTION_CONTROL_DEFAULT;
    options->priority = Z_PRIORITY_DEFAULT;
}

int8_t z_declare_publisher(z_owned_publisher_t *pub, const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr,
                           const z_publisher_options_t *options) {
    _z_keyexpr_t key = *keyexpr;

    pub->_val = NULL;
    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
    if (_Z_RC_IN_VAL(zs)._tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(&_Z_RC_IN_VAL(zs), keyexpr);
        if (r == NULL) {
            uint16_t id = _z_declare_resource(&_Z_RC_IN_VAL(zs), *keyexpr);
            key = _z_rid_with_suffix(id, NULL);
        }
    }
    // Set options
    z_publisher_options_t opt;
    z_publisher_options_default(&opt);
    if (options != NULL) {
        opt.congestion_control = options->congestion_control;
        opt.priority = options->priority;
    }
    // Set publisher
    _z_publisher_t *int_pub = _z_declare_publisher(zs, key, opt.congestion_control, opt.priority);
    if (int_pub == NULL) {
        if (key._id != Z_RESOURCE_ID_NONE) {
            _z_undeclare_resource(&_Z_RC_IN_VAL(zs), key._id);
        }
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Create write filter
    int8_t res = _z_write_filter_create(int_pub);
    if (res != _Z_RES_OK) {
        if (key._id != Z_RESOURCE_ID_NONE) {
            _z_undeclare_resource(&_Z_RC_IN_VAL(zs), key._id);
        }
        return res;
    }
    pub->_val = int_pub;
    return _Z_RES_OK;
}

int8_t z_undeclare_publisher(z_owned_publisher_t *pub) { return _z_publisher_drop(&pub->_val); }

void z_publisher_put_options_default(z_publisher_put_options_t *options) {
    options->encoding = z_encoding_default();
#if Z_FEATURE_ATTACHMENT == 1
    options->attachment = z_attachment_null();
#endif
}

void z_publisher_delete_options_default(z_publisher_delete_options_t *options) { options->__dummy = 0; }

int8_t z_publisher_put(const z_loaned_publisher_t *pub, const uint8_t *payload, size_t len,
                       const z_publisher_put_options_t *options) {
    int8_t ret = 0;
    // Build options
    z_publisher_put_options_t opt;
    z_publisher_put_options_default(&opt);
    if (options != NULL) {
        opt.encoding = options->encoding;
#if Z_FEATURE_ATTACHMENT == 1
        opt.attachment = options->attachment;
#endif
    }
    // Check if write filter is active before writing
    if (!_z_write_filter_active(pub)) {
        // Write value
        ret = _z_write(&pub->_zn.in->val, pub->_key, payload, len, opt.encoding, Z_SAMPLE_KIND_PUT,
                       pub->_congestion_control, pub->_priority
#if Z_FEATURE_ATTACHMENT == 1
                       ,
                       opt.attachment
#endif
        );
    }

#if Z_FEATURE_ATTACHMENT == 1
    z_attachment_t att = opt.attachment;
#else
    z_attachment_t att = z_attachment_null();
#endif
    // Trigger local subscriptions
    _z_trigger_local_subscriptions(&pub->_zn.in->val, pub->_key, payload, len, _Z_N_QOS_DEFAULT, att);
    return ret;
}

int8_t z_publisher_delete(const z_loaned_publisher_t *pub, const z_publisher_delete_options_t *options) {
    (void)(options);
    return _z_write(&pub->_zn.in->val, pub->_key, NULL, 0, z_encoding_default(), Z_SAMPLE_KIND_DELETE,
                    pub->_congestion_control, pub->_priority
#if Z_FEATURE_ATTACHMENT == 1
                    ,
                    z_attachment_null()
#endif
    );
}

z_owned_keyexpr_t z_publisher_keyexpr(z_loaned_publisher_t *publisher) {
    z_owned_keyexpr_t ret = {._val = z_malloc(sizeof(_z_keyexpr_t))};
    if (ret._val != NULL && publisher != NULL) {
        *ret._val = _z_keyexpr_duplicate(publisher->_key);
    }
    return ret;
}
#endif

#if Z_FEATURE_QUERY == 1
OWNED_FUNCTIONS_RC(reply)

void z_get_options_default(z_get_options_t *options) {
    options->target = z_query_target_default();
    options->consolidation = z_query_consolidation_default();
    options->encoding = z_encoding_default();
    options->payload = NULL;
#if Z_FEATURE_ATTACHMENT == 1
    options->attachment = z_attachment_null();
#endif
    options->timeout_ms = Z_GET_TIMEOUT_DEFAULT;
}

int8_t z_get(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, const char *parameters,
             z_owned_closure_reply_t *callback, z_get_options_t *options) {
    int8_t ret = _Z_RES_OK;

    void *ctx = callback->context;
    callback->context = NULL;

    z_get_options_t opt;
    z_get_options_default(&opt);
    if (options != NULL) {
        opt.consolidation = options->consolidation;
        opt.target = options->target;
        opt.encoding = options->encoding;
        opt.payload = z_bytes_move(options->payload);
#if Z_FEATURE_ATTACHMENT == 1
        opt.attachment = options->attachment;
#endif
    }

    if (opt.consolidation.mode == Z_CONSOLIDATION_MODE_AUTO) {
        const char *lp = (parameters == NULL) ? "" : parameters;
        if (strstr(lp, Z_SELECTOR_TIME) != NULL) {
            opt.consolidation.mode = Z_CONSOLIDATION_MODE_NONE;
        } else {
            opt.consolidation.mode = Z_CONSOLIDATION_MODE_LATEST;
        }
    }
    // Set value
    _z_value_t value = {.payload = _z_bytes_from_owned_bytes(opt.payload), .encoding = opt.encoding};

    ret = _z_query(&_Z_RC_IN_VAL(zs), *keyexpr, parameters, opt.target, opt.consolidation.mode, value, callback->call,
                   callback->drop, ctx, opt.timeout_ms
#if Z_FEATURE_ATTACHMENT == 1
                   ,
                   opt.attachment
#endif
    );
    if (opt.payload != NULL) {
        z_bytes_drop(opt.payload);
    }
    return ret;
}

_Bool z_reply_is_ok(const z_loaned_reply_t *reply) {
    _ZP_UNUSED(reply);
    // For the moment always return TRUE.
    // The support for reply errors will come in the next release.
    return true;
}

const z_loaned_sample_t *z_reply_ok(const z_loaned_reply_t *reply) { return &reply->in->val.data.sample; }

const z_loaned_value_t *z_reply_err(const z_loaned_reply_t *reply) {
    _ZP_UNUSED(reply);
    return NULL;
}
#endif

#if Z_FEATURE_QUERYABLE == 1
int8_t _z_queryable_drop(_z_queryable_t **queryable) {
    int8_t ret = _Z_RES_OK;

    ret = _z_undeclare_queryable(*queryable);
    _z_queryable_free(queryable);
    return ret;
}

OWNED_FUNCTIONS_RC(query)
OWNED_FUNCTIONS_PTR(_z_queryable_t, queryable, _z_owner_noop_copy, _z_queryable_drop)

void z_queryable_options_default(z_queryable_options_t *options) { options->complete = _Z_QUERYABLE_COMPLETE_DEFAULT; }

int8_t z_declare_queryable(z_owned_queryable_t *queryable, const z_loaned_session_t *zs,
                           const z_loaned_keyexpr_t *keyexpr, z_owned_closure_query_t *callback,
                           const z_queryable_options_t *options) {
    void *ctx = callback->context;
    callback->context = NULL;

    _z_keyexpr_t key = *keyexpr;

    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
    if (_Z_RC_IN_VAL(zs)._tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(&_Z_RC_IN_VAL(zs), keyexpr);
        if (r == NULL) {
            uint16_t id = _z_declare_resource(&_Z_RC_IN_VAL(zs), *keyexpr);
            key = _z_rid_with_suffix(id, NULL);
        }
    }

    z_queryable_options_t opt;
    z_queryable_options_default(&opt);
    if (options != NULL) {
        opt.complete = options->complete;
    }

    queryable->_val = _z_declare_queryable(zs, key, opt.complete, callback->call, callback->drop, ctx);

    return _Z_RES_OK;
}

int8_t z_undeclare_queryable(z_owned_queryable_t *queryable) { return _z_queryable_drop(&queryable->_val); }

void z_query_reply_options_default(z_query_reply_options_t *options) { options->encoding = z_encoding_default(); }

int8_t z_query_reply(const z_loaned_query_t *query, const z_loaned_keyexpr_t *keyexpr, z_owned_bytes_t *payload,
                     const z_query_reply_options_t *options) {
    z_query_reply_options_t opts;
    if (options == NULL) {
        z_query_reply_options_default(&opts);
    } else {
        opts = *options;
    }
    // Set value
    _z_value_t value = {.payload = _z_bytes_from_owned_bytes(payload), .encoding = opts.encoding};

    int8_t ret = _z_send_reply(&query->in->val, *keyexpr, value, Z_SAMPLE_KIND_PUT, opts.attachment);
    if (payload != NULL) {
        z_bytes_drop(payload);
    }
    return ret;
}
#endif

int8_t z_keyexpr_new(z_owned_keyexpr_t *key, const char *name) {
    int8_t ret = _Z_RES_OK;

    key->_val = name ? (_z_keyexpr_t *)z_malloc(sizeof(_z_keyexpr_t)) : NULL;
    if (key->_val != NULL) {
        *key->_val = _z_rid_with_suffix(Z_RESOURCE_ID_NONE, name);
    } else {
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    return ret;
}

int8_t z_declare_keyexpr(z_owned_keyexpr_t *key, const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr) {
    int8_t ret = _Z_RES_OK;

    key->_val = (_z_keyexpr_t *)z_malloc(sizeof(_z_keyexpr_t));
    if (key->_val != NULL) {
        uint16_t id = _z_declare_resource(&_Z_RC_IN_VAL(zs), *keyexpr);
        *key->_val = _z_rid_with_suffix(id, NULL);
    } else {
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    return ret;
}

int8_t z_undeclare_keyexpr(const z_loaned_session_t *zs, z_owned_keyexpr_t *keyexpr) {
    int8_t ret = _Z_RES_OK;

    ret = _z_undeclare_resource(&_Z_RC_IN_VAL(zs), keyexpr->_val->_id);
    z_keyexpr_drop(keyexpr);

    return ret;
}

#if Z_FEATURE_SUBSCRIPTION == 1
int8_t _z_subscriber_drop(_z_subscriber_t **sub) {
    int8_t ret = _Z_RES_OK;

    ret = _z_undeclare_subscriber(*sub);
    _z_subscriber_free(sub);

    return ret;
}

OWNED_FUNCTIONS_PTR(_z_subscriber_t, subscriber, _z_owner_noop_copy, _z_subscriber_drop)

void z_subscriber_options_default(z_subscriber_options_t *options) { options->reliability = Z_RELIABILITY_DEFAULT; }

int8_t z_declare_subscriber(z_owned_subscriber_t *sub, const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr,
                            z_owned_closure_sample_t *callback, const z_subscriber_options_t *options) {
    void *ctx = callback->context;
    callback->context = NULL;
    char *suffix = NULL;

    _z_keyexpr_t key = *keyexpr;
    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
    if (_Z_RC_IN_VAL(zs)._tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(&_Z_RC_IN_VAL(zs), keyexpr);
        if (r == NULL) {
            char *wild = strpbrk(keyexpr->_suffix, "*$");
            _Bool do_keydecl = true;
            if (wild != NULL && wild != keyexpr->_suffix) {
                wild -= 1;
                size_t len = wild - keyexpr->_suffix;
                suffix = z_malloc(len + 1);
                if (suffix != NULL) {
                    memcpy(suffix, keyexpr->_suffix, len);
                    suffix[len] = 0;
                    // TODO(sashacmc): Why we modify it? Rework to remove cast
                    ((z_loaned_keyexpr_t *)keyexpr)->_suffix = suffix;
                    _z_keyexpr_set_owns_suffix((z_loaned_keyexpr_t *)keyexpr, false);
                } else {
                    do_keydecl = false;
                }
            }
            if (do_keydecl) {
                uint16_t id = _z_declare_resource(&_Z_RC_IN_VAL(zs), *keyexpr);
                key = _z_rid_with_suffix(id, wild);
            }
        }
    }

    _z_subinfo_t subinfo = _z_subinfo_default();
    if (options != NULL) {
        subinfo.reliability = options->reliability;
    }
    _z_subscriber_t *int_sub = _z_declare_subscriber(zs, key, subinfo, callback->call, callback->drop, ctx);
    if (suffix != NULL) {
        z_free(suffix);
    }

    sub->_val = int_sub;

    if (int_sub == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    } else {
        return _Z_RES_OK;
    }
}

int8_t z_undeclare_subscriber(z_owned_subscriber_t *sub) { return _z_subscriber_drop(&sub->_val); }

z_owned_keyexpr_t z_subscriber_keyexpr(z_loaned_subscriber_t *sub) {
    z_owned_keyexpr_t ret;
    z_keyexpr_null(&ret);
    if (sub != NULL) {
        uint32_t lookup = sub->_entity_id;
        _z_subscription_rc_list_t *tail = sub->_zn.in->val._local_subscriptions;
        while (tail != NULL && !z_keyexpr_check(&ret)) {
            _z_subscription_rc_t *head = _z_subscription_rc_list_head(tail);
            if (head->in->val._id == lookup) {
                _z_keyexpr_t key = _z_keyexpr_duplicate(head->in->val._key);
                ret = (z_owned_keyexpr_t){._val = z_malloc(sizeof(_z_keyexpr_t))};
                if (ret._val != NULL) {
                    *ret._val = key;
                } else {
                    _z_keyexpr_clear(&key);
                }
            }
            tail = _z_subscription_rc_list_tail(tail);
        }
    }
    return ret;
}
#endif

/**************** Tasks ****************/
void zp_task_read_options_default(zp_task_read_options_t *options) {
#if Z_FEATURE_MULTI_THREAD == 1
    options->task_attributes = NULL;
#else
    options->__dummy = 0;
#endif
}

int8_t zp_start_read_task(z_loaned_session_t *zs, const zp_task_read_options_t *options) {
    (void)(options);
#if Z_FEATURE_MULTI_THREAD == 1
    zp_task_read_options_t opt;
    zp_task_read_options_default(&opt);
    if (options != NULL) {
        opt.task_attributes = options->task_attributes;
    }
    return _zp_start_read_task(&_Z_RC_IN_VAL(zs), opt.task_attributes);
#else
    (void)(zs);
    return -1;
#endif
}

int8_t zp_stop_read_task(z_loaned_session_t *zs) {
#if Z_FEATURE_MULTI_THREAD == 1
    return _zp_stop_read_task(&_Z_RC_IN_VAL(zs));
#else
    (void)(zs);
    return -1;
#endif
}

void zp_task_lease_options_default(zp_task_lease_options_t *options) {
#if Z_FEATURE_MULTI_THREAD == 1
    options->task_attributes = NULL;
#else
    options->__dummy = 0;
#endif
}

int8_t zp_start_lease_task(z_loaned_session_t *zs, const zp_task_lease_options_t *options) {
    (void)(options);
#if Z_FEATURE_MULTI_THREAD == 1
    zp_task_lease_options_t opt;
    zp_task_lease_options_default(&opt);
    if (options != NULL) {
        opt.task_attributes = options->task_attributes;
    }
    return _zp_start_lease_task(&_Z_RC_IN_VAL(zs), opt.task_attributes);
#else
    (void)(zs);
    return -1;
#endif
}

int8_t zp_stop_lease_task(z_loaned_session_t *zs) {
#if Z_FEATURE_MULTI_THREAD == 1
    return _zp_stop_lease_task(&_Z_RC_IN_VAL(zs));
#else
    (void)(zs);
    return -1;
#endif
}

void zp_read_options_default(zp_read_options_t *options) { options->__dummy = 0; }

int8_t zp_read(const z_loaned_session_t *zs, const zp_read_options_t *options) {
    (void)(options);
    return _zp_read(&_Z_RC_IN_VAL(zs));
}

void zp_send_keep_alive_options_default(zp_send_keep_alive_options_t *options) { options->__dummy = 0; }

int8_t zp_send_keep_alive(const z_loaned_session_t *zs, const zp_send_keep_alive_options_t *options) {
    (void)(options);
    return _zp_send_keep_alive(&_Z_RC_IN_VAL(zs));
}

void zp_send_join_options_default(zp_send_join_options_t *options) { options->__dummy = 0; }

int8_t zp_send_join(const z_loaned_session_t *zs, const zp_send_join_options_t *options) {
    (void)(options);
    return _zp_send_join(&_Z_RC_IN_VAL(zs));
}
#if Z_FEATURE_ATTACHMENT == 1
void _z_bytes_pair_clear(struct _z_bytes_pair_t *this_) {
    _z_bytes_clear(&this_->key);
    _z_bytes_clear(&this_->value);
}

z_attachment_t z_bytes_map_as_attachment(const z_owned_bytes_map_t *this_) {
    if (!z_bytes_map_check(this_)) {
        return z_attachment_null();
    }
    return (z_attachment_t){.data = this_, .iteration_driver = (z_attachment_iter_driver_t)z_bytes_map_iter};
}
bool z_bytes_map_check(const z_owned_bytes_map_t *this_) { return this_->_inner != NULL; }
void z_bytes_map_drop(z_owned_bytes_map_t *this_) { _z_bytes_pair_list_free(&this_->_inner); }

int8_t _z_bytes_map_insert_by_alias(z_loaned_bytes_t *key, z_loaned_bytes_t *value, void *this_) {
    z_bytes_map_insert_by_alias((z_owned_bytes_map_t *)this_, key, value);
    return 0;
}
int8_t _z_bytes_map_insert_by_copy(z_loaned_bytes_t *key, z_loaned_bytes_t *value, void *this_) {
    z_bytes_map_insert_by_copy((z_owned_bytes_map_t *)this_, key, value);
    return 0;
}
z_owned_bytes_map_t z_bytes_map_from_attachment(z_attachment_t this_) {
    if (!z_attachment_check(&this_)) {
        return z_bytes_map_null();
    }
    z_owned_bytes_map_t map = z_bytes_map_new();
    z_attachment_iterate(this_, _z_bytes_map_insert_by_copy, &map);
    return map;
}
z_owned_bytes_map_t z_bytes_map_from_attachment_aliasing(z_attachment_t this_) {
    if (!z_attachment_check(&this_)) {
        return z_bytes_map_null();
    }
    z_owned_bytes_map_t map = z_bytes_map_new();
    z_attachment_iterate(this_, _z_bytes_map_insert_by_alias, &map);
    return map;
}
z_loaned_bytes_t *z_bytes_map_get(const z_owned_bytes_map_t *this_, z_loaned_bytes_t *key) {
    _z_bytes_pair_list_t *current = this_->_inner;
    while (current) {
        struct _z_bytes_pair_t *head = _z_bytes_pair_list_head(current);
        if (_z_bytes_eq(&key, &head->key)) {
            return _z_bytes_wrap(head->value.start, head->value.len);
        }
    }
    return z_bytes_null();
}
void z_bytes_map_insert_by_alias(const z_owned_bytes_map_t *this_, z_loaned_bytes_t *key, z_loaned_bytes_t *value) {
    _z_bytes_pair_list_t *current = this_->_inner;
    while (current) {
        struct _z_bytes_pair_t *head = _z_bytes_pair_list_head(current);
        if (_z_bytes_eq(&key, &head->key)) {
            break;
        }
        current = _z_bytes_pair_list_tail(current);
    }
    if (current) {
        struct _z_bytes_pair_t *head = _z_bytes_pair_list_head(current);
        _z_bytes_clear(&head->value);
        head->value = _z_bytes_wrap(value.start, value.len);
    } else {
        struct _z_bytes_pair_t *insert = z_malloc(sizeof(struct _z_bytes_pair_t));
        memset(insert, 0, sizeof(struct _z_bytes_pair_t));
        insert->key = _z_bytes_wrap(key.start, key.len);
        insert->value = _z_bytes_wrap(value.start, value.len);
        ((z_owned_bytes_map_t *)this_)->_inner = _z_bytes_pair_list_push(this_->_inner, insert);
    }
}
void z_bytes_map_insert_by_copy(const z_owned_bytes_map_t *this_, z_loaned_bytes_t *key, z_loaned_bytes_t *value) {
    _z_bytes_pair_list_t *current = this_->_inner;
    while (current) {
        struct _z_bytes_pair_t *head = _z_bytes_pair_list_head(current);
        if (_z_bytes_eq(&key, &head->key)) {
            break;
        }
        current = _z_bytes_pair_list_tail(current);
    }
    if (current) {
        struct _z_bytes_pair_t *head = _z_bytes_pair_list_head(current);
        _z_bytes_clear(&head->value);
        _z_bytes_copy(&head->value, &value);
        if (!head->key._is_alloc) {
            _z_bytes_copy(&head->key, &key);
        }
    } else {
        struct _z_bytes_pair_t *insert = z_malloc(sizeof(struct _z_bytes_pair_t));
        memset(insert, 0, sizeof(struct _z_bytes_pair_t));
        _z_bytes_copy(&insert->key, &key);
        _z_bytes_copy(&insert->value, &value);
        ((z_owned_bytes_map_t *)this_)->_inner = _z_bytes_pair_list_push(this_->_inner, insert);
    }
}
int8_t z_bytes_map_iter(const z_owned_bytes_map_t *this_, z_attachment_iter_body_t body, void *ctx) {
    _z_bytes_pair_list_t *current = this_->_inner;
    while (current) {
        struct _z_bytes_pair_t *head = _z_bytes_pair_list_head(current);
        int8_t ret = body(head->key, head->value, ctx);
        if (ret) {
            return ret;
        }
        current = _z_bytes_pair_list_tail(current);
    }
    return 0;
}
z_owned_bytes_map_t z_bytes_map_new(void) { return (z_owned_bytes_map_t){._inner = _z_bytes_pair_list_new()}; }
z_owned_bytes_map_t z_bytes_map_null(void) { return (z_owned_bytes_map_t){._inner = NULL}; }

int8_t z_bytes_from_str(z_owned_bytes_t *bytes const char *str) {
    bytes->_val = z_bytes_wrap((const uint8_t *)str, strlen(str));
    return _Z_RES_OK;
}
#endif
