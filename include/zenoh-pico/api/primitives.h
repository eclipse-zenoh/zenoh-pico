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
void z_bytes_clear(z_owned_bytes_t b);
void z_bytes_free(z_owned_bytes_t **b);

z_owned_string_t z_string_new(const z_str_t s);
void z_string_clear(z_owned_string_t s);
void z_string_free(z_owned_string_t **s);

z_keyexpr_t z_expr(const z_str_t name);
z_keyexpr_t z_id(z_zint_t id);
z_keyexpr_t z_id_with_suffix(z_zint_t id, const z_str_t suffix);
z_keyexpr_t z_keyexpr_new_loaned(z_zint_t id, const z_str_t suffix);
z_owned_keyexpr_t z_expr_new(const z_str_t name);
z_owned_keyexpr_t z_id_with_suffix_new(z_zint_t id, const z_str_t suffix);
z_owned_keyexpr_t z_keyexpr_new(z_zint_t id, const z_str_t suffix);
void z_keyexpr_clear(z_owned_keyexpr_t keyexpr);
void z_keyexpr_free(z_owned_keyexpr_t **keyexpr);

z_owned_config_t z_config_new(void);
z_owned_config_t z_config_empty(void);
z_owned_config_t z_config_default(void);
z_owned_config_t z_config_client(const char *const *peers, z_zint_t n_peers);
z_owned_config_t z_config_peer(void);
z_owned_config_t z_config_from_file(const char *path);
z_owned_config_t z_config_from_str(const char *str);
z_owned_string_t z_config_get(z_config_t *config, z_string_t key);
z_string_t z_config_to_str(z_config_t *config);
int z_config_insert(z_config_t *config, unsigned int key, z_string_t value);
//int z_config_insert_json(z_config_t config, z_string_t key, z_string_t value);
void z_config_clear(z_owned_config_t config);
void z_config_free(z_owned_config_t **config);

z_encoding_t z_encoding_default(void);
void z_encoding_clear(z_owned_encoding_t encoding);
void z_encoding_free(z_owned_encoding_t **encoding);

z_subinfo_t z_subinfo_default(void);
const z_period_t z_subinfo_period(const z_subinfo_t *info);

z_target_t z_target_default(void);
z_query_target_t z_query_target_default(void);
z_query_consolidation_t z_query_consolidation_auto(void);
z_query_consolidation_t z_query_consolidation_default(void);
z_query_consolidation_t z_query_consolidation_full(void);
z_query_consolidation_t z_query_consolidation_last_router(void);
z_query_consolidation_t z_query_consolidation_lazy(void);
z_query_consolidation_t z_query_consolidation_none(void);
z_query_consolidation_t z_query_consolidation_reception(void);
z_keyexpr_t z_query_key_expr(const z_query_t *query);
z_str_t z_query_predicate(const z_query_t *query);

void z_reply_data_array_clear(z_owned_reply_data_array_t replies);
void z_reply_data_array_free(z_owned_reply_data_array_t **replies);

void z_reply_data_clear(z_owned_reply_data_t reply_data);
void z_reply_data_free(z_owned_reply_data_t **reply_data);

void z_reply_clear(z_owned_reply_t reply);
void z_reply_free(z_owned_reply_t **reply);

void z_sample_clear(z_owned_sample_t sample);
void z_sample_free(z_owned_sample_t **sample);

void z_hello_array_clear(z_owned_hello_array_t hellos);
void z_hello_array_free(z_owned_hello_array_t **hellos);

void z_hello_clear(z_owned_hello_t hello);
void z_hello_free(z_owned_hello_t **hello);

void z_str_array_clear(z_owned_str_array_t strs);
void z_str_array_free(z_owned_str_array_t **strs);

/************* Primitives **************/
z_owned_hello_array_t z_scout(z_zint_t what, z_owned_config_t config, unsigned long timeout);

z_owned_session_t z_open(z_owned_config_t config);
void z_close(z_owned_session_t zs);

z_owned_info_t z_info(const z_session_t *zs);
z_owned_string_t z_info_as_str(const z_session_t *zs);
z_str_t z_info_get(z_info_t *info, unsigned int key);
void z_info_clear(z_owned_info_t zi);
void z_info_free(z_owned_info_t **zi);

z_keyexpr_t z_declare_expr(z_session_t *zs, z_owned_keyexpr_t keyexpr);
void z_undeclare_expr(z_session_t *zs, z_keyexpr_t *keyexpr);

z_owned_publisher_t z_declare_publication(z_session_t *zs, z_keyexpr_t *keyexpr);
void z_publisher_close(z_owned_publisher_t sub);
int z_put(z_session_t *zs, z_keyexpr_t *keyexpr, const uint8_t *payload, z_zint_t len);
int z_put_ext(z_session_t *zs, z_keyexpr_t *keyexpr, const uint8_t *payload, z_zint_t len, const z_put_options_t *opt);
z_put_options_t z_put_options_default(void);

z_owned_subscriber_t z_subscribe(z_session_t *zs, z_owned_keyexpr_t keyexpr, z_subinfo_t sub_info, void (*callback)(const z_sample_t*, const void*), void *arg);
void z_pull(const z_subscriber_t *sub);
void z_subscriber_close(z_owned_subscriber_t sub);

// void z_get(z_session_t *zs, z_owned_keyexpr_t keyexpr, const z_str_t predicate, z_query_target_t target, z_query_consolidation_t consolidation, void (*callback)(z_owned_reply_t, const void*), void *arg);

z_owned_reply_data_array_t z_get_collect(z_session_t *zs, z_owned_keyexpr_t keyexpr, const z_str_t predicate, z_query_target_t target, z_query_consolidation_t consolidation);

z_owned_queryable_t z_queryable_new(z_session_t *zs, z_owned_keyexpr_t keyexpr, unsigned int kind, void (*callback)(const z_query_t*, const void*), void *arg);
void z_queryable_close(z_owned_queryable_t queryable);
void z_send_reply(const z_query_t *query, const z_str_t key, const uint8_t *payload, size_t len);

/************** Ownership **************/
uint8_t z_bytes_check(const z_owned_bytes_t *bytes);
z_bytes_t *z_bytes_loan(const z_owned_bytes_t *bytes);
z_owned_bytes_t z_bytes_move(z_owned_bytes_t *bytes);

z_keyexpr_t z_keyexpr_loan(const z_owned_keyexpr_t *keyexpr);
uint8_t z_keyexpr_check(const z_owned_keyexpr_t *keyexpr);
z_owned_keyexpr_t z_keyexpr_move(z_owned_keyexpr_t *keyexpr);

uint8_t z_string_check(const z_owned_string_t *string);
z_string_t *z_string_loan(const z_owned_string_t *string);
z_owned_string_t z_string_move(z_owned_string_t *string);

uint8_t z_encoding_check(const z_owned_encoding_t *encoding);
z_encoding_t *z_encoding_loan(const z_owned_encoding_t *encoding);
z_owned_encoding_t z_encoding_move(z_owned_encoding_t *encoding);

uint8_t z_sample_check(const z_owned_sample_t *sample);
z_sample_t *z_sample_loan(const z_owned_sample_t *sample);
z_owned_sample_t z_sample_move(z_owned_sample_t *sample);

uint8_t z_config_check(const z_owned_config_t *config);
z_config_t *z_config_loan(const z_owned_config_t *config);
z_owned_config_t z_config_move(z_owned_config_t *config);

uint8_t z_session_check(const z_owned_session_t *session);
z_session_t *z_session_loan(const z_owned_session_t *session);
z_owned_session_t z_session_move(z_owned_session_t *session);

uint8_t z_info_check(const z_owned_info_t *info);
z_info_t *z_info_loan(const z_owned_info_t *info);
z_owned_info_t z_info_move(z_owned_info_t *info);

uint8_t z_publisher_check(const z_owned_publisher_t *publisher);
z_publisher_t *z_publisher_loan(const z_owned_publisher_t *publisher);
z_owned_publisher_t z_publisher_move(z_owned_publisher_t *publisher);

uint8_t z_subscriber_check(const z_owned_subscriber_t *subscriber);
z_subscriber_t *z_subscriber_loan(const z_owned_subscriber_t *subscriber);
z_owned_subscriber_t z_subscriber_move(z_owned_subscriber_t *subscriber);

uint8_t z_queryable_check(const z_owned_queryable_t *queryable);
z_queryable_t *z_queryable_loan(const z_owned_queryable_t *queryable);
z_owned_queryable_t z_queryable_move(z_owned_queryable_t *queryable);

uint8_t z_str_array_check(const z_owned_str_array_t *str_a);
z_str_array_t *z_str_array_loan(const z_owned_str_array_t *str_a);
z_owned_str_array_t z_str_array_move(z_owned_str_array_t *str_a);

uint8_t z_hello_check(const z_owned_hello_t *hello);
z_hello_t *z_hello_loan(const z_owned_hello_t *hello);
z_owned_hello_t z_hello_move(z_owned_hello_t *hello);

uint8_t z_reply_data_array_check(const z_owned_reply_data_array_t *replies);
z_reply_data_array_t *z_reply_data_array_loan(const z_owned_reply_data_array_t *replies);
z_owned_reply_data_array_t z_reply_data_array_move(z_owned_reply_data_array_t *replies);

uint8_t z_hello_array_check(const z_owned_hello_array_t *hellos);
z_hello_array_t *z_hello_array_loan(const z_owned_hello_array_t *hellos);
z_owned_hello_array_t z_hello_array_move(z_owned_hello_array_t *hellos);

uint8_t z_sample_check(const z_owned_sample_t *sample);
uint8_t z_reply_check(const z_owned_reply_t *reply);
uint8_t z_reply_data_check(const z_owned_reply_data_t *reply_data);
uint8_t z_reply_data_array_check(const z_owned_reply_data_array_t *replies);
uint8_t z_hello_array_check(const z_owned_hello_array_t *hellos);
uint8_t z_hello_check(const z_owned_hello_t *hello);
uint8_t z_str_array_check(const z_owned_str_array_t *strs);

#endif /* ZENOH_PICO_API_PRIMITIVES_H */
