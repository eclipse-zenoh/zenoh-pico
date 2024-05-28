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
                  z_owned_keyexpr_t : z_keyexpr_loan,                 \
                  z_view_keyexpr_t : z_view_keyexpr_loan,             \
                  z_owned_config_t : z_config_loan,                   \
                  z_owned_scouting_config_t : z_scouting_config_loan, \
                  z_owned_session_t : z_session_loan,                 \
                  z_owned_publisher_t : z_publisher_loan,             \
                  z_owned_reply_t : z_reply_loan,                     \
                  z_owned_hello_t : z_hello_loan,                     \
                  z_owned_string_t : z_string_loan,                   \
                  z_view_string_t : z_view_string_loan,               \
                  z_owned_string_array_t : z_string_array_loan,       \
                  z_owned_sample_t : z_sample_loan,                   \
                  z_owned_query_t : z_query_loan,                     \
                  z_owned_bytes_t : z_bytes_loan                      \
            )(&x)

#define z_loan_mut(x) _Generic((x), \
                  z_owned_keyexpr_t : z_keyexpr_loan_mut,                 \
                  z_owned_config_t : z_config_loan_mut,                   \
                  z_owned_scouting_config_t : z_scouting_config_loan_mut, \
                  z_owned_session_t : z_session_loan_mut,                 \
                  z_owned_publisher_t : z_publisher_loan_mut,             \
                  z_owned_reply_t : z_reply_loan_mut,                     \
                  z_owned_hello_t : z_hello_loan_mut,                     \
                  z_owned_string_t : z_string_loan_mut,                   \
                  z_view_string_t : z_view_string_loan_mut,               \
                  z_owned_string_array_t : z_string_array_loan_mut,       \
                  z_owned_sample_t : z_sample_loan_mut,                   \
                  z_owned_query_t : z_query_loan_mut,                     \
                  z_owned_bytes_t : z_bytes_loan_mut                      \
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
                  z_owned_scouting_config_t * : z_scouting_config_drop,             \
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
                  z_owned_closure_sample_t * : z_closure_sample_drop,               \
                  z_owned_closure_owned_sample_t * : z_closure_owned_sample_drop,   \
                  z_owned_closure_query_t * : z_closure_query_drop,                 \
                  z_owned_closure_reply_t * : z_closure_reply_drop,                 \
                  z_owned_closure_hello_t * : z_closure_hello_drop,                 \
                  z_owned_closure_zid_t * : z_closure_zid_drop,                     \
                  z_owned_sample_ring_channel_t * : z_sample_ring_channel_drop,     \
                  z_owned_sample_fifo_channel_t * : z_sample_fifo_channel_drop,     \
                  z_owned_query_ring_channel_t * : z_query_ring_channel_drop,       \
                  z_owned_query_fifo_channel_t * : z_query_fifo_channel_drop,       \
                  z_owned_reply_ring_channel_t * : z_reply_ring_channel_drop,       \
                  z_owned_reply_fifo_channel_t * : z_reply_fifo_channel_drop        \
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
                  z_view_keyexpr_t : z_keyexpr_is_initialized,         \
                  z_owned_value_t : z_value_check,                     \
                  z_owned_config_t : z_config_check,                   \
                  z_owned_scouting_config_t : z_scouting_config_check, \
                  z_owned_session_t : z_session_check,                 \
                  z_owned_subscriber_t : z_subscriber_check,           \
                  z_owned_publisher_t : z_publisher_check,             \
                  z_owned_queryable_t : z_queryable_check,             \
                  z_owned_reply_t : z_reply_check,                     \
                  z_owned_hello_t : z_hello_check,                     \
                  z_owned_string_t : z_string_check,                   \
                  z_owned_string_array_t : z_string_array_check,       \
                  z_owned_bytes_t : z_bytes_check,                     \
                  z_owned_sample_t : z_sample_check,                   \
                  z_owned_query_t : z_query_check                      \
            )(&x)

/**
 * Defines a generic function for calling closure stored in any of ``z_owned_closure_X_t``
 *
 * Parameters:
 *   x: The closure to call
 */
#define z_call(x, ...) \
    _Generic((x), z_owned_closure_sample_t : z_closure_sample_call,                 \
                  z_owned_closure_query_t : z_closure_query_call,                   \
                  z_owned_closure_reply_t : z_closure_reply_call,                   \
                  z_owned_closure_hello_t : z_closure_hello_call,                   \
                  z_owned_closure_zid_t : z_closure_zid_call,                       \
                  z_owned_closure_owned_sample_t : z_closure_owned_sample_call,     \
                  z_owned_closure_owned_query_t : z_closure_owned_query_call,       \
                  z_owned_closure_owned_reply_t : z_closure_owned_reply_call        \
            ) (&x, __VA_ARGS__)

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
                  z_owned_scouting_config_t : z_scouting_config_move,             \
                  z_owned_session_t : z_session_move,                             \
                  z_owned_subscriber_t : z_subscriber_move,                       \
                  z_owned_publisher_t : z_publisher_move,                         \
                  z_owned_queryable_t : z_queryable_move,                         \
                  z_owned_reply_t : z_reply_move,                                 \
                  z_owned_hello_t : z_hello_move,                                 \
                  z_owned_string_t : z_string_move,                               \
                  z_owned_string_array_t : z_string_array_move,                   \
                  z_owned_closure_sample_t : z_closure_sample_move,               \
                  z_owned_closure_owned_sample_t : z_closure_owned_sample_move,   \
                  z_owned_closure_query_t : z_closure_query_move,                 \
                  z_owned_closure_owned_query_t : z_closure_owned_query_move,     \
                  z_owned_closure_reply_t : z_closure_reply_move,                 \
                  z_owned_closure_hello_t : z_closure_hello_move,                 \
                  z_owned_closure_zid_t  : z_closure_zid_move,                    \
                  z_owned_sample_t : z_sample_move,                               \
                  z_owned_query_t : z_query_move,                                 \
                  z_owned_bytes_t : z_bytes_move,                                 \
                  z_owned_sample_ring_channel_t : z_sample_ring_channel_move,     \
                  z_owned_sample_fifo_channel_t : z_sample_fifo_channel_move,     \
                  z_owned_query_ring_channel_t : z_query_ring_channel_move,       \
                  z_owned_query_fifo_channel_t : z_query_fifo_channel_move,       \
                  z_owned_reply_ring_channel_t : z_reply_ring_channel_move,       \
                  z_owned_reply_fifo_channel_t : z_reply_fifo_channel_move        \
            )(&x)

/**
 * Defines a generic function for cloning any of the ``z_owned_X_t`` types.
 *
 * Parameters:
 *   x: The instance to clone.
 *
 * Returns:
 *   Returns the cloned instance of `x`.
 */
#define z_clone(x) _Generic((x), \
                  z_owned_keyexpr_t : z_keyexpr_clone,                 \
                  z_owned_config_t : z_config_clone,                   \
                  z_owned_session_t : z_session_clone,                 \
                  z_owned_subscriber_t : z_subscriber_clone,           \
                  z_owned_publisher_t : z_publisher_clone,             \
                  z_owned_queryable_t : z_queryable_clone,             \
                  z_owned_reply_t : z_reply_clone,                     \
                  z_owned_hello_t : z_hello_clone,                     \
                  z_owned_string_t : z_string_clone,                   \
                  z_owned_string_array_t : z_string_array_clone        \
            )(&x)

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
                  z_owned_scouting_config_t * : z_scouting_config_null,             \
                  z_owned_subscriber_t * : z_subscriber_null,                       \
                  z_owned_queryable_t * : z_queryable_null,                         \
                  z_owned_query_t * : z_query_null,                                 \
                  z_owned_reply_t * : z_reply_null,                                 \
                  z_owned_hello_t * : z_hello_null,                                 \
                  z_owned_string_t * : z_string_null,                               \
                  z_owned_closure_sample_t * : z_closure_sample_null,               \
                  z_owned_closure_owned_sample_t * : z_closure_owned_sample_null,   \
                  z_owned_closure_query_t * : z_closure_query_null,                 \
                  z_owned_closure_owned_query_t * : z_closure_owned_query_null,     \
                  z_owned_closure_reply_t * : z_closure_reply_null,                 \
                  z_owned_closure_hello_t * : z_closure_hello_null,                 \
                  z_owned_closure_zid_t * : z_closure_zid_null,                     \
                  z_owned_sample_t * : z_sample_null                                \
            )(x)

// clang-format on

#define _z_closure_overloader(closure, callback, dropper, ctx, ...) \
    do {                                                            \
        (closure)->call = callback;                                 \
        (closure)->drop = dropper;                                  \
        (closure)->context = ctx;                                   \
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

#else

// TODO(sashacmc): z_loan_mut

// clang-format off
template<class T> struct zenoh_loan_type { typedef T type; };
template<class T> inline typename zenoh_loan_type<T>::type z_loan(const T&);

template<> struct zenoh_loan_type<z_owned_session_t>{ typedef z_session_t type; };
template<> struct zenoh_loan_type<z_owned_keyexpr_t>{ typedef z_keyexpr_t type; };
template<> struct zenoh_loan_type<z_owned_config_t>{ typedef z_config_t type; };
template<> struct zenoh_loan_type<z_owned_publisher_t>{ typedef z_publisher_t type; };
template<> struct zenoh_loan_type<z_owned_hello_t>{ typedef z_hello_t type; };
template<> struct zenoh_loan_type<z_owned_string_t>{  typedef const char* type; };
template<> struct zenoh_loan_type<z_view_string_t>{  typedef z_loaned_string_t type; };

template<> inline z_session_t z_loan(const z_owned_session_t& x) { return z_session_loan(&x); }
template<> inline z_keyexpr_t z_loan(const z_owned_keyexpr_t& x) { return z_keyexpr_loan(&x); }
template<> inline z_config_t z_loan(const z_owned_config_t& x) { return z_config_loan(&x); }
template<> inline z_publisher_t z_loan(const z_owned_publisher_t& x) { return z_publisher_loan(&x); }
template<> inline z_hello_t z_loan(const z_owned_hello_t& x) { return z_hello_loan(&x); }
template<> inline const char* z_loan(const z_owned_string_t& x) { return z_string_loan(&x); }
template<> inline z_loaned_string_t z_loan(const z_view_string_t& x) { return z_view_string_loan(&x); }

template<class T> struct zenoh_drop_type { typedef T type; };
template<class T> inline typename zenoh_drop_type<T>::type z_drop(T*);

template<> struct zenoh_drop_type<z_owned_session_t> { typedef int8_t type; };
template<> struct zenoh_drop_type<z_owned_publisher_t> { typedef int8_t type; };
template<> struct zenoh_drop_type<z_owned_keyexpr_t> { typedef void type; };
template<> struct zenoh_drop_type<z_owned_config_t> { typedef void type; };
template<> struct zenoh_drop_type<z_owned_scouting_config_t> { typedef void type; };
template<> struct zenoh_drop_type<z_owned_subscriber_t> { typedef int8_t type; };
template<> struct zenoh_drop_type<z_owned_queryable_t> { typedef int8_t type; };
template<> struct zenoh_drop_type<z_owned_reply_t> { typedef void type; };
template<> struct zenoh_drop_type<z_owned_hello_t> { typedef void type; };
template<> struct zenoh_drop_type<z_owned_string_t> { typedef void type; };
template<> struct zenoh_drop_type<z_owned_closure_sample_t> { typedef void type; };
template<> struct zenoh_drop_type<z_owned_clusure_owned_sample_t> { typedef void type; };
template<> struct zenoh_drop_type<z_owned_closure_query_t> { typedef void type; };
template<> struct zenoh_drop_type<z_owned_closure_reply_t> { typedef void type; };
template<> struct zenoh_drop_type<z_owned_closure_hello_t> { typedef void type; };
template<> struct zenoh_drop_type<z_owned_closure_zid_t> { typedef void type; };
template<> struct zenoh_drop_type<z_owned_sample_ring_channel_t> { typedef void type; };
template<> struct zenoh_drop_type<z_owned_sample_fifo_channel_t> { typedef void type; };

template<> inline int8_t z_drop(z_owned_session_t* v) { return z_close(v); }
template<> inline int8_t z_drop(z_owned_publisher_t* v) { return z_undeclare_publisher(v); }
template<> inline void z_drop(z_owned_keyexpr_t* v) { z_keyexpr_drop(v); }
template<> inline void z_drop(z_owned_config_t* v) { z_config_drop(v); }
template<> inline void z_drop(z_owned_scouting_config_t* v) { z_scouting_config_drop(v); }
template<> inline int8_t z_drop(z_owned_subscriber_t* v) { return z_undeclare_subscriber(v); }
template<> inline int8_t z_drop(z_owned_queryable_t* v) { return z_undeclare_queryable(v); }
template<> inline void z_drop(z_owned_reply_t* v) { z_reply_drop(v); }
template<> inline void z_drop(z_owned_hello_t* v) { z_hello_drop(v); }
template<> inline void z_drop(z_owned_string_t* v) { z_string_drop(v); }
template<> inline void z_drop(z_owned_closure_sample_t* v) { z_closure_sample_drop(v); }
template<> inline void z_drop(z_owned_closure_owned_sample_t* v) { z_closure_owned_sample_drop(v); }
template<> inline void z_drop(z_owned_closure_query_t* v) { z_closure_query_drop(v); }
template<> inline void z_drop(z_owned_closure_reply_t* v) { z_closure_reply_drop(v); }
template<> inline void z_drop(z_owned_closure_hello_t* v) { z_closure_hello_drop(v); }
template<> inline void z_drop(z_owned_closure_zid_t* v) { z_closure_zid_drop(v); }
template<> inline void z_drop(z_owned_sample_ring_channel_t* v) { z_owned_sample_ring_channel_drop(v); }
template<> inline void z_drop(z_owned_sample_fifo_channel_t* v) { z_owned_sample_fifo_channel_drop(v); }
template<> inline void z_drop(z_owned_query_ring_channel_t* v) { z_owned_query_ring_channel_drop(v); }
template<> inline void z_drop(z_owned_query_fifo_channel_t* v) { z_owned_query_fifo_channel_drop(v); }
template<> inline void z_drop(z_owned_reply_ring_channel_t* v) { z_owned_reply_ring_channel_drop(v); }
template<> inline void z_drop(z_owned_reply_fifo_channel_t* v) { z_owned_reply_fifo_channel_drop(v); }

inline void z_null(z_owned_session_t* v) { z_session_null(v); }
inline void z_null(z_owned_publisher_t* v) { z_publisher_null(v); }
inline void z_null(z_owned_keyexpr_t* v) { z_keyexpr_null(v); }
inline void z_null(z_owned_config_t* v) { z_config_null(v); }
inline void z_null(z_owned_scouting_config_t* v) { z_scouting_config_null(v); }
inline void z_null(z_owned_subscriber_t* v) { z_subscriber_null(v); }
inline void z_null(z_owned_queryable_t* v) { z_queryable_null(v); }
inline void z_null(z_owned_reply_t* v) { z_reply_null(v); }
inline void z_null(z_owned_hello_t* v) { z_hello_null(v); }
inline void z_null(z_owned_string_t* v) { z_string_null(v); }
inline void z_null(z_owned_closure_sample_t* v) { z_closure_sample_null(v); }
inline void z_null(z_owned_clusure_owned_sample_t* v) { z_closure_owned_sample_null(v); }
inline void z_null(z_owned_closure_query_t* v) { z_closure_query_null(v); }
inline void z_null(z_owned_clusure_owned_query_t* v) { z_closure_owned_query_null(v); }
inline void z_null(z_owned_closure_reply_t* v) { z_closure_reply_null(v); }
inline void z_null(z_owned_closure_hello_t* v) { z_closure_hello_null(v); }
inline void z_null(z_owned_closure_zid_t* v) { z_closure_zid_null(v); }

inline bool z_check(const z_owned_session_t& v) { return z_session_check(&v); }
inline bool z_check(const z_owned_publisher_t& v) { return z_publisher_check(&v); }
inline bool z_check(const z_owned_keyexpr_t& v) { return z_keyexpr_check(&v); }
inline bool z_check(const z_keyexpr_t& v) { return z_keyexpr_is_initialized(&v); }
inline bool z_check(const z_owned_config_t& v) { return z_config_check(&v); }
inline bool z_check(const z_owned_scouting_config_t& v) { return z_scouting_config_check(&v); }
inline bool z_check(const z_bytes_t& v) { return z_bytes_check(&v); }
inline bool z_check(const z_owned_subscriber_t& v) { return z_subscriber_check(&v); }
inline bool z_check(const z_owned_queryable_t& v) { return z_queryable_check(&v); }
inline bool z_check(const z_owned_reply_t& v) { return z_reply_check(&v); }
inline bool z_check(const z_owned_hello_t& v) { return z_hello_check(&v); }
inline bool z_check(const z_owned_string_t& v) { return z_string_check(&v); }
inline bool z_check(const z_owned_string_t& v) { return z_sample_check(&v); }

inline void z_call(const z_owned_closure_sample_t &closure, const z_loaned_sample_t *sample) 
    { z_closure_sample_call(&closure, sample); }
inline void z_call(const z_owned_closure_owned_sample_t &closure, const z_owned_sample_t *sample) 
    { z_closure_owned_sample_call(&closure, sample); }
inline void z_call(const z_owned_closure_query_t &closure, const z_loaned_query_t *query)
    { z_closure_query_call(&closure, query); }
inline void z_call(const z_owned_closure_owned_query_t &closure, const z_owned_query_t *query) 
    { z_closure_owned_query_call(&closure, query); }
inline void z_call(const z_owned_closure_reply_t &closure, const z_loaned_reply_t *reply)
    { z_closure_reply_call(&closure, reply); }
inline void z_call(const z_owned_closure_owned_reply_t &closure, z_owned_reply_t *reply)
    { z_closure_owned_reply_call(&closure, reply); }
inline void z_call(const z_owned_closure_hello_t &closure, z_owned_hello_t *hello)
    { z_closure_hello_call(&closure, hello); }
inline void z_call(const z_owned_closure_zid_t &closure, const z_id_t *zid)
    { z_closure_zid_call(&closure, zid); }
// clang-format on

#define _z_closure_overloader(closure, callback, dropper, ctx, ...)              \
    do {                                                                         \
        (closure)->call = callback;                                              \
        (closure)->drop = dropper;                                               \
        (closure)->context = const_cast<void *>(static_cast<const void *>(ctx)); \
    } while (0);

#define z_closure(...) _z_closure_overloader(__VA_ARGS__, NULL, NULL)

#define z_move(x) (&x)

#endif

#endif /* ZENOH_C_STANDARD != 99 */

#endif /* ZENOH_PICO_API_MACROS_H */
