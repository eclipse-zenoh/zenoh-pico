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

#include <stdbool.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/net/config.h"
#include "zenoh-pico/net/logger.h"
#include "zenoh-pico/net/memory.h"
#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/net/resource.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/utils.h"

/*************** Logging ***************/
_Bool z_init_logger(void) {
    _z_init_logger();
    return true;
}

/********* Data Types Handlers *********/
z_keyexpr_t z_keyexpr(const char *name) { return _z_rname(name); }

char *z_keyexpr_to_string(z_keyexpr_t keyexpr) {
    if (keyexpr._id != Z_RESOURCE_ID_NONE) return NULL;

    char *ret = (char *)malloc(strlen(keyexpr._suffix) + 1);
    strcpy(ret, keyexpr._suffix);

    return ret;
}

char *zp_keyexpr_resolve(z_session_t *zs, z_keyexpr_t keyexpr) {
    _z_keyexpr_t ekey = _z_get_expanded_key_from_key(zs, _Z_RESOURCE_IS_LOCAL, &keyexpr);
    return (char *)ekey._suffix;  // ekey will be out of scope so, suffix can be safely casted as non-const
}

_Bool z_keyexpr_is_valid(z_keyexpr_t *keyexpr) {
    if (keyexpr->_id != Z_RESOURCE_ID_NONE || keyexpr->_suffix != NULL) return true;

    return false;
}

z_keyexpr_canon_status_t z_keyexpr_is_canon(const char *start, size_t len) { return _z_keyexpr_is_canon(start, len); }

z_keyexpr_canon_status_t zp_keyexpr_is_canon_null_terminated(const char *start) {
    return _z_keyexpr_is_canon(start, strlen(start));
}

z_keyexpr_canon_status_t z_keyexpr_canonize(char *start, size_t *len) { return _z_keyexpr_canonize(start, len); }

z_keyexpr_canon_status_t zp_keyexpr_canonize_null_terminated(char *start) {
    size_t len = strlen(start);
    size_t newlen = len;
    z_keyexpr_canon_status_t result = _z_keyexpr_canonize(start, &newlen);
    if (newlen < len) {
        start[newlen] = '\0';
    }
    return result;
}

_Bool z_keyexpr_includes(const char *l, size_t llen, const char *r, size_t rlen) {
    return _z_keyexpr_includes(l, llen, r, rlen);
}

_Bool zp_keyexpr_includes_null_terminated(const char *l, const char *r) {
    return _z_keyexpr_includes(l, strlen(l), r, strlen(r));
}

_Bool z_keyexpr_intersect(const char *l, size_t llen, const char *r, size_t rlen) {
    return _z_keyexpr_intersect(l, llen, r, rlen);
}

_Bool zp_keyexpr_intersect_null_terminated(const char *l, const char *r) {
    return _z_keyexpr_intersect(l, strlen(l), r, strlen(r));
}

_Bool z_keyexpr_equals(const char *l, size_t llen, const char *r, size_t rlen) {
    if (llen != rlen) {
        return false;
    }

    if (strncmp(l, r, llen) != 0) {
        return false;
    }

    return true;
}

_Bool zp_keyexpr_equals_null_terminated(const char *l, const char *r) {
    if (strcmp(l, r) != 0) {
        return false;
    }

    return true;
}

z_owned_config_t zp_config_new(void) { return (z_owned_config_t){._value = _z_config_empty()}; }

z_owned_config_t zp_config_empty(void) { return (z_owned_config_t){._value = _z_config_empty()}; }

z_owned_config_t zp_config_default(void) { return (z_owned_config_t){._value = _z_config_default()}; }

const char *zp_config_get(z_config_t *config, unsigned int key) { return _z_config_get(config, key); }

int8_t zp_config_insert(z_config_t *config, unsigned int key, z_string_t value) {
    return _zp_config_insert(config, key, value);
}

z_encoding_t z_encoding_default(void) {
    return (_z_encoding_t){.prefix = Z_ENCODING_PREFIX_EMPTY, .suffix = _z_bytes_make(0)};
}

z_query_target_t z_query_target_default(void) { return Z_QUERY_TARGET_BEST_MATCHING; }

z_query_consolidation_t z_query_consolidation_auto(void) {
    return (z_query_consolidation_t){.tag = Z_QUERY_CONSOLIDATION_AUTO};
}

z_query_consolidation_t z_query_consolidation_latest(void) {
    return (z_query_consolidation_t){.tag = Z_QUERY_CONSOLIDATION_MANUAL, .mode = Z_CONSOLIDATION_MODE_LATEST};
}

z_query_consolidation_t z_query_consolidation_monotonic(void) {
    return (z_query_consolidation_t){.tag = Z_QUERY_CONSOLIDATION_MANUAL, .mode = Z_CONSOLIDATION_MODE_MONOTONIC};
}

z_query_consolidation_t z_query_consolidation_none(void) {
    return (z_query_consolidation_t){.tag = Z_QUERY_CONSOLIDATION_MANUAL, .mode = Z_CONSOLIDATION_MODE_NONE};
}

z_query_consolidation_t z_query_consolidation_default(void) { return z_query_consolidation_auto(); }

z_bytes_t z_query_value_selector(z_query_t *query) {
    z_bytes_t value_selector = _z_bytes_wrap((uint8_t *)query->_value_selector, strlen(query->_value_selector));
    return value_selector;
}

z_keyexpr_t z_query_keyexpr(z_query_t *query) { return query->_key; }

/**************** Loans ****************/
#define _MUTABLE_OWNED_FUNCTIONS_DEFINITION(type, ownedtype, name, f_free, f_copy) \
    _Bool z_##name##_check(const ownedtype *val) { return val->_value != NULL; }   \
    type *z_##name##_loan(const ownedtype *val) { return val->_value; }            \
    ownedtype *z_##name##_move(ownedtype *val) { return val; }                     \
    ownedtype z_##name##_clone(ownedtype *val) {                                   \
        ownedtype ret;                                                             \
        ret._value = (type *)z_malloc(sizeof(type));                               \
        f_copy(ret._value, val->_value);                                           \
        return ret;                                                                \
    }                                                                              \
    void z_##name##_drop(ownedtype *val) { f_free(&val->_value); }

#define _IMMUTABLE_OWNED_FUNCTIONS_DEFINITION(type, ownedtype, name, f_free, f_copy) \
    _Bool z_##name##_check(const ownedtype *val) { return val->_value != NULL; }     \
    type z_##name##_loan(const ownedtype *val) { return *val->_value; }              \
    ownedtype *z_##name##_move(ownedtype *val) { return val; }                       \
    ownedtype z_##name##_clone(ownedtype *val) {                                     \
        ownedtype ret;                                                               \
        ret._value = (type *)z_malloc(sizeof(type));                                 \
        f_copy(ret._value, val->_value);                                             \
        return ret;                                                                  \
    }                                                                                \
    void z_##name##_drop(ownedtype *val) { f_free(&val->_value); }

static inline void _z_owner_noop_free(void *s) { (void)(s); }

static inline void _z_owner_noop_copy(void *dst, const void *src) {
    (void)(dst);
    (void)(src);
}

_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_bytes_t, z_owned_bytes_t, bytes, _z_bytes_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_string_t, z_owned_string_t, string, _z_string_free, _z_owner_noop_copy)
_IMMUTABLE_OWNED_FUNCTIONS_DEFINITION(z_keyexpr_t, z_owned_keyexpr_t, keyexpr, _z_keyexpr_free, _z_keyexpr_copy)

_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_config_t, z_owned_config_t, config, _z_config_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_session_t, z_owned_session_t, session, _z_session_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_pull_subscriber_t, z_owned_pull_subscriber_t, pull_subscriber, _z_owner_noop_free,
                                    _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_subscriber_t, z_owned_subscriber_t, subscriber, _z_owner_noop_free,
                                    _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_publisher_t, z_owned_publisher_t, publisher, _z_publisher_free,
                                    _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_queryable_t, z_owned_queryable_t, queryable, _z_owner_noop_free,
                                    _z_owner_noop_copy)

_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_encoding_t, z_owned_encoding_t, encoding, _z_owner_noop_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_query_target_t, z_owned_query_target_t, query_target, _z_owner_noop_free,
                                    _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_query_consolidation_t, z_owned_query_consolidation_t, query_consolidation,
                                    _z_owner_noop_free, _z_owner_noop_copy)

_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_sample_t, z_owned_sample_t, sample, _z_sample_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_hello_t, z_owned_hello_t, hello, _z_hello_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_reply_t, z_owned_reply_t, reply, _z_owner_noop_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_reply_data_t, z_owned_reply_data_t, reply_data, _z_reply_data_free,
                                    _z_owner_noop_copy)

_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_str_array_t, z_owned_str_array_t, str_array, _z_str_array_free,
                                    _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_hello_array_t, z_owned_hello_array_t, hello_array, _z_hello_array_free,
                                    _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_reply_data_array_t, z_owned_reply_data_array_t, reply_data_array,
                                    _z_reply_data_array_free, _z_owner_noop_copy)

z_owned_closure_sample_t z_closure_sample(_z_data_handler_t call, _z_dropper_handler_t drop, void *context) {
    return (z_owned_closure_sample_t){.call = call, .drop = drop, .context = context};
}

z_owned_closure_query_t z_closure_query(_z_questionable_handler_t call, _z_dropper_handler_t drop, void *context) {
    return (z_owned_closure_query_t){.call = call, .drop = drop, .context = context};
}

z_owned_closure_reply_t z_closure_reply(z_owned_reply_handler_t call, _z_dropper_handler_t drop, void *context) {
    return (z_owned_closure_reply_t){.call = call, .drop = drop, .context = context};
}

z_owned_closure_zid_t z_closure_zid(z_id_handler_t call, _z_dropper_handler_t drop, void *context) {
    return (z_owned_closure_zid_t){.call = call, .drop = drop, .context = context};
}

z_owned_closure_sample_t *z_closure_sample_move(z_owned_closure_sample_t *closure_sample) { return closure_sample; }

z_owned_closure_query_t *z_closure_query_move(z_owned_closure_query_t *closure_query) { return closure_query; }

z_owned_closure_reply_t *z_closure_reply_move(z_owned_closure_reply_t *closure_reply) { return closure_reply; }

z_owned_closure_zid_t *z_closure_zid_move(z_owned_closure_zid_t *closure_zid) { return closure_zid; }

/************* Primitives **************/
z_owned_hello_array_t z_scout(z_zint_t what, z_owned_config_t *config, uint32_t timeout) {
    z_owned_hello_array_t hellos;
    hellos._value = (z_hello_array_t *)z_malloc(sizeof(z_hello_array_t));
    *hellos._value = _z_scout(what, config->_value, timeout);

    z_config_drop(config);
    config->_value = NULL;

    return hellos;
}

z_owned_session_t z_open(z_owned_config_t *config) {
    z_owned_session_t zs;
    zs._value = _z_open(config->_value);

    z_config_drop(config);
    config->_value = NULL;

    return zs;
}

int8_t z_close(z_owned_session_t *zs) {
    _z_close(zs->_value);

    z_session_drop(zs);
    zs->_value = NULL;

    return 0;
}

void z_info_peers_zid(const z_session_t *zs, z_owned_closure_zid_t *callback) {
    void *ctx = callback->context;
    callback->context = NULL;

    z_id_t id;
#if Z_MULTICAST_TRANSPORT == 1
    if (zs->_tp->_type == _Z_TRANSPORT_MULTICAST_TYPE) {
        _z_transport_peer_entry_list_t *l = zs->_tp->_transport._multicast._peers;
        for (; l != NULL; l = _z_transport_peer_entry_list_tail(l)) {
            _z_transport_peer_entry_t *val = _z_transport_peer_entry_list_head(l);
            memcpy(&id.id[0], val->_remote_pid.start, val->_remote_pid.len);
            memset(&id.id[val->_remote_pid.len], 0, sizeof(id) - val->_remote_pid.len);

            callback->call(&id, ctx);
        }
    }
#endif  // Z_MULTICAST_TRANSPORT == 1

    if (callback->drop != NULL) callback->drop(ctx);
}

void z_info_routers_zid(const z_session_t *zs, z_owned_closure_zid_t *callback) {
    void *ctx = callback->context;
    callback->context = NULL;

    z_id_t id;
#if Z_UNICAST_TRANSPORT == 1
    if (zs->_tp->_type == _Z_TRANSPORT_UNICAST_TYPE) {
        memcpy(&id.id[0], zs->_tp->_transport._unicast._remote_pid.start, zs->_tp->_transport._unicast._remote_pid.len);
        memset(&id.id[zs->_tp->_transport._unicast._remote_pid.len], 0,
               sizeof(id) - zs->_tp->_transport._unicast._remote_pid.len);

        callback->call(&id, ctx);
    }
#endif  // Z_UNICAST_TRANSPORT == 1

    if (callback->drop != NULL) callback->drop(ctx);
}

z_id_t z_info_zid(const z_session_t *zs) {
    z_id_t id;
    memcpy(&id.id[0], zs->_tp_manager->_local_pid.start, zs->_tp_manager->_local_pid.len);
    memset(&id.id[zs->_tp_manager->_local_pid.len], 0, sizeof(id) - zs->_tp_manager->_local_pid.len);

    return id;
}

z_put_options_t z_put_options_default(void) {
    return (z_put_options_t){
        .encoding = z_encoding_default(), .congestion_control = Z_CONGESTION_CONTROL_DROP, .priority = Z_PRIORITY_DATA};
}

z_delete_options_t z_delete_options_default(void) {
    return (z_delete_options_t){.congestion_control = Z_CONGESTION_CONTROL_DROP};
}

int8_t z_put(z_session_t *zs, z_keyexpr_t keyexpr, const uint8_t *payload, z_zint_t payload_len,
             const z_put_options_t *options) {
    if (options != NULL)
        return _z_write_ext(zs, keyexpr, (const uint8_t *)payload, payload_len, options->encoding, Z_SAMPLE_KIND_PUT,
                            options->congestion_control);

    return _z_write_ext(zs, keyexpr, (const uint8_t *)payload, payload_len, z_encoding_default(), Z_SAMPLE_KIND_PUT,
                        Z_CONGESTION_CONTROL_DROP);
}

int8_t z_delete(z_session_t *zs, z_keyexpr_t keyexpr, const z_delete_options_t *options) {
    if (options != NULL)
        return _z_write_ext(zs, keyexpr, NULL, 0, z_encoding_default(), Z_SAMPLE_KIND_DELETE,
                            options->congestion_control);

    return _z_write_ext(zs, keyexpr, NULL, 0, z_encoding_default(), Z_SAMPLE_KIND_DELETE, Z_CONGESTION_CONTROL_DROP);
}

z_get_options_t z_get_options_default(void) {
    return (z_get_options_t){.target = z_query_target_default(), .consolidation = z_query_consolidation_default()};
}

typedef struct {
    z_owned_reply_handler_t user_call;
    void *ctx;
} __z_reply_handler_wrapper_t;

void __z_reply_handler(_z_reply_t *reply, void *arg) {
    z_owned_reply_t oreply = {._value = reply};

    __z_reply_handler_wrapper_t *wrapped_ctx = (__z_reply_handler_wrapper_t *)arg;
    wrapped_ctx->user_call(oreply, wrapped_ctx->ctx);
}

int8_t z_get(z_session_t *zs, z_keyexpr_t keyexpr, const char *value_selector, z_owned_closure_reply_t *callback,
             const z_get_options_t *options) {
    void *ctx = callback->context;
    callback->context = NULL;

    // Default consolidation is full
    z_query_consolidation_t consolidation = z_query_consolidation_default();
    _z_target_t target = {._kind = Z_QUERYABLE_ALL_KINDS, ._target = z_query_target_default()};

    if (options != NULL) {
        consolidation = options->consolidation;
        target._target = options->target;
    }

    if (consolidation.tag == Z_QUERY_CONSOLIDATION_AUTO) {
        if (strstr(value_selector, Z_SELECTOR_TIME) != NULL)
            consolidation = z_query_consolidation_none();
        else
            consolidation = z_query_consolidation_latest();
    }

    // TODO[API-NET]: When API and NET are a single layer, there is no wrap the user callback and args
    //                to enclose the z_reply_t into a z_owned_reply_t.
    __z_reply_handler_wrapper_t *wrapped_ctx =
        (__z_reply_handler_wrapper_t *)z_malloc(sizeof(__z_reply_handler_wrapper_t));
    wrapped_ctx->user_call = callback->call;
    wrapped_ctx->ctx = ctx;

    return _z_query(zs, keyexpr, value_selector, target, consolidation.mode, __z_reply_handler, wrapped_ctx,
                    callback->drop, ctx);
}

z_owned_keyexpr_t z_declare_keyexpr(z_session_t *zs, z_keyexpr_t keyexpr) {
    z_owned_keyexpr_t key;
    key._value = (z_keyexpr_t *)z_malloc(sizeof(z_keyexpr_t));
    *key._value = _z_rid_with_suffix(_z_declare_resource(zs, keyexpr), NULL);

    return key;
}

int8_t z_undeclare_keyexpr(z_session_t *zs, z_owned_keyexpr_t *keyexpr) {
    _z_undeclare_resource(zs, keyexpr->_value->_id);

    z_keyexpr_drop(keyexpr);
    keyexpr->_value = NULL;

    return 0;
}

z_publisher_options_t z_publisher_options_default(void) {
    return (z_publisher_options_t){
        .local_routing = -1, .congestion_control = Z_CONGESTION_CONTROL_DROP, .priority = Z_PRIORITY_DATA_HIGH};
}

z_owned_publisher_t z_declare_publisher(z_session_t *zs, z_keyexpr_t keyexpr, z_publisher_options_t *options) {
    z_keyexpr_t key = keyexpr;

    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
#if Z_MULTICAST_TRANSPORT == 1
    if (zs->_tp->_type != _Z_TRANSPORT_MULTICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(zs, _Z_RESOURCE_IS_LOCAL, &keyexpr);
        if (r == NULL) {
            key = _z_rid_with_suffix(_z_declare_resource(zs, keyexpr), NULL);
        }
    }
#endif  // Z_MULTICAST_TRANSPORT == 1

    if (options != NULL) {
        return (z_owned_publisher_t){._value = _z_declare_publisher(zs, key, options->local_routing,
                                                                    options->congestion_control, options->priority)};
    }

    z_publisher_options_t opt = z_publisher_options_default();
    return (z_owned_publisher_t){
        ._value = _z_declare_publisher(zs, key, opt.local_routing, opt.congestion_control, opt.priority)};
}

int8_t z_undeclare_publisher(z_owned_publisher_t *pub) {
    _z_undeclare_publisher(pub->_value);

    z_publisher_drop(pub);
    z_free(pub->_value);
    pub->_value = NULL;

    return 0;
}

z_publisher_put_options_t z_publisher_put_options_default(void) {
    return (z_publisher_put_options_t){.encoding = z_encoding_default()};
}

z_publisher_delete_options_t z_publisher_delete_options_default(void) { return (z_publisher_delete_options_t){}; }

int8_t z_publisher_put(const z_publisher_t *pub, const uint8_t *payload, size_t len,
                       const z_publisher_put_options_t *options) {
    if (options != NULL)
        return _z_write_ext(pub->_zn, pub->_key, payload, len, options->encoding, Z_SAMPLE_KIND_PUT,
                            pub->_congestion_control);

    return _z_write_ext(pub->_zn, pub->_key, payload, len, z_encoding_default(), Z_SAMPLE_KIND_PUT,
                        pub->_congestion_control);
}

int8_t z_publisher_delete(const z_publisher_t *pub, const z_publisher_delete_options_t *options) {
    (void)(options);
    return _z_write_ext(pub->_zn, pub->_key, NULL, 0, z_encoding_default(), Z_SAMPLE_KIND_DELETE,
                        pub->_congestion_control);
}

z_subscriber_options_t z_subscriber_options_default(void) {
    return (z_subscriber_options_t){.reliability = Z_RELIABILITY_RELIABLE};
}

z_pull_subscriber_options_t z_pull_subscriber_options_default(void) {
    return (z_pull_subscriber_options_t){.reliability = Z_RELIABILITY_RELIABLE};
}

z_owned_subscriber_t z_declare_subscriber(z_session_t *zs, z_keyexpr_t keyexpr, z_owned_closure_sample_t *callback,
                                          const z_subscriber_options_t *options) {
    void *ctx = callback->context;
    callback->context = NULL;

    z_keyexpr_t key = keyexpr;
    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
#if Z_MULTICAST_TRANSPORT == 1
    if (zs->_tp->_type != _Z_TRANSPORT_MULTICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(zs, _Z_RESOURCE_IS_LOCAL, &keyexpr);
        if (r == NULL) key = _z_rid_with_suffix(_z_declare_resource(zs, keyexpr), NULL);
    }
#endif  // Z_MULTICAST_TRANSPORT == 1

    _z_subinfo_t subinfo = _z_subinfo_push_default();
    if (options != NULL) subinfo.reliability = options->reliability;

    return (z_owned_subscriber_t){._value =
                                      _z_declare_subscriber(zs, key, subinfo, callback->call, callback->drop, ctx)};
}

z_owned_pull_subscriber_t z_declare_pull_subscriber(z_session_t *zs, z_keyexpr_t keyexpr,
                                                    z_owned_closure_sample_t *callback,
                                                    const z_pull_subscriber_options_t *options) {
    (void)(options);

    void *ctx = callback->context;
    callback->context = NULL;

    z_keyexpr_t key = keyexpr;
    _z_resource_t *r = _z_get_resource_by_key(zs, _Z_RESOURCE_IS_LOCAL, &keyexpr);
    if (r == NULL) key = _z_rid_with_suffix(_z_declare_resource(zs, keyexpr), NULL);

    _z_subinfo_t subinfo = _z_subinfo_pull_default();
    if (options != NULL) subinfo.reliability = options->reliability;

    return (z_owned_pull_subscriber_t){
        ._value = _z_declare_subscriber(zs, key, subinfo, callback->call, callback->drop, ctx)};
}

int8_t z_undeclare_subscriber(z_owned_subscriber_t *sub) {
    _z_undeclare_subscriber(sub->_value);

    z_subscriber_drop(sub);
    z_free(sub->_value);
    sub->_value = NULL;

    return 0;
}

int8_t z_undeclare_pull_subscriber(z_owned_pull_subscriber_t *sub) {
    _z_undeclare_subscriber(sub->_value);

    z_pull_subscriber_drop(sub);
    z_free(sub->_value);
    sub->_value = NULL;

    return 0;
}

int8_t z_pull(const z_pull_subscriber_t *sub) { return _z_pull(sub); }

z_queryable_options_t z_queryable_options_default(void) {
    return (z_queryable_options_t){.complete = _Z_QUERYABLE_COMPLETE_DEFAULT};
}

z_owned_queryable_t z_declare_queryable(z_session_t *zs, z_keyexpr_t keyexpr, z_owned_closure_query_t *callback,
                                        const z_queryable_options_t *options) {
    void *ctx = callback->context;
    callback->context = NULL;

    z_keyexpr_t key = keyexpr;
    _z_resource_t *r = _z_get_resource_by_key(zs, _Z_RESOURCE_IS_LOCAL, &keyexpr);
    if (r == NULL) key = _z_rid_with_suffix(_z_declare_resource(zs, keyexpr), NULL);

    if (options != NULL)
        return (z_owned_queryable_t){
            ._value = _z_declare_queryable(zs, key, options->complete, callback->call, callback->drop, ctx)};

    z_queryable_options_t opt = z_queryable_options_default();
    return (z_owned_queryable_t){._value =
                                     _z_declare_queryable(zs, key, opt.complete, callback->call, callback->drop, ctx)};
}

int8_t z_undeclare_queryable(z_owned_queryable_t *queryable) {
    _z_undeclare_queryable(queryable->_value);

    z_queryable_drop(queryable);
    z_free(queryable->_value);
    queryable->_value = NULL;

    return 0;
}

z_query_reply_options_t z_query_reply_options_default(void) { return (z_query_reply_options_t){}; }

int8_t z_query_reply(const z_query_t *query, const z_keyexpr_t keyexpr, const uint8_t *payload, size_t payload_len,
                     const z_query_reply_options_t *options) {
    (void)(options);
    return _z_send_reply(query, keyexpr, payload, payload_len);
}

_Bool z_reply_is_ok(const z_owned_reply_t *reply) {
    (void)(reply);
    // For the moment always return TRUE.
    // The support for reply errors will come in the next release.
    return true;
}

z_value_t z_reply_err(const z_owned_reply_t *reply) {
    (void)(reply);
    return (z_value_t){.payload = _z_bytes_make(0), .encoding = z_encoding_default()};
}

z_sample_t z_reply_ok(z_owned_reply_t *reply) { return reply->_value->data.sample; }

/**************** Tasks ****************/
int8_t zp_start_read_task(z_session_t *zs) {
#if Z_MULTI_THREAD == 1
    return _zp_start_read_task(zs);
#else
    (void)(zs);
    return -1;
#endif
}

int8_t zp_stop_read_task(z_session_t *zs) {
#if Z_MULTI_THREAD == 1
    return _zp_stop_read_task(zs);
#else
    (void)(zs);
    return -1;
#endif
}

int8_t zp_start_lease_task(z_session_t *zs) {
#if Z_MULTI_THREAD == 1
    return _zp_start_lease_task(zs);
#else
    (void)(zs);
    return -1;
#endif
}

int8_t zp_stop_lease_task(z_session_t *zs) {
#if Z_MULTI_THREAD == 1
    return _zp_stop_lease_task(zs);
#else
    (void)(zs);
    return -1;
#endif
}

int8_t zp_read(z_session_t *zs) { return _zp_read(zs); }

int8_t zp_send_keep_alive(z_session_t *zs) { return _zp_send_keep_alive(zs); }
