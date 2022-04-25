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
    return (z_owned_config_t){.value = _z_properties_default()};
}

int z_config_insert(z_config_t *config, unsigned int key, z_string_t value)
{
    return _z_properties_insert(config, key, value);
}

z_owned_session_t z_open(z_owned_config_t config)
{
    z_owned_session_t zs;
    zs.value = _z_open(config.value);
    z_config_clear(config);
    return zs; 
}

void z_close(z_owned_session_t zs)
{
    _z_close(zs.value);
}

z_owned_info_t z_info(const z_session_t *zs)
{
    z_owned_info_t zi;
    zi.value = _z_info(zs);
    return zi;
}

//z_owned_string_t z_info_as_str(const z_session_t *zs)
//{
//    z_string_t *str = (z_string_t*)malloc(sizeof(z_string_t));
//    *str = (z_string_t){.val = NULL, .len = 0};
//
//    _z_properties_t *zi = _z_info(zs);
//
//    _z_string_t append;
//    _z_string_move_str(&append, "info_router_pid : ");
//    _z_string_append(str, &append);
//
//    append = _z_properties_get(zi, Z_INFO_PID_KEY);
//    _z_string_append(str, &append);
//
//    _z_string_move_str(&append, "\ninfo_pid : ");
//    _z_string_append(str, &append);
//
//    append = _z_properties_get(zi, Z_INFO_ROUTER_PID_KEY);
//    _z_string_append(str, &append);
//
//    _z_properties_free(&zi);
//
//    z_owned_string_t ret = {.value = str, .is_valid = 1};
//    return ret;
//}

z_str_t z_info_get(z_info_t *info, unsigned int key)
{
    return _z_properties_get(info, key);
}

void z_info_clear(z_owned_info_t zi)
{
    _z_properties_free(&zi.value);
    free(zi.value);
}

z_keyexpr_t z_expr(const z_str_t name)
{
    return _z_rname(name);
}

z_keyexpr_t z_id(const z_zint_t id)
{
    return _z_rid(id);
}

z_owned_keyexpr_t z_expr_new(const z_str_t name)
{
    z_owned_keyexpr_t key;
    key.value = (z_keyexpr_t*)malloc(sizeof(z_keyexpr_t));
    *key.value = _z_rname(name);
    return key;
}

z_owned_keyexpr_t z_id_with_suffix_new(const z_zint_t id, const z_str_t name)
{
    z_owned_keyexpr_t key;
    key.value = (z_keyexpr_t*)malloc(sizeof(z_keyexpr_t));
    *key.value = _z_rid_with_suffix(id, name);
    return key;
}

z_keyexpr_t z_declare_expr(z_session_t *zs, z_owned_keyexpr_t keyexpr)
{
    return _z_rid(_z_declare_resource(zs, *keyexpr.value));
}

void z_undeclare_expr(z_session_t *zs, z_keyexpr_t *keyexpr)
{
    _z_undeclare_resource(zs, keyexpr->rid);
}

z_owned_publisher_t z_declare_publication(z_session_t *zs, z_keyexpr_t *keyexpr)
{
    z_owned_publisher_t pub;
    pub.value = _z_declare_publisher(zs, *keyexpr);
    return pub;
}

z_encoding_t z_encoding_default(void)
{
    _z_encoding_t ret = {.prefix = Z_ENCODING_APP_OCTETSTREAM, .suffix = ""};
    return ret;
}

void z_encoding_clear(z_owned_encoding_t encoding)
{
    _z_str_clear(encoding.value->suffix);
}

void z_encoding_free(z_owned_encoding_t **encoding)
{
    z_owned_encoding_t *ptr = *encoding;
    z_encoding_clear(*ptr);

    free(ptr);
    *encoding = NULL;
}

z_owned_queryable_t z_queryable_new(z_session_t *zs, z_owned_keyexpr_t keyexpr, unsigned int kind, void (*callback)(const z_query_t*, const void*), void *arg)
{
    z_owned_queryable_t qable;
    qable.value = _z_declare_queryable(zs, *keyexpr.value, kind, callback, arg);
    return qable;
}

void z_queryable_close(z_owned_queryable_t queryable)
{
    _z_undeclare_queryable(queryable.value);
    free(queryable.value);
}

z_query_consolidation_t z_query_consolidation_auto(void)
{
    z_query_consolidation_t ret = {.tag = Z_QUERY_CONSOLIDATION_AUTO};
    return ret;
}

z_query_consolidation_t z_query_consolidation_default(void)
{
    return z_query_consolidation_reception();
}

z_query_consolidation_t z_query_consolidation_full(void)
{
    z_query_consolidation_t ret = {.tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                   .strategy.manual = {.first_routers = Z_CONSOLIDATION_MODE_FULL, .last_router = Z_CONSOLIDATION_MODE_FULL, .reception = Z_CONSOLIDATION_MODE_FULL}};
    return ret;
}

z_query_consolidation_t z_query_consolidation_last_router(void)
{
    z_query_consolidation_t ret = {.tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                   .strategy.manual = {.first_routers = Z_CONSOLIDATION_MODE_LAZY, .last_router = Z_CONSOLIDATION_MODE_FULL, .reception = Z_CONSOLIDATION_MODE_FULL}};
    return ret;
}

z_query_consolidation_t z_query_consolidation_lazy(void)
{
    z_query_consolidation_t ret = {.tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                   .strategy.manual = {.first_routers = Z_CONSOLIDATION_MODE_LAZY, .last_router = Z_CONSOLIDATION_MODE_LAZY, .reception = Z_CONSOLIDATION_MODE_LAZY}};
    return ret;
}

z_query_consolidation_t z_query_consolidation_none(void)
{
    z_query_consolidation_t ret = {.tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                   .strategy.manual = {.first_routers = Z_CONSOLIDATION_MODE_NONE, .last_router = Z_CONSOLIDATION_MODE_NONE, .reception = Z_CONSOLIDATION_MODE_NONE}};
    return ret;
}

z_query_consolidation_t z_query_consolidation_reception(void)
{
    z_query_consolidation_t ret = {.tag = Z_QUERY_CONSOLIDATION_MANUAL,
                                   .strategy.manual = {.first_routers = Z_CONSOLIDATION_MODE_LAZY, .last_router = Z_CONSOLIDATION_MODE_LAZY, .reception = Z_CONSOLIDATION_MODE_FULL}};
    return ret;
}

z_keyexpr_t z_query_key_expr(const z_query_t *query)
{
    return _z_rname(query->rname);
}

z_str_t z_query_predicate(const z_query_t *query)
{
    return query->predicate;
}

z_query_target_t z_query_target_default(void)
{
    return _z_query_target_default();
}

z_owned_reply_data_array_t z_get_collect(z_session_t *zs,
                                         z_owned_keyexpr_t keyexpr,
                                         const z_str_t predicate,
                                         z_query_target_t target,
                                         z_query_consolidation_t consolidation)
{    
    _z_consolidation_strategy_t strategy;
    if (consolidation.tag == Z_QUERY_CONSOLIDATION_MANUAL)
        strategy = consolidation.strategy.manual;
    else
    {
        // if (keyexpr.rname.)
        strategy = z_query_consolidation_default().strategy.manual;
        // QueryConsolidation::Auto => {
        //     if self.selector.has_time_range() {
        //         ConsolidationStrategy::none()
        //     } else {
        //         ConsolidationStrategy::default()
        //     }
        // }
    }

    z_owned_reply_data_array_t rda;
    rda.value = (z_reply_data_array_t*)malloc(sizeof(z_reply_data_array_t));
    *rda.value = _z_query_collect(zs, *keyexpr.value, predicate, target, strategy);
    return rda;
}

void z_send_reply(const z_query_t *query, const z_str_t key, const uint8_t *payload, size_t len)
{
    _z_send_reply(query, key, payload, len);
}

z_owned_hello_array_t z_scout(z_zint_t what, z_owned_config_t config, unsigned long timeout)
{
    z_owned_hello_array_t hellos;
    hellos.value = (z_hello_array_t*)malloc(sizeof(z_hello_array_t));
    *hellos.value = _z_scout(what, config.value, timeout);

    z_config_clear(config);

    return hellos; 
}

// void z_get(z_session_t *zs, z_keyexpr_t keyexpr, z_str_t predicate, z_query_target_t target, z_query_consolidation_t consolidation, void (*callback)(z_reply_t, const void*), void *arg)
// {
    // _z_reply_data_array_t replies = _z_query_collect(zs, keyexpr, predicate, target, consolidation);

    // for (unsigned int i = 0; i < replies.len; i++)
    //     callback(replies.val[i], arg);

    // _z_reply_data_array_free(replies);
// }

int z_put(z_session_t *zs, z_keyexpr_t *keyexpr, const uint8_t *payload, size_t len)
{
    return _z_write(zs, *keyexpr, (const uint8_t *)payload, len);
}

int z_put_ext(z_session_t *zs, z_keyexpr_t *keyexpr, const uint8_t *payload, z_zint_t len, const z_put_options_t *opt)
{
    return _z_write_ext(zs, *keyexpr, (const uint8_t *)payload, len, opt->encoding, opt->kind, opt->congestion_control);
}

z_put_options_t z_put_options_default(void)
{
    return (z_put_options_t) {.encoding = z_encoding_default(), .kind = Z_DATA_KIND_DEFAULT, .congestion_control = Z_CONGESTION_CONTROL_DROP, .priority = Z_PRIORITY_DATA};
}

z_owned_subscriber_t z_subscribe(z_session_t *zs, z_owned_keyexpr_t keyexpr, z_subinfo_t sub_info, void (*callback)(const z_sample_t*, const void*), void *arg)
{
    z_owned_subscriber_t sub;
    sub.value = _z_declare_subscriber(zs, *keyexpr.value, sub_info, callback, arg);
    return sub;
}

void z_pull(const z_subscriber_t *sub)
{
    _z_pull(sub);
}

void z_subscriber_close(z_owned_subscriber_t sub)
{
    _z_undeclare_subscriber(sub.value);
    free(sub.value);
}

void z_publisher_close(z_owned_publisher_t pub)
{
    _z_undeclare_publisher(pub.value);
    free(pub.value);
}

z_subinfo_t z_subinfo_default(void)
{
    return _z_subinfo_default();
}

/**************** Loans ****************/
z_bytes_t *z_bytes_loan(const z_owned_bytes_t *bytes)
{
    return bytes->value;
}

z_config_t *z_config_loan(const z_owned_config_t *config)
{
    return config->value;
}

z_encoding_t *z_encoding_loan(const z_owned_encoding_t *encoding)
{
    return encoding->value;
}

z_info_t *z_info_loan(const z_owned_info_t *info)
{
    return info->value;
}

z_keyexpr_t z_keyexpr_loan(const z_owned_keyexpr_t *key)
{
    return *key->value;
}

z_session_t *z_session_loan(const z_owned_session_t *session)
{
    return session->value;
}

z_sample_t *z_sample_loan(const z_owned_sample_t *sample)
{
    return sample->value;
}

z_string_t *z_string_loan(const z_owned_string_t *string)
{
    return string->value;
}

z_str_array_t *z_str_array_loan(const z_owned_str_array_t *str_a)
{
    return str_a->value;
}

z_hello_t *z_hello_loan(const z_owned_hello_t *hello)
{
    return hello->value;
}

z_hello_array_t *z_hello_array_loan(const z_owned_hello_array_t *hellos)
{
    return hellos->value;
}

z_subscriber_t *z_subscriber_loan(const z_owned_subscriber_t *subscriber)
{
    return subscriber->value;
}

z_publisher_t *z_publisher_loan(const z_owned_publisher_t *publisher)
{
    return publisher->value;
}

z_queryable_t *z_queryable_loan(const z_owned_queryable_t *queryable)
{
    return queryable->value;
}

/**************** Moves ****************/
z_owned_bytes_t z_bytes_move(z_owned_bytes_t *bytes)
{
    z_owned_bytes_t ret = {.value = bytes->value};
    bytes->value = NULL;
    return ret;
}

z_owned_config_t z_config_move(z_owned_config_t *config)
{
    z_owned_config_t ret = {.value = config->value};
    config->value = NULL;
    return ret;
}

z_owned_string_t z_string_move(z_owned_string_t *string)
{
    z_owned_string_t ret = {.value = string->value};
    string->value = NULL;
    return ret;
}

z_owned_encoding_t z_encoding_move(z_owned_encoding_t *encoding)
{
    z_owned_encoding_t ret = {.value = encoding->value};
    encoding->value = NULL;
    return ret;
}

z_owned_info_t z_info_move(z_owned_info_t *info)
{
    z_owned_info_t ret = {.value = info->value};
    info->value = NULL;
    return ret;
}

z_owned_keyexpr_t z_keyexpr_move(z_owned_keyexpr_t *key)
{
    z_owned_keyexpr_t ret = {.value = key->value};
    key->value = NULL;
    return ret;
}

z_owned_session_t z_session_move(z_owned_session_t *session)
{
    z_owned_session_t ret = {.value = session->value};
    session->value = NULL;
    return ret;
}

z_owned_sample_t z_sample_move(z_owned_sample_t *sample)
{
    z_owned_sample_t ret = {.value = sample->value};
    sample->value = NULL;
    return ret;
}

z_owned_str_array_t z_str_array_move(z_owned_str_array_t *str_a)
{
    z_owned_str_array_t ret = {.value = str_a->value};
    str_a->value = NULL;
    return ret;
}

z_owned_hello_t z_hello_move(z_owned_hello_t *hello)
{
    z_owned_hello_t ret = {.value = hello->value};
    hello->value = NULL;
    return ret;
}

z_owned_reply_data_array_t z_reply_data_array_move(z_owned_reply_data_array_t *reply_data_a)
{
    z_owned_reply_data_array_t ret = {.value = reply_data_a->value};
    reply_data_a->value = NULL;
    return ret;
}

z_owned_hello_array_t z_hello_array_move(z_owned_hello_array_t *hellos)
{
    z_owned_hello_array_t ret = {.value = hellos->value};
    hellos->value = NULL;
    return ret;
}

z_owned_subscriber_t z_subscriber_move(z_owned_subscriber_t *subscriber)
{
    z_owned_subscriber_t ret = {.value = subscriber->value};
    subscriber->value = NULL;
    return ret;
}

z_owned_publisher_t z_publisher_move(z_owned_publisher_t *publisher)
{
    z_owned_publisher_t ret = {.value = publisher->value};
    publisher->value = NULL;
    return ret;
}

z_owned_queryable_t z_queryable_move(z_owned_queryable_t *queryable)
{
    z_owned_queryable_t ret = {.value = queryable->value};
    queryable->value = NULL;
    return ret;
}

/*************** Checks ****************/
uint8_t z_bytes_check(const z_owned_bytes_t *bytes)
{
    return bytes->value != NULL;
}

uint8_t z_config_check(const z_owned_config_t *config)
{
    return config->value != NULL;
}

uint8_t z_encoding_check(const z_owned_encoding_t *encoding)
{
    return encoding->value != NULL;
}

uint8_t z_info_check(const z_owned_info_t *info)
{
    return info->value != NULL;
}

uint8_t z_keyexpr_check(const z_owned_keyexpr_t *key)
{
    return key->value != NULL;
}

uint8_t z_session_check(const z_owned_session_t *session)
{
    return session->value != NULL;
}

uint8_t z_sample_check(const z_owned_sample_t *sample)
{
    return sample->value != NULL;
}

uint8_t z_string_check(const z_owned_string_t *string)
{
    return string->value != NULL;
}

uint8_t z_str_array_check(const z_owned_str_array_t *str_a)
{
    return str_a->value != NULL;
}

uint8_t z_reply_data_array_check(const z_owned_reply_data_array_t *reply_data_a)
{
    return reply_data_a->value != NULL;
}

uint8_t z_hello_check(const z_owned_hello_t *hello)
{
    return hello->value != NULL;
}

uint8_t z_hello_array_check(const z_owned_hello_array_t *hellos)
{
    return hellos->value != NULL;
}

uint8_t z_subscriber_check(const z_owned_subscriber_t *subscriber)
{
    return subscriber->value != NULL;
}

uint8_t z_publisher_check(const z_owned_publisher_t *publisher)
{
    return publisher->value != NULL;
}

uint8_t z_queryable_check(const z_owned_queryable_t *queryable)
{
    return queryable->value != NULL;
}

/*************** Clears ****************/
void z_string_clear(z_owned_string_t string)
{
    _z_string_free(&string.value);
}

void z_config_clear(z_owned_config_t config)
{
    _z_properties_free(&config.value);
}

void z_keyexpr_clear(z_owned_keyexpr_t key)
{
    _z_reskey_free(&key.value);
}

void z_hello_array_clear(z_owned_hello_array_t hello_a)
{
    _z_hello_array_free(&hello_a.value);
}

void z_reply_data_array_clear(z_owned_reply_data_array_t reply_data_a)
{
    _z_reply_data_array_free(&reply_data_a.value);
}

/*************** Frees *****************/
void z_config_free(z_owned_config_t **config)
{
    z_owned_config_t *ptr = *config;
    z_config_clear(*ptr);

    free(ptr);
    *config = NULL;
}

void z_hello_array_free(z_owned_hello_array_t **hello_a)
{
    z_owned_hello_array_t *ptr = *hello_a;
    z_hello_array_clear(*ptr);

    free(ptr);
    *hello_a = NULL;
}

void z_reply_data_array_free(z_owned_reply_data_array_t **reply_data_a)
{
    z_owned_reply_data_array_t *ptr = *reply_data_a;
    z_reply_data_array_clear(*ptr);

    free(ptr);
    *reply_data_a = NULL;
}

void z_keyexpr_free(z_owned_keyexpr_t **key)
{
    z_owned_keyexpr_t *ptr = *key;
    z_keyexpr_clear(*ptr);

    free(ptr);
    *key = NULL;
}