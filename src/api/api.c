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
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/protocol/utils.h"

void z_init_logger(void)
{
    // TODO
}

z_owned_config_t z_config_default(void)
{
    return (z_owned_config_t){._value = _z_config_default()};
}

uint8_t z_config_insert(z_config_t *config, unsigned int key, z_string_t value)
{
    return _z_config_insert(config, key, value);
}

z_owned_session_t z_open(z_owned_config_t *config)
{
    z_owned_session_t zs;
    zs._value = _z_open(config->_value);

    config->_value = NULL;
    return zs;
}

void z_close(z_owned_session_t *zs)
{
    _z_close(zs->_value);

    z_session_clear(zs);
    zs->_value = NULL;
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

z_owned_keyexpr_t z_id_new(const z_zint_t id)
{
    z_owned_keyexpr_t key;
    key._value = (z_keyexpr_t*)malloc(sizeof(z_keyexpr_t));
    *key._value = _z_rid(id);
    return key;
}

z_keyexpr_t z_keyexpr(const char *name)
{
    return _z_rname(name);
}

z_owned_keyexpr_t z_id_with_suffix_new(const z_zint_t id, const char *name)
{
    z_owned_keyexpr_t key;
    key._value = (z_keyexpr_t*)malloc(sizeof(z_keyexpr_t));
    *key._value = _z_rid_with_suffix(id, name);
    return key;
}

z_owned_keyexpr_t z_declare_keyexpr(z_session_t *zs, z_keyexpr_t keyexpr)
{
    z_owned_keyexpr_t key;
    key._value = (z_keyexpr_t*)malloc(sizeof(z_keyexpr_t));
    *key._value = _z_rid_with_suffix(_z_declare_resource(zs, keyexpr), NULL);

    return key;
}

void z_undeclare_expr(z_session_t *zs, z_owned_keyexpr_t *keyexpr)
{
    _z_undeclare_resource(zs, keyexpr->_value->id);

    z_keyexpr_clear(keyexpr);
    keyexpr->_value = NULL;
}

z_owned_publisher_t z_declare_publication(z_session_t *zs, z_keyexpr_t keyexpr)
{
    return (z_owned_publisher_t){._value = _z_declare_publisher(zs, keyexpr)};
}

z_encoding_t z_encoding_default(void)
{
    return (_z_encoding_t){.prefix = Z_ENCODING_APP_OCTETSTREAM, .suffix = ""};
}

z_owned_queryable_t z_queryable_new(z_session_t *zs, z_keyexpr_t keyexpr, unsigned int kind, void (*callback)(const z_query_t*, const void*), void *arg)
{
    return (z_owned_queryable_t){._value = _z_declare_queryable(zs, keyexpr, kind, callback, arg)};
}

void z_queryable_close(z_owned_queryable_t *queryable)
{
    _z_undeclare_queryable(queryable->_value);

    z_queryable_clear(queryable);
    free(queryable->_value);
    queryable->_value = NULL;
}

z_query_consolidation_t z_query_consolidation_auto(void)
{
    return (z_query_consolidation_t) {._tag = Z_QUERY_CONSOLIDATION_AUTO};
}

z_query_consolidation_t z_query_consolidation_default(void)
{
    return z_query_consolidation_reception();
}

z_query_consolidation_t z_query_consolidation_full(void)
{
    return (z_query_consolidation_t) {._tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                      ._strategy._manual = {.first_routers = Z_CONSOLIDATION_MODE_FULL, .last_router = Z_CONSOLIDATION_MODE_FULL, .reception = Z_CONSOLIDATION_MODE_FULL}};
}

z_query_consolidation_t z_query_consolidation_last_router(void)
{
    return (z_query_consolidation_t) {._tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                      ._strategy._manual = {.first_routers = Z_CONSOLIDATION_MODE_LAZY, .last_router = Z_CONSOLIDATION_MODE_FULL, .reception = Z_CONSOLIDATION_MODE_FULL}};
}

z_query_consolidation_t z_query_consolidation_lazy(void)
{
    return (z_query_consolidation_t) {._tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                      ._strategy._manual = {.first_routers = Z_CONSOLIDATION_MODE_LAZY, .last_router = Z_CONSOLIDATION_MODE_LAZY, .reception = Z_CONSOLIDATION_MODE_LAZY}};
}

z_query_consolidation_t z_query_consolidation_none(void)
{
    return (z_query_consolidation_t) {._tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                      ._strategy._manual = {.first_routers = Z_CONSOLIDATION_MODE_NONE, .last_router = Z_CONSOLIDATION_MODE_NONE, .reception = Z_CONSOLIDATION_MODE_NONE}};
}

z_query_consolidation_t z_query_consolidation_reception(void)
{
    return (z_query_consolidation_t) {._tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                      ._strategy._manual = {.first_routers = Z_CONSOLIDATION_MODE_LAZY, .last_router = Z_CONSOLIDATION_MODE_LAZY, .reception = Z_CONSOLIDATION_MODE_FULL}};
}

z_target_t z_target_default(void)
{
    return _z_target_default();
}

void z_get(z_session_t *zs, z_keyexpr_t keyexpr, const char *predicate, z_target_t target, z_query_consolidation_t consolidation, void (*callback)(const z_reply_t*, const void*), void *arg)
{
    // FIXME: move this to a function
    _z_consolidation_strategy_t strategy;
    if (consolidation._tag == Z_QUERY_CONSOLIDATION_MANUAL)
        strategy = consolidation._strategy._manual;
    else
    {
        // if (keyexpr.rname.)
        strategy = z_query_consolidation_default()._strategy._manual;
        // QueryConsolidation::Auto => {
        //     if self.selector.has_time_range() {
        //         ConsolidationStrategy::none()
        //     } else {
        //         ConsolidationStrategy::default()
        //     }
        // }
    }

    _z_query(zs, keyexpr, predicate, target, strategy, callback, arg);
}

z_owned_reply_data_array_t z_get_collect(z_session_t *zs, z_keyexpr_t keyexpr, const char *predicate, z_target_t target, z_query_consolidation_t consolidation)
{
    // FIXME: move this to a function
    _z_consolidation_strategy_t strategy;
    if (consolidation._tag == Z_QUERY_CONSOLIDATION_MANUAL)
        strategy = consolidation._strategy._manual;
    else
    {
        // if (keyexpr.rname.)
        strategy = z_query_consolidation_default()._strategy._manual;
        // QueryConsolidation::Auto => {
        //     if self.selector.has_time_range() {
        //         ConsolidationStrategy::none()
        //     } else {
        //         ConsolidationStrategy::default()
        //     }
        // }
    }

    z_owned_reply_data_array_t rda;
    rda._value = (z_reply_data_array_t*)malloc(sizeof(z_reply_data_array_t));
    *rda._value = _z_query_collect(zs, keyexpr, predicate, target, strategy);

    return rda;
}

void z_send_reply(const z_query_t *query, const char *key, const uint8_t *payload, size_t len)
{
    _z_send_reply(query, key, payload, len);
}

z_owned_hello_array_t z_scout(z_zint_t what, z_owned_config_t *config, unsigned long timeout)
{
    z_owned_hello_array_t hellos;
    hellos._value = (z_hello_array_t*)malloc(sizeof(z_hello_array_t));
    *hellos._value = _z_scout(what, config->_value, timeout);

    z_config_clear(config);
    config->_value = NULL;

    return hellos; 
}

int z_put(z_session_t *zs, z_keyexpr_t *keyexpr, const uint8_t *payload, size_t len)
{
    return _z_write(zs, *keyexpr, (const uint8_t *)payload, len);
}

int z_put_ext(z_session_t *zs, z_keyexpr_t keyexpr, const uint8_t *payload, z_zint_t len, const z_put_options_t *opt)
{
    return _z_write_ext(zs, keyexpr, (const uint8_t *)payload, len, opt->encoding, opt->kind, opt->congestion_control);
}

z_put_options_t z_put_options_default(void)
{
    return (z_put_options_t) {.encoding = z_encoding_default(), .kind = Z_DATA_KIND_DEFAULT, .congestion_control = Z_CONGESTION_CONTROL_DROP, .priority = Z_PRIORITY_DATA};
}

z_subscriber_options_t z_subscriber_options_default(void)
{
    return (z_subscriber_options_t){.reliability = Z_RELIABILITY_RELIABLE, .cargs = NULL};
}

z_owned_subscriber_t z_declare_subscriber(z_session_t *zs, z_keyexpr_t keyexpr, void (*callback)(const z_sample_t*, const void*), const z_subscriber_options_t *opts)
{
    _z_subinfo_t subinfo = _z_subinfo_default();
    void *cargs = NULL;
    if (opts != NULL)
    {
        subinfo.reliability = opts->reliability;
        cargs = opts->cargs;
    }


    return (z_owned_subscriber_t){._value = _z_declare_subscriber(zs, keyexpr, subinfo, callback, cargs)};
}

void z_pull(const z_subscriber_t *sub)
{
    _z_pull(sub);
}

void z_subscriber_close(z_owned_subscriber_t *sub)
{
    _z_undeclare_subscriber(sub->_value);

    z_subscriber_clear(sub);
    free(sub->_value);
    sub->_value = NULL;
}

void z_publisher_close(z_owned_publisher_t *pub)
{
    _z_undeclare_publisher(pub->_value);

    z_publisher_clear(pub);
    free(pub->_value);
    pub->_value = NULL;
}

/**************** Tasks ****************/
int zp_start_read_task(z_session_t *zs)
{
    return _zp_start_read_task(zs);
}

int zp_stop_read_task(z_session_t *zs)
{
    return _zp_stop_read_task(zs);
}

int zp_start_lease_task(z_session_t *zs)
{
    return _zp_start_lease_task(zs);
}

int zp_stop_lease_task(z_session_t *zs)
{
    return _zp_stop_lease_task(zs);
}

/**************** Loans ****************/
#define _OWNED_FUNCTIONS_DEFINITION(type, ownedtype, name, f_free, f_copy)    \
    uint8_t z_##name##_check(const ownedtype *val)                            \
    {                                                                         \
        return val->_value != NULL;                                           \
    }                                                                         \
    type *z_##name##_loan(const ownedtype *val)                               \
    {                                                                         \
        return val->_value;                                                   \
    }                                                                         \
    ownedtype *z_##name##_move(ownedtype *val)                                \
    {                                                                         \
        return val;                                                           \
    }                                                                         \
    ownedtype z_##name##_clone(ownedtype *val)                                \
    {                                                                         \
        ownedtype ret;                                                        \
        ret._value = (type*)malloc(sizeof(type));                             \
        f_copy(ret._value, val->_value);                                      \
        return ret;                                                           \
    }                                                                         \
    void z_##name##_clear(ownedtype *val)                                     \
    {                                                                         \
        f_free(&val->_value);                                                 \
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

// _OWNED_FUNCTIONS_DEFINITION(z_str_t, z_owned_str_t, str, _z_owner_noop_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_bytes_t, z_owned_bytes_t, bytes, _z_bytes_free, _z_owner_noop_copy)

_OWNED_FUNCTIONS_DEFINITION(z_string_t, z_owned_string_t, string, _z_string_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_keyexpr_t, z_owned_keyexpr_t, keyexpr, _z_keyexpr_free, _z_keyexpr_copy)

_OWNED_FUNCTIONS_DEFINITION(z_config_t, z_owned_config_t, config, _z_config_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_session_t, z_owned_session_t, session, _z_session_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_info_t, z_owned_info_t, info, _z_config_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_subscriber_t, z_owned_subscriber_t, subscriber, _z_owner_noop_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_publisher_t, z_owned_publisher_t, publisher, _z_owner_noop_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_queryable_t, z_owned_queryable_t, queryable, _z_owner_noop_free, _z_owner_noop_copy)

_OWNED_FUNCTIONS_DEFINITION(z_encoding_t, z_owned_encoding_t, encoding, _z_owner_noop_free, _z_owner_noop_copy)
// _OWNED_FUNCTIONS_DEFINITION(z_subinfo_t, z_owned_subinfo_t, subinfo, _z_subinfo_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_period_t, z_owned_period_t, period, _z_owner_noop_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_consolidation_strategy_t, z_owned_consolidation_strategy_t, consolidation_strategy, _z_owner_noop_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_query_target_t, z_owned_query_target_t, query_target, _z_owner_noop_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_target_t, z_owned_target_t, target, _z_owner_noop_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_query_consolidation_t, z_owned_query_consolidation_t, query_consolidation, _z_owner_noop_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_put_options_t, z_owned_put_options_t, put_options, _z_owner_noop_free, _z_owner_noop_copy)

_OWNED_FUNCTIONS_DEFINITION(z_sample_t, z_owned_sample_t, sample, _z_sample_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_hello_t, z_owned_hello_t, hello, _z_hello_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_reply_t, z_owned_reply_t, reply, _z_owner_noop_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_reply_data_t, z_owned_reply_data_t, reply_data, _z_reply_data_free, _z_owner_noop_copy)

_OWNED_FUNCTIONS_DEFINITION(z_str_array_t, z_owned_str_array_t, str_array, _z_str_array_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_hello_array_t, z_owned_hello_array_t, hello_array, _z_hello_array_free, _z_owner_noop_copy)
_OWNED_FUNCTIONS_DEFINITION(z_reply_data_array_t, z_owned_reply_data_array_t, reply_data_array, _z_reply_data_array_free, _z_owner_noop_copy)
