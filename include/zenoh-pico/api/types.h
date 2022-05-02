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

/* Loaned types */
typedef _z_zint_t z_zint_t;
typedef _z_str_t z_str_t;
typedef _z_bytes_t z_bytes_t;

typedef _z_string_t z_string_t;
typedef _z_keyexpr_t z_keyexpr_t;

typedef _z_config_t z_config_t;
typedef _z_session_t z_session_t;
typedef _z_config_t z_info_t;
typedef _z_subscriber_t z_subscriber_t;
typedef _z_publisher_t z_publisher_t;
typedef _z_queryable_t z_queryable_t;

typedef _z_encoding_t z_encoding_t;
typedef _z_subinfo_t z_subinfo_t;
typedef _z_period_t z_period_t;
typedef _z_consolidation_strategy_t z_consolidation_strategy_t;
typedef _z_query_target_t z_query_target_t;
typedef _z_target_t z_target_t;
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

typedef _z_sample_t z_sample_t;
typedef _z_hello_t z_hello_t;
typedef _z_reply_t z_reply_t;
typedef _z_reply_data_t z_reply_data_t;

typedef _z_str_array_t z_str_array_t;
typedef _z_hello_array_t z_hello_array_t;
typedef _z_reply_data_array_t z_reply_data_array_t;

/* Owned types */
#define _OWNED_TYPE(type, name)  \
    typedef struct               \
    {                            \
        type *_value;            \
    } z_owned_##name##_t;

_OWNED_TYPE(z_str_t, str)
_OWNED_TYPE(z_bytes_t, bytes)

_OWNED_TYPE(z_string_t, string)
_OWNED_TYPE(z_keyexpr_t, keyexpr)

_OWNED_TYPE(z_config_t, config)
_OWNED_TYPE(z_session_t, session)
_OWNED_TYPE(z_info_t, info)
_OWNED_TYPE(z_subscriber_t, subscriber)
_OWNED_TYPE(z_publisher_t, publisher)
_OWNED_TYPE(z_queryable_t, queryable)

_OWNED_TYPE(z_encoding_t, encoding)
_OWNED_TYPE(z_subinfo_t, subinfo)
_OWNED_TYPE(z_period_t, period)
_OWNED_TYPE(z_consolidation_strategy_t, consolidation_strategy)
_OWNED_TYPE(z_query_target_t, query_target)
_OWNED_TYPE(z_target_t, target)
_OWNED_TYPE(z_query_consolidation_t, query_consolidation)
_OWNED_TYPE(z_put_options_t, put_options)

_OWNED_TYPE(z_sample_t, sample)
_OWNED_TYPE(z_hello_t, hello)
_OWNED_TYPE(z_reply_t, reply)
_OWNED_TYPE(z_reply_data_t, reply_data)

_OWNED_TYPE(z_str_array_t, str_array)
_OWNED_TYPE(z_hello_array_t, hello_array)
_OWNED_TYPE(z_reply_data_array_t, reply_data_array)

#endif /* ZENOH_PICO_API_TYPES_H */
