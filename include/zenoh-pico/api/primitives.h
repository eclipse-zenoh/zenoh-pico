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

#include <stdint.h>
#include <stdbool.h>

#include "zenoh-pico/api/types.h"

#include "zenoh-pico/net/session.h"
#include "zenoh-pico/net/subscribe.h"
#include "zenoh-pico/net/query.h"

/*************** Logging ***************/
int8_t z_init_logger(void);

/********* Data Types Handlers *********/
z_owned_bytes_t z_bytes_new(const uint8_t *start, z_zint_t len);
z_owned_string_t z_string_new(const char *s);
z_keyexpr_t z_keyexpr(const char *name);
const char *z_keyexpr_to_string(z_keyexpr_t key);
char *z_keyexpr_resolve(z_session_t *zs, z_keyexpr_t key);
uint8_t z_keyexpr_is_valid(z_keyexpr_t *key);

z_keyexpr_canon_status_t z_keyexpr_is_canon(const char *start, size_t len);
z_keyexpr_canon_status_t zp_keyexpr_is_canon_null_terminated(const char *start);
z_keyexpr_canon_status_t z_keyexpr_canonize(char *start, size_t *len);
z_keyexpr_canon_status_t zp_keyexpr_canonize_null_terminated(char *start);
bool z_keyexpr_includes(const char *l, size_t llen, const char *r, size_t rlen);
bool zp_keyexpr_includes_null_terminated(const char *l, const char *r);
bool z_keyexpr_intersect(const char *l, size_t llen, const char *r, size_t rlen);
bool zp_keyexpr_intersect_null_terminated(const char *l, const char *r);
bool z_keyexpr_equals(const char *l, size_t llen, const char *r, size_t rlen);
bool zp_keyexpr_equals_null_terminated(const char *l, const char *r);

z_owned_config_t zp_config_new(void);
z_owned_config_t zp_config_empty(void);
z_owned_config_t zp_config_default(void);
const char *zp_config_get(z_config_t *config, unsigned int key);
int8_t zp_config_insert(z_config_t *config, unsigned int key, z_string_t value);

z_encoding_t z_encoding_default(void);

z_query_target_t z_query_target_default(void);
z_query_consolidation_t z_query_consolidation_auto(void);
z_query_consolidation_t z_query_consolidation_default(void);
z_query_consolidation_t z_query_consolidation_full(void);
z_query_consolidation_t z_query_consolidation_last_router(void);
z_query_consolidation_t z_query_consolidation_lazy(void);
z_query_consolidation_t z_query_consolidation_none(void);
z_query_consolidation_t z_query_consolidation_reception(void);

z_bytes_t z_query_value_selector(z_query_t *query);
z_keyexpr_t z_query_keyexpr(z_query_t *query);

/**************** Loans ****************/
#define _MUTABLE_OWNED_FUNCTIONS(type, ownedtype, name)  \
    bool z_##name##_check(const ownedtype *name);     \
    type *z_##name##_loan(const ownedtype *name);        \
    ownedtype *z_##name##_move(ownedtype *name);         \
    ownedtype z_##name##_clone(ownedtype *name);         \
    void z_##name##_drop(ownedtype *name);

#define _IMMUTABLE_OWNED_FUNCTIONS(type, ownedtype, name)  \
    bool z_##name##_check(const ownedtype *name);       \
    type z_##name##_loan(const ownedtype *name);          \
    ownedtype *z_##name##_move(ownedtype *name);           \
    ownedtype z_##name##_clone(ownedtype *name);           \
    void z_##name##_drop(ownedtype *name);

_MUTABLE_OWNED_FUNCTIONS(z_bytes_t, z_owned_bytes_t, bytes)
_MUTABLE_OWNED_FUNCTIONS(z_string_t, z_owned_string_t, string)
_IMMUTABLE_OWNED_FUNCTIONS(z_keyexpr_t, z_owned_keyexpr_t, keyexpr)

_MUTABLE_OWNED_FUNCTIONS(z_config_t, z_owned_config_t, config)
_MUTABLE_OWNED_FUNCTIONS(z_session_t, z_owned_session_t, session)
_MUTABLE_OWNED_FUNCTIONS(z_info_t, z_owned_info_t, info)
_MUTABLE_OWNED_FUNCTIONS(z_subscriber_t, z_owned_subscriber_t, subscriber)
_MUTABLE_OWNED_FUNCTIONS(z_pull_subscriber_t, z_owned_pull_subscriber_t, pull_subscriber)
_MUTABLE_OWNED_FUNCTIONS(z_publisher_t, z_owned_publisher_t, publisher)
_MUTABLE_OWNED_FUNCTIONS(z_queryable_t, z_owned_queryable_t, queryable)

_MUTABLE_OWNED_FUNCTIONS(z_encoding_t, z_owned_encoding_t, encoding)
_MUTABLE_OWNED_FUNCTIONS(z_period_t, z_owned_period_t, period)
_MUTABLE_OWNED_FUNCTIONS(z_consolidation_strategy_t, z_owned_consolidation_strategy_t, consolidation_strategy)
_MUTABLE_OWNED_FUNCTIONS(z_query_target_t, z_owned_query_target_t, query_target)
_MUTABLE_OWNED_FUNCTIONS(z_query_consolidation_t, z_owned_query_consolidation_t, query_consolidation)
_MUTABLE_OWNED_FUNCTIONS(z_put_options_t, z_owned_put_options_t, put_options)

_MUTABLE_OWNED_FUNCTIONS(z_sample_t, z_owned_sample_t, sample)
_MUTABLE_OWNED_FUNCTIONS(z_hello_t, z_owned_hello_t, hello)
_MUTABLE_OWNED_FUNCTIONS(z_reply_t, z_owned_reply_t, reply)
_MUTABLE_OWNED_FUNCTIONS(z_reply_data_t, z_owned_reply_data_t, reply_data)

_MUTABLE_OWNED_FUNCTIONS(z_str_array_t, z_owned_str_array_t, str_array)
_MUTABLE_OWNED_FUNCTIONS(z_hello_array_t, z_owned_hello_array_t, hello_array)
_MUTABLE_OWNED_FUNCTIONS(z_reply_data_array_t, z_owned_reply_data_array_t, reply_data_array)

z_owned_closure_sample_t *z_closure_sample_move(z_owned_closure_sample_t *closure_sample);
z_owned_closure_query_t *z_closure_query_move(z_owned_closure_query_t *closure_query);
z_owned_closure_reply_t *z_closure_reply_move(z_owned_closure_reply_t *closure_reply);
z_owned_closure_zid_t *z_closure_zid_move(z_owned_closure_zid_t *closure_zid);

/************* Primitives **************/
z_owned_hello_array_t z_scout(z_zint_t what, z_owned_config_t *config, unsigned long timeout);

z_owned_session_t z_open(z_owned_config_t *config);
int8_t z_close(z_owned_session_t *zs);

void z_info_peers_zid(const z_session_t *zs, z_owned_closure_zid_t *callback);
void z_info_routers_zid(const z_session_t *zs, z_owned_closure_zid_t *callback);
z_id_t z_info_zid(const z_session_t *session);

z_put_options_t z_put_options_default(void);
int8_t z_put(z_session_t *zs, z_keyexpr_t keyexpr, const uint8_t *payload, z_zint_t len, const z_put_options_t *opt);

z_get_options_t z_get_options_default(void);
int8_t z_get(z_session_t *zs, z_keyexpr_t keyexpr, const char *predicate, z_owned_closure_reply_t *callback, const z_get_options_t *options);

z_owned_keyexpr_t z_declare_keyexpr(z_session_t *zs, z_keyexpr_t keyexpr);
int8_t z_undeclare_keyexpr(z_session_t *zs, z_owned_keyexpr_t *keyexpr);

z_publisher_options_t z_publisher_options_default(void);
z_owned_publisher_t z_declare_publisher(z_session_t *zs, z_keyexpr_t keyexpr, z_publisher_options_t *options);
int8_t z_undeclare_publisher(z_owned_publisher_t *pub);

int8_t z_publisher_put(const z_publisher_t *pub, const uint8_t *payload, size_t len, const z_publisher_put_options_t *options);
int8_t z_publisher_delete(const z_publisher_t *pub);

z_subscriber_options_t z_subscriber_options_default(void);
z_owned_subscriber_t z_declare_subscriber(z_session_t *zs, z_keyexpr_t keyexpr, z_owned_closure_sample_t *callback, const z_subscriber_options_t *options);
int8_t z_undeclare_subscriber(z_owned_subscriber_t *sub);

z_owned_pull_subscriber_t z_declare_pull_subscriber(z_session_t *zs, z_keyexpr_t keyexpr, z_owned_closure_sample_t *callback, const z_subscriber_options_t *options);
int8_t z_undeclare_pull_subscriber(z_owned_pull_subscriber_t *sub);
int8_t z_pull(const z_pull_subscriber_t *sub);

z_queryable_options_t z_queryable_options_default(void);
z_owned_queryable_t z_declare_queryable(z_session_t *zs, z_keyexpr_t keyexpr, z_owned_closure_query_t *callback, const z_queryable_options_t *options);
int8_t z_undeclare_queryable(z_owned_queryable_t *queryable);

int8_t z_query_reply(const z_query_t *query, const z_keyexpr_t keyexpr, const uint8_t *payload, size_t len);
uint8_t z_reply_is_ok(const z_owned_reply_t *reply);
z_sample_t z_reply_ok(z_owned_reply_t *reply);

/************* Tasks **************/
int8_t zp_start_read_task(z_session_t *zs);
int8_t zp_stop_read_task(z_session_t *zs);

int8_t zp_start_lease_task(z_session_t *zs);
int8_t zp_stop_lease_task(z_session_t *zs);

#endif /* ZENOH_PICO_API_PRIMITIVES_H */
