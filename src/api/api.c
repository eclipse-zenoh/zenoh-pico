/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include "zenoh-pico/api/primitives.h"

#include "zenoh-pico/net/config.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/net/resource.h"
#include "zenoh-pico/net/memory.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/protocol/utils.h"

/*************** Logging ***************/
int8_t z_init_logger(void)
{
    // TODO
    return 0;
}

/********* Data Types Handlers *********/
z_owned_bytes_t z_bytes_new(const uint8_t *start, z_zint_t len)
{
    z_owned_bytes_t ret;
    ret._value = (z_bytes_t*)malloc(sizeof(z_bytes_t));
    ret._value->start = start;
    ret._value->len = len;
    ret._value->_is_alloc = 0;

    return ret;
}

z_owned_string_t z_string_new(const char *s)
{
    z_owned_string_t ret;
    ret._value = (z_string_t*)malloc(sizeof(z_string_t));
    ret._value->val = (char *)s; // FIXME
    ret._value->len = strlen(s);

    return ret;
}

z_keyexpr_t z_keyexpr(const char *name)
{
    return _z_rname(name);
}

z_owned_config_t zp_config_new(void)
{
    return (z_owned_config_t){._value = _z_config_empty()};
}

z_owned_config_t zp_config_empty(void)
{
    return (z_owned_config_t){._value = _z_config_empty()};
}

z_owned_config_t zp_config_default(void)
{
    return (z_owned_config_t){._value = _z_config_default()};
}

z_owned_config_t zp_config_client(const char *const *peers, size_t n_peers)
{
    (void) (n_peers);
    return (z_owned_config_t){._value = _z_config_client(peers[0])};
}

z_owned_config_t zp_config_peer(void)
{
    // Not implemented yet in Zenoh-Pico
    return (z_owned_config_t){._value = NULL};
}

z_owned_config_t zp_config_from_file(const char *path)
{
    // Not implemented yet in Zenoh-Pico
    (void) (path);
    return (z_owned_config_t){._value = NULL};
}

z_owned_config_t zp_config_from_str(const char *str)
{
    // Not implemented yet in Zenoh-Pico
    (void) (str);
    return (z_owned_config_t){._value = NULL};
}

const char *zp_config_get(z_config_t *config, unsigned int key)
{
    return _z_config_get(config, key);
}

const char *zp_config_to_str(z_config_t *config)
{
    // Not implemented yet in Zenoh-Pico
    (void) (config);
    return NULL;
}

int8_t zp_config_insert(z_config_t *config, unsigned int key, z_string_t value)
{
    return _zp_config_insert(config, key, value);
}

int8_t zp_config_insert_json(z_config_t *config, const char *key, const char *value)
{
    // Not implemented yet in Zenoh-Pico
    (void) (config);
    (void) (key);
    (void) (value);
    return 0;
}

z_encoding_t z_encoding_default(void)
{
    return (_z_encoding_t){.prefix = Z_ENCODING_APP_OCTETSTREAM, .suffix = ""};
}

z_query_target_t z_query_target_default(void)
{
    return Z_TARGET_BEST_MATCHING;
}

z_query_consolidation_t z_query_consolidation_auto(void)
{
    return (z_query_consolidation_t) {.tag = Z_QUERY_CONSOLIDATION_AUTO};
}

z_query_consolidation_t z_query_consolidation_default(void)
{
    return z_query_consolidation_reception();
}

z_query_consolidation_t z_query_consolidation_full(void)
{
    return (z_query_consolidation_t) {.tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                      .manual = {.first_routers = Z_CONSOLIDATION_MODE_FULL, .last_router = Z_CONSOLIDATION_MODE_FULL, .reception = Z_CONSOLIDATION_MODE_FULL}};
}

z_query_consolidation_t z_query_consolidation_last_router(void)
{
    return (z_query_consolidation_t) {.tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                      .manual = {.first_routers = Z_CONSOLIDATION_MODE_LAZY, .last_router = Z_CONSOLIDATION_MODE_FULL, .reception = Z_CONSOLIDATION_MODE_FULL}};
}

z_query_consolidation_t z_query_consolidation_lazy(void)
{
    return (z_query_consolidation_t) {.tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                      .manual = {.first_routers = Z_CONSOLIDATION_MODE_LAZY, .last_router = Z_CONSOLIDATION_MODE_LAZY, .reception = Z_CONSOLIDATION_MODE_LAZY}};
}

z_query_consolidation_t z_query_consolidation_none(void)
{
    return (z_query_consolidation_t) {.tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                      .manual = {.first_routers = Z_CONSOLIDATION_MODE_NONE, .last_router = Z_CONSOLIDATION_MODE_NONE, .reception = Z_CONSOLIDATION_MODE_NONE}};
}

z_query_consolidation_t z_query_consolidation_reception(void)
{
    return (z_query_consolidation_t) {.tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                      .manual = {.first_routers = Z_CONSOLIDATION_MODE_LAZY, .last_router = Z_CONSOLIDATION_MODE_LAZY, .reception = Z_CONSOLIDATION_MODE_FULL}};
}

/**************** Loans ****************/
#define _MUTABLE_OWNED_FUNCTIONS_DEFINITION(type, ownedtype, name, f_free, f_copy)    \
    bool z_##name##_check(const ownedtype *val)                                    \
    {                                                                                 \
        return val->_value != NULL;                                                   \
    }                                                                                 \
    type *z_##name##_loan(const ownedtype *val)                                       \
    {                                                                                 \
        return val->_value;                                                           \
    }                                                                                 \
    ownedtype *z_##name##_move(ownedtype *val)                                        \
    {                                                                                 \
        return val;                                                                   \
    }                                                                                 \
    ownedtype z_##name##_clone(ownedtype *val)                                        \
    {                                                                                 \
        ownedtype ret;                                                                \
        ret._value = (type*)malloc(sizeof(type));                                     \
        f_copy(ret._value, val->_value);                                              \
        return ret;                                                                   \
    }                                                                                 \
    void z_##name##_drop(ownedtype *val)                                              \
    {                                                                                 \
        f_free(&val->_value);                                                         \
    }

#define _IMMUTABLE_OWNED_FUNCTIONS_DEFINITION(type, ownedtype, name, f_free, f_copy)    \
    bool z_##name##_check(const ownedtype *val)                                      \
    {                                                                                   \
        return val->_value != NULL;                                                     \
    }                                                                                   \
    type z_##name##_loan(const ownedtype *val)                                          \
    {                                                                                   \
        return *val->_value;                                                             \
    }                                                                                   \
    ownedtype *z_##name##_move(ownedtype *val)                                          \
    {                                                                                   \
        return val;                                                                     \
    }                                                                                   \
    ownedtype z_##name##_clone(ownedtype *val)                                          \
    {                                                                                   \
        ownedtype ret;                                                                  \
        ret._value = (type*)malloc(sizeof(type));                                       \
        f_copy(ret._value, val->_value);                                                \
        return ret;                                                                     \
    }                                                                                   \
    void z_##name##_drop(ownedtype *val)                                                \
    {                                                                                   \
        f_free(&val->_value);                                                           \
    }

static inline void _z_owner_noop_free(void *s)
{
    (void)(s);
}

static inline void _z_owner_noop_copy(void *dst, const void *src)
{
    (void)(dst);
    (void)(src);
}

_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_bytes_t, z_owned_bytes_t, bytes, _z_bytes_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_string_t, z_owned_string_t, string, _z_string_free, _z_owner_noop_copy)
_IMMUTABLE_OWNED_FUNCTIONS_DEFINITION(z_keyexpr_t, z_owned_keyexpr_t, keyexpr, _z_keyexpr_free, _z_keyexpr_copy)

_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_config_t, z_owned_config_t, config, _z_config_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_session_t, z_owned_session_t, session, _z_session_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_info_t, z_owned_info_t, info, _z_config_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_pull_subscriber_t, z_owned_pull_subscriber_t, pull_subscriber, _z_owner_noop_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_subscriber_t, z_owned_subscriber_t, subscriber, _z_owner_noop_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_publisher_t, z_owned_publisher_t, publisher, _z_owner_noop_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_queryable_t, z_owned_queryable_t, queryable, _z_owner_noop_free, _z_owner_noop_copy)

_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_encoding_t, z_owned_encoding_t, encoding, _z_owner_noop_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_period_t, z_owned_period_t, period, _z_owner_noop_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_consolidation_strategy_t, z_owned_consolidation_strategy_t, consolidation_strategy, _z_owner_noop_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_query_target_t, z_owned_query_target_t, query_target, _z_owner_noop_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_query_consolidation_t, z_owned_query_consolidation_t, query_consolidation, _z_owner_noop_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_put_options_t, z_owned_put_options_t, put_options, _z_owner_noop_free, _z_owner_noop_copy)

_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_sample_t, z_owned_sample_t, sample, _z_sample_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_hello_t, z_owned_hello_t, hello, _z_hello_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_reply_t, z_owned_reply_t, reply, _z_owner_noop_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_reply_data_t, z_owned_reply_data_t, reply_data, _z_reply_data_free, _z_owner_noop_copy)

_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_str_array_t, z_owned_str_array_t, str_array, _z_str_array_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_hello_array_t, z_owned_hello_array_t, hello_array, _z_hello_array_free, _z_owner_noop_copy)
_MUTABLE_OWNED_FUNCTIONS_DEFINITION(z_reply_data_array_t, z_owned_reply_data_array_t, reply_data_array, _z_reply_data_array_free, _z_owner_noop_copy)

z_owned_closure_sample_t *z_closure_sample_move(z_owned_closure_sample_t *closure_sample)
{
    return closure_sample;
}

z_owned_closure_query_t *z_closure_query_move(z_owned_closure_query_t *closure_query)
{
    return closure_query;
}

z_owned_closure_reply_t *z_closure_reply_move(z_owned_closure_reply_t *closure_reply)
{
    return closure_reply;
}

/************* Primitives **************/
z_owned_hello_array_t z_scout(z_zint_t what, z_owned_config_t *config, unsigned long timeout)
{
    z_owned_hello_array_t hellos;
    hellos._value = (z_hello_array_t*)malloc(sizeof(z_hello_array_t));
    *hellos._value = _z_scout(what, config->_value, timeout);

    z_config_drop(config);
    config->_value = NULL;

    return hellos; 
}

z_owned_session_t z_open(z_owned_config_t *config)
{
    z_owned_session_t zs;
    zs._value = _z_open(config->_value);
    config->_value = NULL;

    return zs;
}

int8_t z_close(z_owned_session_t *zs)
{
    _z_close(zs->_value);

    z_session_drop(zs);
    zs->_value = NULL;

    return 0;
}

z_owned_info_t z_info(const z_session_t *zs)
{
    z_owned_info_t zi;
    zi._value = _z_info(zs);

    return zi;
}

//z_owned_string_t z_info_as_str(const z_session_t *zs)
//{
//    z_string_t *str = (z_string_t*)malloc(sizeof(z_string_t));
//    *str = (z_string_t){.val = NULL, .len = 0};
//
//    _z_config_t *zi = _z_info(zs);
//
//    _z_string_t append;
//    _z_string_move_str(&append, "info_router_pid : ");
//    _z_string_append(str, &append);
//
//    append = _z_config_get(zi, Z_INFO_PID_KEY);
//    _z_string_append(str, &append);
//
//    _z_string_move_str(&append, "\ninfo_pid : ");
//    _z_string_append(str, &append);
//
//    append = _z_config_get(zi, Z_INFO_ROUTER_PID_KEY);
//    _z_string_append(str, &append);
//
//    _z_config_free(&zi);
//
//    z_owned_string_t ret = {._value = str, .is_valid = 1};
//    return ret;
//}

char *z_info_get(z_info_t *info, unsigned int key)
{
    return _z_config_get(info, key);
}

z_put_options_t z_put_options_default(void)
{
    return (z_put_options_t) {.encoding = z_encoding_default(), .congestion_control = Z_CONGESTION_CONTROL_DEFAULT, .priority = Z_PRIORITY_DATA};
}

int8_t z_put(z_session_t *zs, z_keyexpr_t keyexpr, const uint8_t *payload, z_zint_t len, const z_put_options_t *options)
{
    if (options != NULL)
        return _z_write_ext(zs, keyexpr, (const uint8_t *)payload, len, options->encoding, Z_SAMPLE_KIND_PUT, options->congestion_control);

    return _z_write_ext(zs, keyexpr, (const uint8_t *)payload, len, z_encoding_default(), Z_SAMPLE_KIND_PUT, Z_CONGESTION_CONTROL_DEFAULT);
}

z_get_options_t z_get_options_default(void)
{
    return (z_get_options_t){.target = z_query_target_default(), .consolidation = z_query_consolidation_default()};
}

int8_t z_get(z_session_t *zs, z_keyexpr_t keyexpr, const char *predicate, z_owned_closure_reply_t *callback, const z_get_options_t *options)
{
    void *ctx = callback->context;
    callback->context = NULL;

    z_consolidation_strategy_t strategy;
    _z_target_t target;
    target._kind = Z_QUERYABLE_ALL_KINDS;

    if (options != NULL)
    {
        // TODO: Check before release
        if (options->consolidation.tag == Z_QUERY_CONSOLIDATION_MANUAL)
            strategy = options->consolidation.manual;
        else
        {
            // if (keyexpr.rname.)
            strategy = z_query_consolidation_default().manual;
            // QueryConsolidation::Auto => {
            //     if self.selector.has_time_range() {
            //         ConsolidationStrategy::none()
            //     } else {
            //         ConsolidationStrategy::default()
            //     }
            // }
        }
        target._target = options->target;
    }
    else
    {
        target._target = z_query_target_default();
        strategy = z_query_consolidation_default().manual;
    }

    return _z_query(zs, keyexpr, predicate, target, strategy, callback->call, callback->drop, ctx);
}

z_owned_keyexpr_t z_declare_keyexpr(z_session_t *zs, z_keyexpr_t keyexpr)
{
    z_owned_keyexpr_t key;
    key._value = (z_keyexpr_t*)malloc(sizeof(z_keyexpr_t));
    *key._value = _z_rid_with_suffix(_z_declare_resource(zs, keyexpr), NULL);

    return key;
}

int8_t z_undeclare_keyexpr(z_session_t *zs, z_owned_keyexpr_t *keyexpr)
{
    _z_undeclare_resource(zs, keyexpr->_value->id);

    z_keyexpr_drop(keyexpr);
    keyexpr->_value = NULL;

    return 0;
}

z_publisher_options_t z_publisher_options_default(void)
{
    return (z_publisher_options_t){.local_routing = -1, .congestion_control = Z_CONGESTION_CONTROL_DEFAULT, .priority = Z_PRIORITY_DATA_HIGH};
}

z_owned_publisher_t z_declare_publisher(z_session_t *zs, z_keyexpr_t keyexpr, z_publisher_options_t *options)
{
    if (options != NULL)
        return (z_owned_publisher_t){._value = _z_declare_publisher(zs, keyexpr, options->local_routing, options->congestion_control, options->priority)};

    z_publisher_options_t opt = z_publisher_options_default();
    return (z_owned_publisher_t){._value = _z_declare_publisher(zs, keyexpr, opt.local_routing, opt.congestion_control, opt.priority)};
}

int8_t z_undeclare_publisher(z_owned_publisher_t *pub)
{
    _z_undeclare_publisher(pub->_value);

    z_publisher_drop(pub);
    free(pub->_value);
    pub->_value = NULL;

    return 0;
}

int8_t z_publisher_put(const z_publisher_t *pub, const uint8_t *payload, size_t len, const z_publisher_put_options_t *options)
{
    if (options != NULL)
        return _z_write_ext(pub->_zn, pub->_key, payload, len, options->encoding, Z_SAMPLE_KIND_PUT, pub->_congestion_control);

    return _z_write_ext(pub->_zn, pub->_key, payload, len, z_encoding_default(), Z_SAMPLE_KIND_PUT, pub->_congestion_control);
}

int8_t z_publisher_delete(const z_publisher_t *pub)
{
    return _z_write_ext(pub->_zn, pub->_key, NULL, 0, z_encoding_default(), Z_SAMPLE_KIND_DELETE, pub->_congestion_control);
}

z_subscriber_options_t z_subscriber_options_default(void)
{
    return (z_subscriber_options_t){.reliability = Z_RELIABILITY_RELIABLE};
}

z_owned_subscriber_t z_declare_subscriber(z_session_t *zs, z_keyexpr_t keyexpr, z_owned_closure_sample_t *callback, const z_subscriber_options_t *options)
{
    void *ctx = callback->context;
    callback->context = NULL;

    _z_subinfo_t subinfo = _z_subinfo_push_default();
    if (options != NULL)
        subinfo.reliability = options->reliability;

    return (z_owned_subscriber_t){._value = _z_declare_subscriber(zs, keyexpr, subinfo, callback->call, callback->drop, ctx)};
}

z_owned_pull_subscriber_t z_declare_pull_subscriber(z_session_t *zs, z_keyexpr_t keyexpr, z_owned_closure_sample_t *callback, const z_subscriber_options_t *options)
{
    void *ctx = callback->context;
    callback->context = NULL;

    _z_subinfo_t subinfo = _z_subinfo_pull_default();
    if (options != NULL)
        subinfo.reliability = options->reliability;

    return (z_owned_pull_subscriber_t){._value = _z_declare_subscriber(zs, keyexpr, subinfo, callback->call, callback->drop, ctx)};
}

int8_t z_undeclare_subscriber(z_owned_subscriber_t *sub)
{
    _z_undeclare_subscriber(sub->_value);

    z_subscriber_drop(sub);
    free(sub->_value);
    sub->_value = NULL;

    return 0;
}

int8_t z_undeclare_pull_subscriber(z_owned_pull_subscriber_t *sub)
{
    _z_undeclare_subscriber(sub->_value);

    z_pull_subscriber_drop(sub);
    free(sub->_value);
    sub->_value = NULL;

    return 0;
}

int8_t z_pull(const z_pull_subscriber_t *sub)
{
    return _z_pull(sub);
}

z_queryable_options_t z_queryable_options_default(void)
{
    return (z_queryable_options_t){.complete = _Z_QUERYABLE_COMPLETE_DEFAULT};
}

z_owned_queryable_t z_declare_queryable(z_session_t *zs, z_keyexpr_t keyexpr, z_owned_closure_query_t *callback, const z_queryable_options_t *options)
{
    void *ctx = callback->context;
    callback->context = NULL;

    if (options != NULL)
        return (z_owned_queryable_t){._value = _z_declare_queryable(zs, keyexpr, options->complete, callback->call, callback->drop, ctx)};

    z_queryable_options_t opt = z_queryable_options_default();
    return (z_owned_queryable_t){._value = _z_declare_queryable(zs, keyexpr, opt.complete, callback->call, callback->drop, ctx)};
}

int8_t z_undeclare_queryable(z_owned_queryable_t *queryable)
{
    _z_undeclare_queryable(queryable->_value);

    z_queryable_drop(queryable);
    free(queryable->_value);
    queryable->_value = NULL;

    return 0;
}

int8_t z_query_reply(const z_query_t *query, const z_keyexpr_t keyexpr, const uint8_t *payload, size_t len)
{
    return _z_send_reply(query, keyexpr, payload, len);
}

uint8_t z_reply_is_ok(const z_owned_reply_t *reply)
{
    (void) (reply);
    // For the moment always return TRUE.
    // The support for reply errors will come in the next release.
    return 1;
}

z_value_t z_reply_err(const z_owned_reply_t *reply)
{
    (void) (reply);
    return (z_value_t){ .payload = _z_bytes_make(0), .encoding = z_encoding_default() };
}

z_sample_t z_reply_ok(const z_owned_reply_t *reply)
{
    return reply->_value->data.sample;
}

/**************** Tasks ****************/
int8_t zp_start_read_task(z_session_t *zs)
{
    return _zp_start_read_task(zs);
}

int8_t zp_stop_read_task(z_session_t *zs)
{
    return _zp_stop_read_task(zs);
}

int8_t zp_start_lease_task(z_session_t *zs)
{
    return _zp_start_lease_task(zs);
}

int8_t zp_stop_lease_task(z_session_t *zs)
{
    return _zp_stop_lease_task(zs);
}
