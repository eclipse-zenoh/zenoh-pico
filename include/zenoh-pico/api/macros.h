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

#define z_loan(x) _Generic((x),                                       \
    z_owned_bytes_t : z_bytes_loan,                                   \
    z_owned_string_t : z_string_loan,                                 \
    z_owned_keyexpr_t : z_keyexpr_loan,                               \
    z_owned_config_t : z_config_loan,                                 \
    z_owned_session_t : z_session_loan,                               \
    z_owned_info_t : z_info_loan,                                     \
    z_owned_subscriber_t : z_subscriber_loan,                         \
    z_owned_publisher_t : z_publisher_loan,                           \
    z_owned_queryable_t : z_queryable_loan,                           \
    z_owned_encoding_t : z_encoding_loan,                             \
    z_owned_period_t : z_period_loan,                                 \
    z_owned_consolidation_strategy_t : z_consolidation_strategy_loan, \
    z_owned_query_target_t : z_query_target_loan,                     \
    z_owned_query_consolidation_t : z_query_consolidation_loan,       \
    z_owned_put_options_t : z_put_options_loan,                       \
    z_owned_sample_t : z_sample_loan,                                 \
    z_owned_hello_t : z_hello_loan,                                   \
    z_owned_reply_t : z_reply_loan,                                   \
    z_owned_reply_data_t : z_reply_data_loan,                         \
    z_owned_str_array_t : z_str_array_loan,                           \
    z_owned_hello_array_t : z_hello_array_loan,                       \
    z_owned_reply_data_array_t : z_reply_data_array_loan)(&x)

#define z_drop(x) _Generic((*x),                                      \
    z_owned_bytes_t : z_bytes_drop,                                   \
    z_owned_string_t : z_string_drop,                                 \
    z_owned_keyexpr_t : z_keyexpr_drop,                               \
    z_owned_config_t : z_config_drop,                                 \
    z_owned_session_t : z_session_drop,                               \
    z_owned_info_t : z_info_drop,                                     \
    z_owned_subscriber_t : z_subscriber_drop,                         \
    z_owned_publisher_t : z_publisher_drop,                           \
    z_owned_queryable_t : z_queryable_drop,                           \
    z_owned_encoding_t : z_encoding_drop,                             \
    z_owned_period_t : z_period_drop,                                 \
    z_owned_consolidation_strategy_t : z_consolidation_strategy_drop, \
    z_owned_query_target_t : z_query_target_drop,                     \
    z_owned_query_consolidation_t : z_query_consolidation_drop,       \
    z_owned_put_options_t : z_put_options_drop,                       \
    z_owned_sample_t : z_sample_drop,                                 \
    z_owned_hello_t : z_hello_drop,                                   \
    z_owned_reply_t : z_reply_drop,                                   \
    z_owned_reply_data_t : z_reply_data_drop,                         \
    z_owned_str_array_t : z_str_array_drop,                           \
    z_owned_hello_array_t : z_hello_array_drop,                       \
    z_owned_reply_data_array_t : z_reply_data_array_drop)(x)

#define z_check(x) _Generic((x),                                       \
    z_owned_bytes_t : z_bytes_check,                                   \
    z_owned_string_t : z_string_check,                                 \
    z_owned_keyexpr_t : z_keyexpr_check,                               \
    z_owned_config_t : z_config_check,                                 \
    z_owned_session_t : z_session_check,                               \
    z_owned_info_t : z_info_check,                                     \
    z_owned_subscriber_t : z_subscriber_check,                         \
    z_owned_publisher_t : z_publisher_check,                           \
    z_owned_queryable_t : z_queryable_check,                           \
    z_owned_encoding_t : z_encoding_check,                             \
    z_owned_period_t : z_period_check,                                 \
    z_owned_consolidation_strategy_t : z_consolidation_strategy_check, \
    z_owned_query_target_t : z_query_target_check,                     \
    z_owned_query_consolidation_t : z_query_consolidation_check,       \
    z_owned_put_options_t : z_put_options_check,                       \
    z_owned_sample_t : z_sample_check,                                 \
    z_owned_hello_t : z_hello_check,                                   \
    z_owned_reply_t : z_reply_check,                                   \
    z_owned_reply_data_t : z_reply_data_check,                         \
    z_owned_str_array_t : z_str_array_check,                           \
    z_owned_hello_array_t : z_hello_array_check,                       \
    z_owned_reply_data_array_t : z_reply_data_array_check)(&x)

#define z_move(x) _Generic((x),                                       \
    z_owned_bytes_t : z_bytes_move,                                   \
    z_owned_string_t : z_string_move,                                 \
    z_owned_keyexpr_t : z_keyexpr_move,                               \
    z_owned_config_t : z_config_move,                                 \
    z_owned_session_t : z_session_move,                               \
    z_owned_info_t : z_info_move,                                     \
    z_owned_subscriber_t : z_subscriber_move,                         \
    z_owned_publisher_t : z_publisher_move,                           \
    z_owned_queryable_t : z_queryable_move,                           \
    z_owned_encoding_t : z_encoding_move,                             \
    z_owned_period_t : z_period_move,                                 \
    z_owned_consolidation_strategy_t : z_consolidation_strategy_move, \
    z_owned_query_target_t : z_query_target_move,                     \
    z_owned_query_consolidation_t : z_query_consolidation_move,       \
    z_owned_put_options_t : z_put_options_move,                       \
    z_owned_sample_t : z_sample_move,                                 \
    z_owned_hello_t : z_hello_move,                                   \
    z_owned_reply_t : z_reply_move,                                   \
    z_owned_reply_data_t : z_reply_data_move,                         \
    z_owned_str_array_t : z_str_array_move,                           \
    z_owned_hello_array_t : z_hello_array_move,                       \
    z_owned_reply_data_array_t : z_reply_data_array_move)(&x)

#define z_clone(x) _Generic((x),                                       \
    z_owned_bytes_t : z_bytes_clone,                                   \
    z_owned_string_t : z_string_clone,                                 \
    z_owned_keyexpr_t : z_keyexpr_clone,                               \
    z_owned_config_t : z_config_clone,                                 \
    z_owned_session_t : z_session_clone,                               \
    z_owned_info_t : z_info_clone,                                     \
    z_owned_subscriber_t : z_subscriber_clone,                         \
    z_owned_publisher_t : z_publisher_clone,                           \
    z_owned_queryable_t : z_queryable_clone,                           \
    z_owned_encoding_t : z_encoding_clone,                             \
    z_owned_period_t : z_period_clone,                                 \
    z_owned_consolidation_strategy_t : z_consolidation_strategy_clone, \
    z_owned_query_target_t : z_query_target_clone,                     \
    z_owned_query_consolidation_t : z_query_consolidation_clone,       \
    z_owned_put_options_t : z_put_options_clone,                       \
    z_owned_sample_t : z_sample_clone,                                 \
    z_owned_hello_t : z_hello_clone,                                   \
    z_owned_reply_t : z_reply_clone,                                   \
    z_owned_reply_data_t : z_reply_data_clone,                         \
    z_owned_str_array_t : z_str_array_clone,                           \
    z_owned_hello_array_t : z_hello_array_clone,                       \
    z_owned_reply_data_array_t : z_reply_data_array_clone)(&x)

#define _z_closure_overloader(callback, droper, ctx, ...) \
  {                                                       \
    .call = callback, .drop = droper, .context = ctx      \
  }
#define z_closure(...) _z_closure_overloader(__VA_ARGS__, 0, 0)

#endif /* ZENOH_C_STANDARD != 99 */

#endif /* ZENOH_PICO_API_MACROS_H */
