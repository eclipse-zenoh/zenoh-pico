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

#ifndef ZENOH_PICO_API_MACROS_H
#define ZENOH_PICO_API_MACROS_H

#include "zenoh-pico/api/types.h"
#include "zenoh-pico/api/primitives.h"

#if ZENOH_C_STANDARD != 99

#define z_loan(x) _Generic((*x),    z_owned_session_t          \
                                : z_session_loan,              \
                                    z_owned_keyexpr_t          \
                                : z_keyexpr_loan,              \
                                    z_owned_config_t           \
                                : z_config_loan,               \
                                    z_owned_string_t           \
                                : z_string_loan,               \
                                    z_owned_bytes_t            \
                                : z_bytes_loan,                \
                                    z_owned_info_t             \
                                : z_info_loan,                 \
                                    z_owned_encoding_t         \
                                : z_encoding_loan,             \
                                    z_owned_subscriber_t       \
                                : z_subscriber_loan,           \
                                    z_owned_reply_data_array_t \
                                : z_reply_data_array_loan,     \
                                    z_owned_hello_array_t      \
                                : z_hello_array_loan)(x)

#define z_clear(x) _Generic((*x),   z_owned_session_t           \
                                : z_close,                      \
                                    z_owned_keyexpr_t           \
                                : z_keyexpr_clear,              \
                                    z_owned_config_t            \
                                : z_config_clear,               \
                                    z_owned_string_t            \
                                : z_string_clear,               \
                                    z_owned_bytes_t             \
                                : z_bytes_clear,                \
                                    z_owned_info_t              \
                                : z_info_clear,                 \
                                    z_owned_subscriber_t        \
                                : z_subscriber_clear,           \
                                    z_owned_encoding_t          \
                                : z_encoding_clear,             \
                                    z_owned_reply_data_array_t  \
                                : z_reply_data_array_clear,     \
                                    z_owned_hello_array_t       \
                                : z_hello_array_clear)(x)

#define z_check(x) _Generic((*x),   z_owned_session_t           \
                                : z_session_check,              \
                                    z_owned_keyexpr_t           \
                                : z_keyexpr_check,              \
                                    z_owned_config_t            \
                                : z_config_check,               \
                                    z_owned_string_t            \
                                : z_string_check,               \
                                    z_owned_bytes_t             \
                                : z_bytes_check,                \
                                    z_owned_info_t              \
                                : z_info_check,                 \
                                    z_owned_subscriber_t        \
                                : z_subscriber_check,           \
                                    z_owned_publisher_t         \
                                : z_publisher_check,            \
                                    z_owned_queryable_t         \
                                : z_queryable_check,            \
                                    z_owned_encoding_t          \
                                : z_encoding_check,             \
                                    z_owned_reply_data_array_t  \
                                : z_reply_data_array_check,     \
                                    z_owned_hello_array_t       \
                                : z_hello_array_check)(x)

#define z_move(x) _Generic((*x),    z_owned_session_t          \
                                : z_session_move,              \
                                    z_owned_keyexpr_t          \
                                : z_keyexpr_move,              \
                                    z_owned_config_t           \
                                : z_config_move,               \
                                    z_owned_string_t           \
                                : z_string_move,               \
                                    z_owned_bytes_t            \
                                : z_bytes_move,                \
                                    z_owned_info_t             \
                                : z_info_move,                 \
                                    z_owned_subscriber_t       \
                                : z_subscriber_move,           \
                                    z_owned_publisher_t        \
                                : z_publisher_move,            \
                                    z_owned_queryable_t        \
                                : z_queryable_move,            \
                                    z_owned_encoding_t         \
                                : z_encoding_move,             \
                                    z_owned_reply_data_array_t \
                                : z_reply_data_array_move,     \
                                    z_owned_hello_array_t      \
                                : z_hello_array_move)(x)
#endif

#endif /* ZENOH_PICO_API_MACROS_H */
