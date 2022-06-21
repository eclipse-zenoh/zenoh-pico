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

#ifndef ZENOH_PICO_API_PRIMITIVES_H
#define ZENOH_PICO_API_PRIMITIVES_H

#include "zenoh-pico/api/types.h"

#include "zenoh-pico/net/session.h"
#include "zenoh-pico/net/subscribe.h"
#include "zenoh-pico/net/query.h"

/*************** Logging ***************/
void z_init_logger(void);

/********* Data Types Handlers *********/
z_owned_bytes_t z_bytes_new(const uint8_t *start, z_zint_t len);
z_owned_string_t z_string_new(const char *s);
z_keyexpr_t z_keyexpr(const char *name);

z_owned_config_t z_config_new(void);
z_owned_config_t z_config_empty(void);
z_owned_config_t z_config_default(void);
z_owned_config_t z_config_client(const char *const *peers, size_t n_peers);
z_owned_config_t z_config_peer(void);
z_owned_config_t z_config_from_file(const char *path);
z_owned_config_t z_config_from_str(const char *str);
const char *z_config_get(z_config_t *config, unsigned int key);
const char *z_config_to_str(z_config_t *config);
uint8_t z_config_insert(z_config_t *config, unsigned int key, z_string_t value);
uint8_t z_config_insert_json(z_config_t *config, const char *key, const char *value);

z_encoding_t z_encoding_default(void);

z_target_t z_target_default(void);
z_query_target_t z_query_target_default(void);
z_query_consolidation_t z_query_consolidation_auto(void);
z_query_consolidation_t z_query_consolidation_default(void);
z_query_consolidation_t z_query_consolidation_full(void);
z_query_consolidation_t z_query_consolidation_last_router(void);
z_query_consolidation_t z_query_consolidation_lazy(void);
z_query_consolidation_t z_query_consolidation_none(void);
z_query_consolidation_t z_query_consolidation_reception(void);

#define _OWNED_FUNCTIONS(type, ownedtype, name)          \
    uint8_t z_##name##_check(const ownedtype *name);     \
    type *z_##name##_loan(const ownedtype *name);        \
    ownedtype *z_##name##_move(ownedtype *name);         \
    ownedtype z_##name##_clone(ownedtype *name);         \
    void z_##name##_drop(ownedtype *name);

_OWNED_FUNCTIONS(z_string_t, z_owned_str_t, str)
_OWNED_FUNCTIONS(z_bytes_t, z_owned_bytes_t, bytes)

_OWNED_FUNCTIONS(z_string_t, z_owned_string_t, string)
_OWNED_FUNCTIONS(z_keyexpr_t, z_owned_keyexpr_t, keyexpr)

_OWNED_FUNCTIONS(z_config_t, z_owned_config_t, config)
_OWNED_FUNCTIONS(z_session_t, z_owned_session_t, session)
_OWNED_FUNCTIONS(z_info_t, z_owned_info_t, info)
_OWNED_FUNCTIONS(z_subscriber_t, z_owned_subscriber_t, subscriber)
_OWNED_FUNCTIONS(z_publisher_t, z_owned_publisher_t, publisher)
_OWNED_FUNCTIONS(z_queryable_t, z_owned_queryable_t, queryable)

_OWNED_FUNCTIONS(z_encoding_t, z_owned_encoding_t, encoding)
_OWNED_FUNCTIONS(z_period_t, z_owned_period_t, period)
_OWNED_FUNCTIONS(z_consolidation_strategy_t, z_owned_consolidation_strategy_t, consolidation_strategy)
_OWNED_FUNCTIONS(z_query_target_t, z_owned_query_target_t, query_target)
_OWNED_FUNCTIONS(z_target_t, z_owned_target_t, target)
_OWNED_FUNCTIONS(z_query_consolidation_t, z_owned_query_consolidation_t, query_consolidation)
_OWNED_FUNCTIONS(z_put_options_t, z_owned_put_options_t, put_options)

_OWNED_FUNCTIONS(z_sample_t, z_owned_sample_t, sample)
_OWNED_FUNCTIONS(z_hello_t, z_owned_hello_t, hello)
_OWNED_FUNCTIONS(z_reply_t, z_owned_reply_t, reply)
_OWNED_FUNCTIONS(z_reply_data_t, z_owned_reply_data_t, reply_data)

_OWNED_FUNCTIONS(z_str_array_t, z_owned_str_array_t, str_array)
_OWNED_FUNCTIONS(z_hello_array_t, z_owned_hello_array_t, hello_array)
_OWNED_FUNCTIONS(z_reply_data_array_t, z_owned_reply_data_array_t, reply_data_array)

/************* Primitives **************/
z_owned_hello_array_t z_scout(z_zint_t what, z_owned_config_t *config, unsigned long timeout);

z_owned_session_t z_open(z_owned_config_t *config);
void z_close(z_owned_session_t *zs);

z_owned_info_t z_info(const z_session_t *zs);
z_owned_string_t z_info_as_str(const z_session_t *zs);
char *z_info_get(z_info_t *info, unsigned int key);

z_owned_keyexpr_t z_declare_keyexpr(z_session_t *zs, z_keyexpr_t keyexpr);
void z_undeclare_expr(z_session_t *zs, z_owned_keyexpr_t *keyexpr);

z_owned_publisher_t z_declare_publisher(z_session_t *zs, z_keyexpr_t keyexpr, z_publisher_options_t *options);
void z_publisher_close(z_owned_publisher_t *sub);
z_publisher_options_t z_publisher_options_default(void);

int z_put(z_session_t *zs, z_keyexpr_t *keyexpr, const uint8_t *payload, z_zint_t len);
int z_put_ext(z_session_t *zs, z_keyexpr_t keyexpr, const uint8_t *payload, z_zint_t len, const z_put_options_t *opt);
z_put_options_t z_put_options_default(void);

z_subscriber_options_t z_subscriber_options_default(void);
z_owned_subscriber_t z_declare_subscriber(z_session_t *zs, z_keyexpr_t keyexpr, z_closure_sample_t *callback, const z_subscriber_options_t *options);
void z_pull(const z_subscriber_t *sub);
void z_subscriber_close(z_owned_subscriber_t *sub);

void z_get(z_session_t *zs, z_keyexpr_t keyexpr, const char *predicate, z_target_t target, z_query_consolidation_t consolidation, void (*callback)(const z_reply_t*, const void*), void *arg);
z_owned_reply_data_array_t z_get_collect(z_session_t *zs, z_keyexpr_t keyexpr, const char *predicate, z_target_t target, z_query_consolidation_t consolidation);

void z_closure_query_call(const z_closure_query_t *closure, z_query_t query);
z_queryable_options_t z_queryable_options_default(void);
z_owned_queryable_t z_declare_queryable(z_session_t *zs, z_keyexpr_t keyexpr, z_closure_query_t *callback, const z_queryable_options_t *options);
void z_queryable_close(z_owned_queryable_t *queryable);
void z_send_reply(const z_query_t *query, const char *key, const uint8_t *payload, size_t len);

/************* Tasks **************/
int zp_start_read_task(z_session_t *zs);
int zp_stop_read_task(z_session_t *zs);

int zp_start_lease_task(z_session_t *zs);
int zp_stop_lease_task(z_session_t *zs);

#endif /* ZENOH_PICO_API_PRIMITIVES_H */
