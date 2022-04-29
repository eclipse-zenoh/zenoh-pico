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

#ifndef ZENOH_PICO_API_TYPES_H
#define ZENOH_PICO_API_TYPES_H

#include "zenoh-pico/net/session.h"
#include "zenoh-pico/net/subscribe.h"
#include "zenoh-pico/net/publish.h"
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/protocol/core.h"

typedef _z_zint_t z_zint_t;

typedef _z_str_t z_str_t;
typedef struct
{
    z_str_t _value;
} z_owned_str_t;

typedef _z_str_array_t z_str_array_t;
typedef struct
{
    z_str_array_t *_value;
} z_owned_str_array_t;

typedef _z_bytes_t z_bytes_t;
typedef struct
{
    z_bytes_t *_value;
} z_owned_bytes_t;

typedef _z_string_t z_string_t;
typedef struct
{
    z_string_t *_value;
} z_owned_string_t;

typedef _z_encoding_t z_encoding_t;
typedef struct
{
    z_encoding_t *_value;
} z_owned_encoding_t;

typedef _z_reskey_t z_keyexpr_t;
typedef struct
{
    z_keyexpr_t *_value;
} z_owned_keyexpr_t;

typedef _z_sample_t z_sample_t;
typedef struct
{
    z_sample_t *_value;
} z_owned_sample_t;

typedef _z_reply_data_t z_reply_data_t;
typedef struct
{
    z_reply_data_t *_value;
} z_owned_reply_data_t;

typedef _z_reply_t z_reply_t;
typedef struct
{
    z_reply_t *_value;
} z_owned_reply_t;

typedef _z_reply_data_array_t z_reply_data_array_t;
typedef struct
{
    z_reply_data_array_t *_value;
} z_owned_reply_data_array_t;

typedef _z_hello_t z_hello_t;
typedef struct
{
    z_hello_t *_value;
} z_owned_hello_t;

typedef _z_hello_array_t z_hello_array_t;
typedef struct
{
    z_hello_array_t *_value;
} z_owned_hello_array_t;

typedef _z_properties_t z_config_t;
typedef struct
{
    z_config_t *_value;
} z_owned_config_t;

typedef _z_session_t z_session_t;
typedef struct
{
    z_session_t *_value;
} z_owned_session_t;

typedef struct
{
    z_publisher_t *_value;
} z_owned_publisher_t;

typedef struct
{
    z_subscriber_t *_value;
} z_owned_subscriber_t;

typedef struct
{
    z_queryable_t *_value;
} z_owned_queryable_t;

typedef _z_target_t z_target_t;
typedef _z_query_target_t z_query_target_t;
typedef _z_subinfo_t z_subinfo_t;
typedef _z_period_t z_period_t;

typedef _z_properties_t z_info_t;
typedef struct
{
    z_info_t *_value;
} z_owned_info_t;

typedef _z_consolidation_strategy_t z_consolidation_strategy_t;

typedef struct
{
    z_query_consolidation_tag_t _tag;
    union
    {
        z_consolidation_strategy_t _manual;
    } _strategy;
} z_query_consolidation_t;

typedef struct
{
    z_encoding_t encoding;
    uint8_t kind;
    uint8_t congestion_control;
    uint8_t priority;
} z_put_options_t;

#endif /* ZENOH_PICO_API_TYPES_H */
