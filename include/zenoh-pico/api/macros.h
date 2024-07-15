//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>

#ifndef ZENOH_PICO_API_MACROS_H
#define ZENOH_PICO_API_MACROS_H

#include "zenoh-pico/api/handlers.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"

#if ZENOH_C_STANDARD != 99

#ifndef __cplusplus

// clang-format off

/**
 * Defines a generic function for loaning any of the ``z_owned_X_t`` types.
 *
 * Parameters:
 *   x: The instance to loan.
 *
 * Returns:
 *   Returns the loaned type associated with `x`.
 */

#define z_loan(x) _Generic((x), \
                  z_owned_keyexpr_t : z_keyexpr_loan,                         \
                  z_view_keyexpr_t : z_view_keyexpr_loan,                     \
                  z_owned_config_t : z_config_loan,                           \
                  z_owned_session_t : z_session_loan,                         \
                  z_owned_subscriber_t : z_subscriber_loan,                   \
                  z_owned_publisher_t : z_publisher_loan,                     \
                  z_owned_queryable_t : z_queryable_loan,                     \
                  z_owned_reply_t : z_reply_loan,                             \
                  z_owned_hello_t : z_hello_loan,                             \
                  z_owned_string_t : z_string_loan,                           \
                  z_view_string_t : z_view_string_loan,                       \
                  z_owned_string_array_t : z_string_array_loan,               \
                  z_owned_sample_t : z_sample_loan,                           \
                  z_owned_query_t : z_query_loan,                             \
                  z_owned_slice_t : z_slice_loan,                             \
                  z_owned_bytes_t : z_bytes_loan,                             \
                  z_owned_encoding_t : z_encoding_loan,                       \
                  z_owned_task_t : z_task_loan,                               \
                  z_owned_mutex_t : z_mutex_loan,                             \
                  z_owned_condvar_t : z_condvar_loan,                         \
                  z_owned_fifo_handler_query_t : z_fifo_handler_query_loan,   \
                  z_owned_fifo_handler_reply_t : z_fifo_handler_reply_loan,   \
                  z_owned_fifo_handler_sample_t : z_fifo_handler_sample_loan, \
                  z_owned_ring_handler_query_t : z_ring_handler_query_loan,   \
                  z_owned_ring_handler_reply_t : z_ring_handler_reply_loan,   \
                  z_owned_ring_handler_sample_t : z_ring_handler_sample_loan, \
                  z_owned_reply_err_t : z_reply_err_loan,                     \
                  z_owned_closure_sample_t : z_closure_sample_loan,           \
                  z_owned_closure_reply_t : z_closure_reply_loan,             \
                  z_owned_closure_query_t : z_closure_query_loan,             \
                  z_owned_closure_hello_t : z_closure_hello_loan,             \
                  z_owned_closure_zid_t : z_closure_zid_loan                  \
            )(&x)

#define z_loan_mut(x) _Generic((x), \
                  z_owned_keyexpr_t : z_keyexpr_loan_mut,                 \
                  z_owned_config_t : z_config_loan_mut,                   \
                  z_owned_session_t : z_session_loan_mut,                 \
                  z_owned_publisher_t : z_publisher_loan_mut,             \
                  z_owned_queryable_t : z_queryable_loan_mut,             \
                  z_owned_reply_t : z_reply_loan_mut,                     \
                  z_owned_hello_t : z_hello_loan_mut,                     \
                  z_owned_string_t : z_string_loan_mut,                   \
                  z_view_string_t : z_view_string_loan_mut,               \
                  z_owned_string_array_t : z_string_array_loan_mut,       \
                  z_owned_sample_t : z_sample_loan_mut,                   \
                  z_owned_query_t : z_query_loan_mut,                     \
                  z_owned_slice_t : z_slice_loan_mut,                     \
                  z_owned_bytes_t : z_bytes_loan_mut,                     \
                  z_owned_task_t : z_task_loan_mut,                       \
                  z_owned_mutex_t : z_mutex_loan_mut,                     \
                  z_owned_condvar_t : z_condvar_loan_mut,                 \
                  z_owned_reply_err_t : z_reply_err_loan_mut              \
            )(&x)
/**
 * Defines a generic function for dropping any of the ``z_owned_X_t`` types.
 *
 * Parameters:
 *   x: The instance to drop.
 */
#define z_drop(x) _Generic((x), \
                  z_owned_keyexpr_t * : z_keyexpr_drop,                             \
                  z_owned_config_t * : z_config_drop,                               \
                  z_owned_session_t * : z_session_drop,                             \
                  z_owned_subscriber_t * : z_subscriber_drop,                       \
                  z_owned_publisher_t * : z_publisher_drop,                         \
                  z_owned_queryable_t * : z_queryable_drop,                         \
                  z_owned_reply_t * : z_reply_drop,                                 \
                  z_owned_hello_t * : z_hello_drop,                                 \
                  z_owned_string_t * : z_string_drop,                               \
                  z_owned_string_array_t * : z_string_array_drop,                   \
                  z_owned_sample_t * : z_sample_drop,                               \
                  z_owned_query_t * : z_query_drop,                                 \
                  z_owned_encoding_t * : z_encoding_drop,                           \
                  z_owned_slice_t  *: z_slice_drop,                                 \
                  z_owned_bytes_t  *: z_bytes_drop,                                 \
                  z_owned_closure_sample_t * : z_closure_sample_drop,               \
                  z_owned_closure_query_t * : z_closure_query_drop,                 \
                  z_owned_closure_reply_t * : z_closure_reply_drop,                 \
                  z_owned_closure_hello_t * : z_closure_hello_drop,                 \
                  z_owned_closure_zid_t * : z_closure_zid_drop,                     \
                  z_owned_mutex_t *: z_mutex_drop,                                  \
                  z_owned_condvar_t *: z_condvar_drop,                              \
                  z_owned_fifo_handler_query_t * : z_fifo_handler_query_drop,       \
                  z_owned_fifo_handler_reply_t * : z_fifo_handler_reply_drop,       \
                  z_owned_fifo_handler_sample_t * : z_fifo_handler_sample_drop,     \
                  z_owned_ring_handler_query_t * : z_ring_handler_query_drop,       \
                  z_owned_ring_handler_reply_t * : z_ring_handler_reply_drop,       \
                  z_owned_ring_handler_sample_t * : z_ring_handler_sample_drop,     \
                  z_owned_reply_err_t * : z_reply_err_drop                          \
            )(x)

/**
 * Defines a generic function for checking the validity of any of the ``z_owned_X_t`` types.
 *
 * Parameters:
 *   x: The instance to check.
 *
 * Returns:
 *   Returns ``true`` if valid, or ``false`` otherwise.
 */

#define z_check(x) _Generic((x), \
                  z_owned_keyexpr_t : z_keyexpr_check,                 \
                  z_view_keyexpr_t : z_view_keyexpr_check,             \
                  z_owned_reply_err_t : z_reply_err_check,             \
                  z_owned_config_t : z_config_check,                   \
                  z_owned_session_t : z_session_check,                 \
                  z_owned_subscriber_t : z_subscriber_check,           \
                  z_owned_publisher_t : z_publisher_check,             \
                  z_owned_queryable_t : z_queryable_check,             \
                  z_owned_reply_t : z_reply_check,                     \
                  z_owned_hello_t : z_hello_check,                     \
                  z_owned_string_t : z_string_check,                   \
                  z_view_string_t : z_view_string_check,               \
                  z_owned_string_array_t : z_string_array_check,       \
                  z_owned_slice_t : z_slice_check,                     \
                  z_owned_bytes_t : z_bytes_check,                     \
                  z_owned_sample_t : z_sample_check,                   \
                  z_owned_query_t : z_query_check,                     \
                  z_owned_encoding_t : z_encoding_check                \
            )(&x)

/**
 * Defines a generic function for calling closure stored in any of ``z_owned_closure_X_t``
 *
 * Parameters:
 *   x: The closure to call
 */
#define z_call(x, ...) \
    _Generic((x), z_loaned_closure_sample_t : z_closure_sample_call,                 \
                  z_loaned_closure_query_t : z_closure_query_call,                   \
                  z_loaned_closure_reply_t : z_closure_reply_call,                   \
                  z_loaned_closure_hello_t : z_closure_hello_call,                   \
                  z_loaned_closure_zid_t : z_closure_zid_call                        \
            ) (&x, __VA_ARGS__)

#define z_try_recv(x, ...) \
    _Generic((x), \
        const z_loaned_fifo_handler_query_t* : z_fifo_handler_query_try_recv, \
        const z_loaned_fifo_handler_reply_t* : z_fifo_handler_reply_try_recv, \
        const z_loaned_fifo_handler_sample_t* : z_fifo_handler_sample_try_recv, \
        const z_loaned_ring_handler_query_t* : z_ring_handler_query_try_recv, \
        const z_loaned_ring_handler_reply_t* : z_ring_handler_reply_try_recv, \
        const z_loaned_ring_handler_sample_t* : z_ring_handler_sample_try_recv \
    )(x, __VA_ARGS__)

#define z_recv(x, ...) \
    _Generic((x), \
        const z_loaned_fifo_handler_query_t* : z_fifo_handler_query_recv, \
        const z_loaned_fifo_handler_reply_t* : z_fifo_handler_reply_recv, \
        const z_loaned_fifo_handler_sample_t* : z_fifo_handler_sample_recv, \
        const z_loaned_ring_handler_query_t* : z_ring_handler_query_recv, \
        const z_loaned_ring_handler_reply_t* : z_ring_handler_reply_recv, \
        const z_loaned_ring_handler_sample_t* : z_ring_handler_sample_recv \
    )(x, __VA_ARGS__)

/**
 * Defines a generic function for moving any of the ``z_owned_X_t`` types.
 *
 * Parameters:
 *   x: The instance to move.
 *
 * Returns:
 *   Returns the instance associated with `x`.
 */
#define z_move(x) _Generic((x), \
                  z_owned_keyexpr_t : z_keyexpr_move,                             \
                  z_owned_config_t : z_config_move,                               \
                  z_owned_session_t : z_session_move,                             \
                  z_owned_subscriber_t : z_subscriber_move,                       \
                  z_owned_publisher_t : z_publisher_move,                         \
                  z_owned_queryable_t : z_queryable_move,                         \
                  z_owned_reply_t : z_reply_move,                                 \
                  z_owned_hello_t : z_hello_move,                                 \
                  z_owned_string_t : z_string_move,                               \
                  z_owned_string_array_t : z_string_array_move,                   \
                  z_owned_closure_sample_t : z_closure_sample_move,               \
                  z_owned_closure_query_t : z_closure_query_move,                 \
                  z_owned_closure_reply_t : z_closure_reply_move,                 \
                  z_owned_closure_hello_t : z_closure_hello_move,                 \
                  z_owned_closure_zid_t  : z_closure_zid_move,                    \
                  z_owned_sample_t : z_sample_move,                               \
                  z_owned_query_t : z_query_move,                                 \
                  z_owned_slice_t : z_slice_move,                                 \
                  z_owned_bytes_t : z_bytes_move,                                 \
                  z_owned_encoding_t : z_encoding_move,                           \
                  z_owned_task_t : z_task_move,                                   \
                  z_owned_mutex_t : z_mutex_move,                                 \
                  z_owned_condvar_t : z_condvar_move,                             \
                  z_owned_ring_handler_query_t : z_ring_handler_query_move,       \
                  z_owned_ring_handler_reply_t : z_ring_handler_reply_move,       \
                  z_owned_ring_handler_sample_t : z_ring_handler_sample_move,     \
                  z_owned_fifo_handler_query_t : z_fifo_handler_query_move,       \
                  z_owned_fifo_handler_reply_t : z_fifo_handler_reply_move,       \
                  z_owned_fifo_handler_sample_t : z_fifo_handler_sample_move,     \
                  z_owned_reply_err_t : z_reply_err_move                          \
            )(&x)

/**
 * Defines a generic function for cloning any of the ``z_owned_X_t`` types.
 *
 * Parameters:
 *   x: The clone storage.
 *   y: The instance to clone.
 *
 * Returns:
 *   Returns the cloned instance of `x`.
 */
#define z_clone(x, y) _Generic((x), \
                  z_owned_keyexpr_t : z_keyexpr_clone,                 \
                  z_owned_session_t : z_session_clone,                 \
                  z_owned_subscriber_t : z_subscriber_clone,           \
                  z_owned_publisher_t : z_publisher_clone,             \
                  z_owned_queryable_t : z_queryable_clone,             \
                  z_owned_query_t : z_query_clone,                     \
                  z_owned_sample_t : z_sample_clone,                   \
                  z_owned_bytes_t : z_bytes_clone,                     \
                  z_owned_encoding_t : z_encoding_clone,               \
                  z_owned_reply_err_t : z_reply_err_clone,             \
                  z_owned_reply_t : z_reply_clone,                     \
                  z_owned_hello_t : z_hello_clone,                     \
                  z_owned_string_t : z_string_clone,                   \
                  z_owned_string_array_t : z_string_array_clone        \
            )(&x, y)

/**
 * Defines a generic function for making null object of any of the ``z_owned_X_t`` types.
 *
 * Returns:
 *   Returns the uninitialized instance of `x`.
 */
#define z_null(x) _Generic((x), \
                  z_owned_session_t * : z_session_null,                             \
                  z_owned_publisher_t * : z_publisher_null,                         \
                  z_owned_keyexpr_t * : z_keyexpr_null,                             \
                  z_owned_config_t * : z_config_null,                               \
                  z_owned_subscriber_t * : z_subscriber_null,                       \
                  z_owned_queryable_t * : z_queryable_null,                         \
                  z_owned_query_t * : z_query_null,                                 \
                  z_owned_reply_t * : z_reply_null,                                 \
                  z_owned_hello_t * : z_hello_null,                                 \
                  z_owned_string_t * : z_string_null,                               \
                  z_owned_slice_t  *: z_slice_null,                                 \
                  z_owned_bytes_t  *: z_bytes_null,                                 \
                  z_owned_closure_sample_t * : z_closure_sample_null,               \
                  z_owned_closure_query_t * : z_closure_query_null,                 \
                  z_owned_closure_reply_t * : z_closure_reply_null,                 \
                  z_owned_closure_hello_t * : z_closure_hello_null,                 \
                  z_owned_closure_zid_t * : z_closure_zid_null,                     \
                  z_owned_sample_t * : z_sample_null,                               \
                  z_owned_encoding_t * : z_encoding_null,                           \
                  z_owned_reply_err_t * : z_reply_err_null                          \
            )(x)

// clang-format on

#define _z_closure_overloader(closure, callback, dropper, ctx, ...) \
    do {                                                            \
        (closure)->_val.call = callback;                            \
        (closure)->_val.drop = dropper;                             \
        (closure)->_val.context = ctx;                              \
    } while (0);

/**
 * Defines a variadic macro to ease the definition of callback closures.
 *
 * Parameters:
 *   callback: the typical ``callback`` function. ``context`` will be passed as its last argument.
 *   dropper: allows the callback's state to be freed. ``context`` will be passed as its last argument.
 *   context: a pointer to an arbitrary state.
 *
 * Returns:
 *   Returns the new closure.
 */
#define z_closure(...) _z_closure_overloader(__VA_ARGS__, 0, 0, 0)

#else  // __cplusplus

// clang-format off

// z_loan definition
inline const z_loaned_keyexpr_t* z_loan(const z_owned_keyexpr_t& x) { return z_keyexpr_loan(&x); }
inline const z_loaned_keyexpr_t* z_loan(const z_view_keyexpr_t& x) { return z_view_keyexpr_loan(&x); }
inline const z_loaned_config_t* z_loan(const z_owned_config_t& x) { return z_config_loan(&x); }
inline const z_loaned_session_t* z_loan(const z_owned_session_t& x) { return z_session_loan(&x); }
inline const z_loaned_subscriber_t* z_loan(const z_owned_subscriber_t& x) { return z_subscriber_loan(&x); }
inline const z_loaned_publisher_t* z_loan(const z_owned_publisher_t& x) { return z_publisher_loan(&x); }
inline const z_loaned_queryable_t* z_loan(const z_owned_queryable_t& x) { return z_queryable_loan(&x); }
inline const z_loaned_reply_t* z_loan(const z_owned_reply_t& x) { return z_reply_loan(&x); }
inline const z_loaned_hello_t* z_loan(const z_owned_hello_t& x) { return z_hello_loan(&x); }
inline const z_loaned_string_t* z_loan(const z_owned_string_t& x) { return z_string_loan(&x); }
inline const z_loaned_string_t* z_loan(const z_view_string_t& x) { return z_view_string_loan(&x); }
inline const z_loaned_string_array_t* z_loan(const z_owned_string_array_t& x) { return z_string_array_loan(&x); }
inline const z_loaned_sample_t* z_loan(const z_owned_sample_t& x) { return z_sample_loan(&x); }
inline const z_loaned_query_t* z_loan(const z_owned_query_t& x) { return z_query_loan(&x); }
inline const z_loaned_slice_t* z_loan(const z_owned_slice_t& x) { return z_slice_loan(&x); }
inline const z_loaned_bytes_t* z_loan(const z_owned_bytes_t& x) { return z_bytes_loan(&x); }
inline const z_loaned_bytes_writer_t* z_loan(const z_owned_bytes_writer_t& x) { return z_bytes_writer_loan(&x); }
inline const z_loaned_encoding_t* z_loan(const z_owned_encoding_t& x) { return z_encoding_loan(&x); }
inline const z_loaned_task_t* z_loan(const z_owned_task_t& x) { return z_task_loan(&x); }
inline const z_loaned_mutex_t* z_loan(const z_owned_mutex_t& x) { return z_mutex_loan(&x); }
inline const z_loaned_condvar_t* z_loan(const z_owned_condvar_t& x) { return z_condvar_loan(&x); }
inline const z_loaned_reply_err_t* z_loan(const z_owned_reply_err_t& x) { return z_reply_err_loan(&x); }
inline const z_loaned_closure_sample_t* z_loan(const z_owned_closure_sample_t& x) { return z_closure_sample_loan(&x); }
inline const z_loaned_closure_reply_t* z_loan(const z_owned_closure_reply_t& x) { return z_closure_reply_loan(&x); }
inline const z_loaned_closure_query_t* z_loan(const z_owned_closure_query_t& x) { return z_closure_query_loan(&x); }
inline const z_loaned_closure_hello_t* z_loan(const z_owned_closure_hello_t& x) { return z_closure_hello_loan(&x); }
inline const z_loaned_closure_zid_t* z_loan(const z_owned_closure_zid_t& x) { return z_closure_zid_loan(&x); }
inline const z_loaned_fifo_handler_query_t* z_loan(const z_owned_fifo_handler_query_t& x) { return z_fifo_handler_query_loan(&x); };
inline const z_loaned_fifo_handler_reply_t* z_loan(const z_owned_fifo_handler_reply_t& x) { return z_fifo_handler_reply_loan(&x); };
inline const z_loaned_fifo_handler_sample_t* z_loan(const z_owned_fifo_handler_sample_t& x) { return z_fifo_handler_sample_loan(&x); };
inline const z_loaned_ring_handler_query_t* z_loan(const z_owned_ring_handler_query_t& x) { return z_ring_handler_query_loan(&x); };
inline const z_loaned_ring_handler_reply_t* z_loan(const z_owned_ring_handler_reply_t& x) { return z_ring_handler_reply_loan(&x); };
inline const z_loaned_ring_handler_sample_t* z_loan(const z_owned_ring_handler_sample_t& x) { return z_ring_handler_sample_loan(&x); };

// z_loan_mut definition
inline z_loaned_keyexpr_t* z_loan_mut(z_owned_keyexpr_t& x) { return z_keyexpr_loan_mut(&x); }
inline z_loaned_keyexpr_t* z_loan_mut(z_view_keyexpr_t& x) { return z_view_keyexpr_loan_mut(&x); }
inline z_loaned_config_t* z_loan_mut(z_owned_config_t& x) { return z_config_loan_mut(&x); }
inline z_loaned_session_t* z_loan_mut(z_owned_session_t& x) { return z_session_loan_mut(&x); }
inline z_loaned_publisher_t* z_loan_mut(z_owned_publisher_t& x) { return z_publisher_loan_mut(&x); }
inline z_loaned_queryable_t* z_loan_mut(z_owned_queryable_t& x) { return z_queryable_loan_mut(&x); }
inline z_loaned_reply_t* z_loan_mut(z_owned_reply_t& x) { return z_reply_loan_mut(&x); }
inline z_loaned_hello_t* z_loan_mut(z_owned_hello_t& x) { return z_hello_loan_mut(&x); }
inline z_loaned_string_t* z_loan_mut(z_owned_string_t& x) { return z_string_loan_mut(&x); }
inline z_loaned_string_t* z_loan_mut(z_view_string_t& x) { return z_view_string_loan_mut(&x); }
inline z_loaned_string_array_t* z_loan_mut(z_owned_string_array_t& x) { return z_string_array_loan_mut(&x); }
inline z_loaned_sample_t* z_loan_mut(z_owned_sample_t& x) { return z_sample_loan_mut(&x); }
inline z_loaned_query_t* z_loan_mut(z_owned_query_t& x) { return z_query_loan_mut(&x); }
inline z_loaned_slice_t* z_loan_mut(z_owned_slice_t& x) { return z_slice_loan_mut(&x); }
inline z_loaned_bytes_t* z_loan_mut(z_owned_bytes_t& x) { return z_bytes_loan_mut(&x); }
inline z_loaned_bytes_writer_t* z_loan_mut(z_owned_bytes_writer_t& x) { return z_bytes_writer_loan_mut(&x); }
inline z_loaned_encoding_t* z_loan_mut(z_owned_encoding_t& x) { return z_encoding_loan_mut(&x); }
inline z_loaned_task_t* z_loan_mut(z_owned_task_t& x) { return z_task_loan_mut(&x); }
inline z_loaned_mutex_t* z_loan_mut(z_owned_mutex_t& x) { return z_mutex_loan_mut(&x); }
inline z_loaned_condvar_t* z_loan_mut(z_owned_condvar_t& x) { return z_condvar_loan_mut(&x); }
inline z_loaned_reply_err_t* z_loan_mut(z_owned_reply_err_t& x) { return z_reply_err_loan_mut(&x); }

// z_drop definition
inline int8_t z_drop(z_owned_session_t* v) { return z_close(v); }
inline int8_t z_drop(z_owned_publisher_t* v) { return z_undeclare_publisher(v); }
inline void z_drop(z_owned_keyexpr_t* v) { z_keyexpr_drop(v); }
inline void z_drop(z_owned_config_t* v) { z_config_drop(v); }
inline int8_t z_drop(z_owned_subscriber_t* v) { return z_undeclare_subscriber(v); }
inline int8_t z_drop(z_owned_queryable_t* v) { return z_undeclare_queryable(v); }
inline void z_drop(z_owned_reply_t* v) { z_reply_drop(v); }
inline void z_drop(z_owned_hello_t* v) { z_hello_drop(v); }
inline void z_drop(z_owned_string_t* v) { z_string_drop(v); }
inline void z_drop(z_owned_slice_t* v) { z_slice_drop(v); }
inline void z_drop(z_owned_string_array_t* v) { z_string_array_drop(v); }
inline void z_drop(z_owned_sample_t* v) { z_sample_drop(v); }
inline void z_drop(z_owned_query_t* v) { z_query_drop(v); }
inline void z_drop(z_owned_bytes_t* v) { z_bytes_drop(v); }
inline void z_drop(z_owned_bytes_writer_t* v) { z_bytes_writer_drop(v); }
inline void z_drop(z_owned_encoding_t* v) { z_encoding_drop(v); }
inline void z_drop(z_owned_mutex_t* v) { z_mutex_drop(v); }
inline void z_drop(z_owned_condvar_t* v) { z_condvar_drop(v); }
inline void z_drop(z_owned_reply_err_t* v) { z_reply_err_drop(v); }
inline void z_drop(z_owned_closure_sample_t* v) { z_closure_sample_drop(v); }
inline void z_drop(z_owned_closure_query_t* v) { z_closure_query_drop(v); }
inline void z_drop(z_owned_closure_reply_t* v) { z_closure_reply_drop(v); }
inline void z_drop(z_owned_closure_hello_t* v) { z_closure_hello_drop(v); }
inline void z_drop(z_owned_closure_zid_t* v) { z_closure_zid_drop(v); }
inline void z_drop(z_owned_ring_handler_sample_t* v) { z_ring_handler_sample_drop(v); }
inline void z_drop(z_owned_fifo_handler_sample_t* v) { z_fifo_handler_sample_drop(v); }
inline void z_drop(z_owned_ring_handler_query_t* v) { z_ring_handler_query_drop(v); }
inline void z_drop(z_owned_fifo_handler_query_t* v) { z_fifo_handler_query_drop(v); }
inline void z_drop(z_owned_ring_handler_reply_t* v) { z_ring_handler_reply_drop(v); }
inline void z_drop(z_owned_fifo_handler_reply_t* v) { z_fifo_handler_reply_drop(v); }

// z_null definition
inline void z_null(z_owned_session_t* v) { z_session_null(v); }
inline void z_null(z_owned_publisher_t* v) { z_publisher_null(v); }
inline void z_null(z_owned_keyexpr_t* v) { z_keyexpr_null(v); }
inline void z_null(z_owned_config_t* v) { z_config_null(v); }
inline void z_null(z_owned_subscriber_t* v) { z_subscriber_null(v); }
inline void z_null(z_owned_queryable_t* v) { z_queryable_null(v); }
inline void z_null(z_owned_query_t* v) { z_query_null(v); }
inline void z_null(z_owned_sample_t* v) { z_sample_null(v); }
inline void z_null(z_owned_reply_t* v) { z_reply_null(v); }
inline void z_null(z_owned_hello_t* v) { z_hello_null(v); }
inline void z_null(z_owned_string_t* v) { z_string_null(v); }
inline void z_null(z_owned_bytes_t* v) { z_bytes_null(v); }
inline void z_null(z_owned_bytes_writer_t* v) { z_bytes_writer_null(v); }
inline void z_null(z_owned_encoding_t* v) { z_encoding_null(v); }
inline void z_null(z_owned_reply_err_t* v) { z_reply_err_null(v); }
inline void z_null(z_owned_closure_sample_t* v) { z_closure_sample_null(v); }
inline void z_null(z_owned_closure_query_t* v) { z_closure_query_null(v); }
inline void z_null(z_owned_closure_reply_t* v) { z_closure_reply_null(v); }
inline void z_null(z_owned_closure_hello_t* v) { z_closure_hello_null(v); }
inline void z_null(z_owned_closure_zid_t* v) { z_closure_zid_null(v); }
inline void z_null(z_owned_ring_handler_query_t* v) { return z_ring_handler_query_null(v); };
inline void z_null(z_owned_ring_handler_reply_t* v) { return z_ring_handler_reply_null(v); };
inline void z_null(z_owned_ring_handler_sample_t* v) { return z_ring_handler_sample_null(v); };
inline void z_null(z_owned_fifo_handler_query_t* v) { return z_fifo_handler_query_null(v); };
inline void z_null(z_owned_fifo_handler_reply_t* v) { return z_fifo_handler_reply_null(v); };
inline void z_null(z_owned_fifo_handler_sample_t* v) { return z_fifo_handler_sample_null(v); };

// z_check definition
inline bool z_check(const z_owned_session_t& v) { return z_session_check(&v); }
inline bool z_check(const z_owned_publisher_t& v) { return z_publisher_check(&v); }
inline bool z_check(const z_owned_keyexpr_t& v) { return z_keyexpr_check(&v); }
inline bool z_check(const z_view_keyexpr_t& v) { return z_view_keyexpr_check(&v); }
inline bool z_check(const z_owned_config_t& v) { return z_config_check(&v); }
inline bool z_check(const z_owned_subscriber_t& v) { return z_subscriber_check(&v); }
inline bool z_check(const z_owned_queryable_t& v) { return z_queryable_check(&v); }
inline bool z_check(const z_owned_reply_t& v) { return z_reply_check(&v); }
inline bool z_check(const z_owned_query_t& v) { return z_query_check(&v); }
inline bool z_check(const z_owned_hello_t& v) { return z_hello_check(&v); }
inline bool z_check(const z_owned_string_t& v) { return z_string_check(&v); }
inline bool z_check(const z_view_string_t& v) { return z_view_string_check(&v); }
inline bool z_check(const z_owned_sample_t& v) { return z_sample_check(&v); }
inline bool z_check(const z_owned_bytes_t& v) { return z_bytes_check(&v); }
inline bool z_check(const z_owned_bytes_writer_t& v) { return z_bytes_writer_check(&v); }
inline bool z_check(const z_owned_encoding_t& v) { return z_encoding_check(&v); }
inline bool z_check(const z_owned_reply_err_t& v) { return z_reply_err_check(&v); }
inline bool z_check(const z_owned_fifo_handler_query_t& v) { return z_fifo_handler_query_check(&v); };
inline bool z_check(const z_owned_fifo_handler_reply_t& v) { return z_fifo_handler_reply_check(&v); };
inline bool z_check(const z_owned_fifo_handler_sample_t& v) { return z_fifo_handler_sample_check(&v); };
inline bool z_check(const z_owned_ring_handler_query_t& v) { return z_ring_handler_query_check(&v); };
inline bool z_check(const z_owned_ring_handler_reply_t& v) { return z_ring_handler_reply_check(&v); };
inline bool z_check(const z_owned_ring_handler_sample_t& v) { return z_ring_handler_sample_check(&v); };

// z_call definition
inline void z_call(const z_loaned_closure_sample_t &closure, const z_loaned_sample_t *sample) 
    { z_closure_sample_call(&closure, sample); }
inline void z_call(const z_loaned_closure_query_t &closure, const z_loaned_query_t *query)
    { z_closure_query_call(&closure, query); }
inline void z_call(const z_loaned_closure_reply_t &closure, const z_loaned_reply_t *reply)
    { z_closure_reply_call(&closure, reply); }
inline void z_call(const z_loaned_closure_hello_t &closure, const z_loaned_hello_t *hello)
    { z_closure_hello_call(&closure, hello); }
inline void z_call(const z_loaned_closure_zid_t &closure, const z_id_t *zid)
    { z_closure_zid_call(&closure, zid); }

inline void z_closure(
    z_owned_closure_hello_t* closure,
    void (*call)(const z_loaned_hello_t*, void*),
    void (*drop)(void*) = NULL,
    void *context = NULL) {
    closure->_val.context = context;
    closure->_val.drop = drop;
    closure->_val.call = call;
};
inline void z_closure(
    z_owned_closure_query_t* closure,
    void (*call)(const z_loaned_query_t*, void*),
    void (*drop)(void*) = NULL,
    void *context = NULL) {
    closure->_val.context = context;
    closure->_val.drop = drop;
    closure->_val.call = call;
};
inline void z_closure(
    z_owned_closure_reply_t* closure,
    void (*call)(const z_loaned_reply_t*, void*),
    void (*drop)(void*) = NULL,
    void *context = NULL) {
    closure->_val.context = context;
    closure->_val.drop = drop;
    closure->_val.call = call;
};
inline void z_closure(
    z_owned_closure_sample_t* closure,
    void (*call)(const z_loaned_sample_t*, void*),
    void (*drop)(void*) = NULL,
    void *context = NULL) {
    closure->_val.context = context;
    closure->_val.drop = drop;
    closure->_val.call = call;
};
inline void z_closure(
    z_owned_closure_zid_t* closure,
    void (*call)(const z_id_t*, void*),
    void (*drop)(void*) = NULL,
    void *context = NULL) {
    closure->_val.context = context;
    closure->_val.drop = drop;
    closure->_val.call = call;
};

inline bool z_try_recv(const z_loaned_fifo_handler_query_t* this_, z_owned_query_t* query) {
    return z_fifo_handler_query_try_recv(this_, query);
};
inline bool z_try_recv(const z_loaned_fifo_handler_reply_t* this_, z_owned_reply_t* reply) {
    return z_fifo_handler_reply_try_recv(this_, reply);
};
inline bool z_try_recv(const z_loaned_fifo_handler_sample_t* this_, z_owned_sample_t* sample) {
    return z_fifo_handler_sample_try_recv(this_, sample);
};
inline bool z_try_recv(const z_loaned_ring_handler_query_t* this_, z_owned_query_t* query) {
    return z_ring_handler_query_try_recv(this_, query);
};
inline bool z_try_recv(const z_loaned_ring_handler_reply_t* this_, z_owned_reply_t* reply) {
    return z_ring_handler_reply_try_recv(this_, reply);
};
inline bool z_try_recv(const z_loaned_ring_handler_sample_t* this_, z_owned_sample_t* sample) {
    return z_ring_handler_sample_try_recv(this_, sample);
};


inline bool z_recv(const z_loaned_fifo_handler_query_t* this_, z_owned_query_t* query) {
    return z_fifo_handler_query_recv(this_, query);
};
inline bool z_recv(const z_loaned_fifo_handler_reply_t* this_, z_owned_reply_t* reply) {
    return z_fifo_handler_reply_recv(this_, reply);
};
inline bool z_recv(const z_loaned_fifo_handler_sample_t* this_, z_owned_sample_t* sample) {
    return z_fifo_handler_sample_recv(this_, sample);
};
inline bool z_recv(const z_loaned_ring_handler_query_t* this_, z_owned_query_t* query) {
    return z_ring_handler_query_recv(this_, query);
};
inline bool z_recv(const z_loaned_ring_handler_reply_t* this_, z_owned_reply_t* reply) {
    return z_ring_handler_reply_recv(this_, reply);
};
inline bool z_recv(const z_loaned_ring_handler_sample_t* this_, z_owned_sample_t* sample) {
    return z_ring_handler_sample_recv(this_, sample);
};

// clang-format on

inline z_owned_bytes_t* z_move(z_owned_bytes_t& x) { return z_bytes_move(&x); };
inline z_owned_bytes_writer_t* z_move(z_owned_bytes_writer_t& x) { return z_bytes_writer_move(&x); };
inline z_owned_closure_hello_t* z_move(z_owned_closure_hello_t& closure) { return z_closure_hello_move(&closure); };
inline z_owned_closure_query_t* z_move(z_owned_closure_query_t& closure) { return z_closure_query_move(&closure); };
inline z_owned_closure_reply_t* z_move(z_owned_closure_reply_t& closure) { return z_closure_reply_move(&closure); };
inline z_owned_closure_sample_t* z_move(z_owned_closure_sample_t& closure) { return z_closure_sample_move(&closure); };
inline z_owned_closure_zid_t* z_move(z_owned_closure_zid_t& closure) { return z_closure_zid_move(&closure); };
inline z_owned_config_t* z_move(z_owned_config_t& x) { return z_config_move(&x); };
inline z_owned_encoding_t* z_move(z_owned_encoding_t& x) { return z_encoding_move(&x); };
inline z_owned_reply_err_t* z_move(z_owned_reply_err_t& x) { return z_reply_err_move(&x); };
inline z_owned_hello_t* z_move(z_owned_hello_t& x) { return z_hello_move(&x); };
inline z_owned_keyexpr_t* z_move(z_owned_keyexpr_t& x) { return z_keyexpr_move(&x); };
inline z_owned_publisher_t* z_move(z_owned_publisher_t& x) { return z_publisher_move(&x); };
inline z_owned_query_t* z_move(z_owned_query_t& x) { return z_query_move(&x); };
inline z_owned_queryable_t* z_move(z_owned_queryable_t& x) { return z_queryable_move(&x); };
inline z_owned_reply_t* z_move(z_owned_reply_t& x) { return z_reply_move(&x); };
inline z_owned_sample_t* z_move(z_owned_sample_t& x) { return z_sample_move(&x); };
inline z_owned_session_t* z_move(z_owned_session_t& x) { return z_session_move(&x); };
inline z_owned_slice_t* z_move(z_owned_slice_t& x) { return z_slice_move(&x); };
inline z_owned_string_array_t* z_move(z_owned_string_array_t& x) { return z_string_array_move(&x); };
inline z_owned_string_t* z_move(z_owned_string_t& x) { return z_string_move(&x); };
inline z_owned_subscriber_t* z_move(z_owned_subscriber_t& x) { return z_subscriber_move(&x); };
inline z_owned_fifo_handler_query_t* z_move(z_owned_fifo_handler_query_t& x) { return (&x); };
inline z_owned_fifo_handler_reply_t* z_move(z_owned_fifo_handler_reply_t& x) { return (&x); };
inline z_owned_fifo_handler_sample_t* z_move(z_owned_fifo_handler_sample_t& x) { return (&x); };
inline z_owned_ring_handler_query_t* z_move(z_owned_ring_handler_query_t& x) { return (&x); };
inline z_owned_ring_handler_reply_t* z_move(z_owned_ring_handler_reply_t& x) { return (&x); };
inline z_owned_ring_handler_sample_t* z_move(z_owned_ring_handler_sample_t& x) { return (&x); };

template <class T>
struct z_loaned_to_owned_type_t {};
template <class T>
struct z_owned_to_loaned_type_t {};
template <>
struct z_loaned_to_owned_type_t<z_loaned_bytes_t> {
    typedef z_owned_bytes_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_bytes_t> {
    typedef z_loaned_bytes_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_bytes_writer_t> {
    typedef z_owned_bytes_writer_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_bytes_writer_t> {
    typedef z_loaned_bytes_writer_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_config_t> {
    typedef z_owned_config_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_config_t> {
    typedef z_loaned_config_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_encoding_t> {
    typedef z_owned_encoding_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_encoding_t> {
    typedef z_loaned_encoding_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_reply_err_t> {
    typedef z_owned_reply_err_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_reply_err_t> {
    typedef z_loaned_reply_err_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_hello_t> {
    typedef z_owned_hello_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_hello_t> {
    typedef z_loaned_hello_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_keyexpr_t> {
    typedef z_owned_keyexpr_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_keyexpr_t> {
    typedef z_loaned_keyexpr_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_publisher_t> {
    typedef z_owned_publisher_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_publisher_t> {
    typedef z_loaned_publisher_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_query_t> {
    typedef z_owned_query_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_query_t> {
    typedef z_loaned_query_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_queryable_t> {
    typedef z_owned_queryable_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_queryable_t> {
    typedef z_loaned_queryable_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_reply_t> {
    typedef z_owned_reply_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_reply_t> {
    typedef z_loaned_reply_t type;
};
;
template <>
struct z_loaned_to_owned_type_t<z_loaned_sample_t> {
    typedef z_owned_sample_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_sample_t> {
    typedef z_loaned_sample_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_session_t> {
    typedef z_owned_session_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_session_t> {
    typedef z_loaned_session_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_slice_t> {
    typedef z_owned_slice_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_slice_t> {
    typedef z_loaned_slice_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_string_array_t> {
    typedef z_owned_string_array_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_string_array_t> {
    typedef z_loaned_string_array_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_string_t> {
    typedef z_owned_string_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_string_t> {
    typedef z_loaned_string_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_subscriber_t> {
    typedef z_owned_subscriber_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_subscriber_t> {
    typedef z_loaned_subscriber_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_closure_sample_t> {
    typedef z_loaned_closure_sample_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_closure_sample_t> {
    typedef z_owned_closure_sample_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_closure_reply_t> {
    typedef z_loaned_closure_reply_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_closure_reply_t> {
    typedef z_owned_closure_reply_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_closure_query_t> {
    typedef z_loaned_closure_query_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_closure_query_t> {
    typedef z_owned_closure_query_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_closure_hello_t> {
    typedef z_loaned_closure_hello_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_closure_hello_t> {
    typedef z_owned_closure_hello_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_closure_zid_t> {
    typedef z_loaned_closure_zid_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_closure_zid_t> {
    typedef z_owned_closure_zid_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_fifo_handler_query_t> {
    typedef z_owned_fifo_handler_query_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_fifo_handler_query_t> {
    typedef z_loaned_fifo_handler_query_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_fifo_handler_reply_t> {
    typedef z_owned_fifo_handler_reply_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_fifo_handler_reply_t> {
    typedef z_loaned_fifo_handler_reply_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_fifo_handler_sample_t> {
    typedef z_owned_fifo_handler_sample_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_fifo_handler_sample_t> {
    typedef z_loaned_fifo_handler_sample_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_ring_handler_query_t> {
    typedef z_owned_ring_handler_query_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_ring_handler_query_t> {
    typedef z_loaned_ring_handler_query_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_ring_handler_reply_t> {
    typedef z_owned_ring_handler_reply_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_ring_handler_reply_t> {
    typedef z_loaned_ring_handler_reply_t type;
};
template <>
struct z_loaned_to_owned_type_t<z_loaned_ring_handler_sample_t> {
    typedef z_owned_ring_handler_sample_t type;
};
template <>
struct z_owned_to_loaned_type_t<z_owned_ring_handler_sample_t> {
    typedef z_loaned_ring_handler_sample_t type;
};
#endif

#endif /* ZENOH_C_STANDARD != 99 */

#endif /* ZENOH_PICO_API_MACROS_H */
