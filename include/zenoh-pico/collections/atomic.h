//
// Copyright (c) 2026 ZettaScale Technology
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
//

#ifndef ZENOH_PICO_COLLECTIONS_ATOMIC_H
#define ZENOH_PICO_COLLECTIONS_ATOMIC_H
#include <stdbool.h>
#include <stdint.h>

#include "zenoh-pico/config.h"

#if Z_FEATURE_MULTI_THREAD == 1
#if ZENOH_C_STANDARD != 99
#ifndef __cplusplus
#include <stdatomic.h>
#define _z_memory_order_t memory_order
#define _z_memory_order_relaxed memory_order_relaxed
#define _z_memory_order_acquire memory_order_acquire
#define _z_memory_order_release memory_order_release
#define _z_memory_order_seq_cst memory_order_seq_cst

#define _ZP_DEFINE_ATOMIC(type, name)                                                                               \
    typedef _Atomic(type) _z_atomic_##name##_t;                                                                     \
    static inline void _z_atomic_##name##_init(_z_atomic_##name##_t *var, type value) { atomic_init(var, value); }  \
    static inline type _z_atomic_##name##_load(_z_atomic_##name##_t *var, _z_memory_order_t order) {                \
        return atomic_load_explicit(var, order);                                                                    \
    }                                                                                                               \
    static inline void _z_atomic_##name##_store(_z_atomic_##name##_t *var, type val, _z_memory_order_t order) {     \
        atomic_store_explicit(var, val, order);                                                                     \
    }                                                                                                               \
    static inline type _z_atomic_##name##_fetch_add(_z_atomic_##name##_t *var, type val, _z_memory_order_t order) { \
        return atomic_fetch_add_explicit(var, val, order);                                                          \
    }                                                                                                               \
    static inline type _z_atomic_##name##_fetch_sub(_z_atomic_##name##_t *var, type val, _z_memory_order_t order) { \
        return atomic_fetch_sub_explicit(var, val, order);                                                          \
    }                                                                                                               \
    static inline bool _z_atomic_##name##_compare_exchange_strong(_z_atomic_##name##_t *var, type *expected,        \
                                                                  type desired, _z_memory_order_t success,          \
                                                                  _z_memory_order_t failure) {                      \
        (void)success;                                                                                              \
        (void)failure;                                                                                              \
        return atomic_compare_exchange_strong_explicit(var, expected, desired, success, failure);                   \
    }                                                                                                               \
    static inline bool _z_atomic_##name##_compare_exchange_weak(_z_atomic_##name##_t *var, type *expected,          \
                                                                type desired, _z_memory_order_t success,            \
                                                                _z_memory_order_t failure) {                        \
        (void)success;                                                                                              \
        (void)failure;                                                                                              \
        return atomic_compare_exchange_weak_explicit(var, expected, desired, success, failure);                     \
    }
static inline void _z_atomic_thread_fence(_z_memory_order_t order) { atomic_thread_fence(order); }
#else
#include <atomic>

#define _z_memory_order_t std::memory_order
#define _z_memory_order_relaxed std::memory_order_relaxed
#define _z_memory_order_acquire std::memory_order_acquire
#define _z_memory_order_release std::memory_order_release
#define _z_memory_order_seq_cst std::memory_order_seq_cst

#define _ZP_DEFINE_ATOMIC(type, name)                                                                               \
    typedef std::atomic<type> _z_atomic_##name##_t;                                                                 \
    static inline void _z_atomic_##name##_init(_z_atomic_##name##_t *var, type value) {                             \
        var->store(value, _z_memory_order_relaxed);                                                                 \
    }                                                                                                               \
    static inline type _z_atomic_##name##_load(_z_atomic_##name##_t *var, _z_memory_order_t order) {                \
        return var->load(order);                                                                                    \
    }                                                                                                               \
    static inline void _z_atomic_##name##_store(_z_atomic_##name##_t *var, type val, _z_memory_order_t order) {     \
        var->store(val, order);                                                                                     \
    }                                                                                                               \
    static inline type _z_atomic_##name##_fetch_add(_z_atomic_##name##_t *var, type val, _z_memory_order_t order) { \
        return var->fetch_add(val, order);                                                                          \
    }                                                                                                               \
    static inline type _z_atomic_##name##_fetch_sub(_z_atomic_##name##_t *var, type val, _z_memory_order_t order) { \
        return var->fetch_sub(val, order);                                                                          \
    }                                                                                                               \
    static inline bool _z_atomic_##name##_compare_exchange_strong(_z_atomic_##name##_t *var, type *expected,        \
                                                                  type desired, _z_memory_order_t success,          \
                                                                  _z_memory_order_t failure) {                      \
        return var->compare_exchange_strong(*expected, desired, success, failure);                                  \
    }                                                                                                               \
    static inline bool _z_atomic_##name##_compare_exchange_weak(_z_atomic_##name##_t *var, type *expected,          \
                                                                type desired, _z_memory_order_t success,            \
                                                                _z_memory_order_t failure) {                        \
        return var->compare_exchange_weak(*expected, desired, success, failure);                                    \
    }
static inline void _z_atomic_thread_fence(_z_memory_order_t order) { std::atomic_thread_fence(order); }
#endif
#else
#ifdef ZENOH_COMPILER_GCC
#define _z_memory_order_t int
#define _z_memory_order_relaxed __ATOMIC_RELAXED
#define _z_memory_order_acquire __ATOMIC_ACQUIRE
#define _z_memory_order_release __ATOMIC_RELEASE
#define _z_memory_order_seq_cst __ATOMIC_SEQ_CST

#define _ZP_DEFINE_ATOMIC(type, name)                                                                               \
    typedef type _z_atomic_##name##_t;                                                                              \
    static inline void _z_atomic_##name##_init(_z_atomic_##name##_t *var, type value) {                             \
        __atomic_store_n(var, value, _z_memory_order_relaxed);                                                      \
    }                                                                                                               \
    static inline type _z_atomic_##name##_load(_z_atomic_##name##_t *var, _z_memory_order_t order) {                \
        return __atomic_load_n(var, order);                                                                         \
    }                                                                                                               \
    static inline void _z_atomic_##name##_store(_z_atomic_##name##_t *var, type val, _z_memory_order_t order) {     \
        __atomic_store_n(var, val, order);                                                                          \
    }                                                                                                               \
    static inline type _z_atomic_##name##_fetch_add(_z_atomic_##name##_t *var, type val, _z_memory_order_t order) { \
        return __atomic_fetch_add(var, val, order);                                                                 \
    }                                                                                                               \
    static inline type _z_atomic_##name##_fetch_sub(_z_atomic_##name##_t *var, type val, _z_memory_order_t order) { \
        return __atomic_fetch_sub(var, val, order);                                                                 \
    }                                                                                                               \
    static inline bool _z_atomic_##name##_compare_exchange_strong(_z_atomic_##name##_t *var, type *expected,        \
                                                                  type desired, _z_memory_order_t success,          \
                                                                  _z_memory_order_t failure) {                      \
        return __atomic_compare_exchange_n(var, expected, desired, false, success, failure);                        \
    }                                                                                                               \
    static inline bool _z_atomic_##name##_compare_exchange_weak(_z_atomic_##name##_t *var, type *expected,          \
                                                                type desired, _z_memory_order_t success,            \
                                                                _z_memory_order_t failure) {                        \
        return __atomic_compare_exchange_n(var, expected, desired, true, success, failure);                         \
    }
static inline void _z_atomic_thread_fence(_z_memory_order_t order) { __atomic_thread_fence(order); }
#else
#error "Atomic operations in C99 only exists for GCC, use GCC or C11 or deactivate multi-thread"
#endif
#endif
#else
#define _z_memory_order_t int
#define _z_memory_order_relaxed 0
#define _z_memory_order_acquire 1
#define _z_memory_order_release 2
#define _z_memory_order_seq_cst 3

#define _ZP_DEFINE_ATOMIC(type, name)                                                                               \
    typedef type _z_atomic_##name##_t;                                                                              \
    static inline void _z_atomic_##name##_init(_z_atomic_##name##_t *var, type value) { *var = value; }             \
    static inline type _z_atomic_##name##_load(_z_atomic_##name##_t *var, _z_memory_order_t order) {                \
        (void)order;                                                                                                \
        return *var;                                                                                                \
    }                                                                                                               \
    static inline void _z_atomic_##name##_store(_z_atomic_##name##_t *var, type val, _z_memory_order_t order) {     \
        (void)order;                                                                                                \
        *var = val;                                                                                                 \
    }                                                                                                               \
    static inline type _z_atomic_##name##_fetch_add(_z_atomic_##name##_t *var, type val, _z_memory_order_t order) { \
        (void)order;                                                                                                \
        type old = *var;                                                                                            \
        *var += val;                                                                                                \
        return old;                                                                                                 \
    }                                                                                                               \
    static inline type _z_atomic_##name##_fetch_sub(_z_atomic_##name##_t *var, type val, _z_memory_order_t order) { \
        (void)order;                                                                                                \
        type old = *var;                                                                                            \
        *var -= val;                                                                                                \
        return old;                                                                                                 \
    }                                                                                                               \
    static inline bool _z_atomic_##name##_compare_exchange_strong(_z_atomic_##name##_t *var, type *expected,        \
                                                                  type desired, _z_memory_order_t success,          \
                                                                  _z_memory_order_t failure) {                      \
        (void)success;                                                                                              \
        (void)failure;                                                                                              \
        if (*var == *expected) {                                                                                    \
            *var = desired;                                                                                         \
            return true;                                                                                            \
        } else {                                                                                                    \
            *expected = *var;                                                                                       \
            return false;                                                                                           \
        }                                                                                                           \
    }                                                                                                               \
    static inline bool _z_atomic_##name##_compare_exchange_weak(_z_atomic_##name##_t *var, type *expected,          \
                                                                type desired, _z_memory_order_t success,            \
                                                                _z_memory_order_t failure) {                        \
        (void)success;                                                                                              \
        (void)failure;                                                                                              \
        return _z_atomic_##name##_compare_exchange_strong(var, expected, desired, success, failure);                \
    }
static inline void _z_atomic_thread_fence(_z_memory_order_t order) {
    (void)order;
    // No-op in single-threaded mode
}
#endif

_ZP_DEFINE_ATOMIC(uint8_t, uint8)
_ZP_DEFINE_ATOMIC(uint16_t, uint16)
_ZP_DEFINE_ATOMIC(uint32_t, uint32)

// _Atomic bool is not supported by gcc, so we implement it using uint8_t
typedef _z_atomic_uint8_t _z_atomic_bool_t;
static inline void _z_atomic_bool_init(_z_atomic_bool_t *var, bool value) { _z_atomic_uint8_init(var, value ? 1 : 0); }
static inline bool _z_atomic_bool_load(_z_atomic_bool_t *var, _z_memory_order_t order) {
    return _z_atomic_uint8_load(var, order) != 0;
}
static inline void _z_atomic_bool_store(_z_atomic_bool_t *var, bool val, _z_memory_order_t order) {
    _z_atomic_uint8_store(var, val ? 1 : 0, order);
}
static inline bool _z_atomic_bool_compare_exchange_strong(_z_atomic_bool_t *var, bool *expected, bool desired,
                                                          _z_memory_order_t success, _z_memory_order_t failure) {
    uint8_t expected_val = *expected ? 1 : 0;
    bool result = _z_atomic_uint8_compare_exchange_strong(var, &expected_val, desired ? 1 : 0, success, failure);
    *expected = expected_val != 0;
    return result;
}
static inline bool _z_atomic_bool_compare_exchange_weak(_z_atomic_bool_t *var, bool *expected, bool desired,
                                                        _z_memory_order_t success, _z_memory_order_t failure) {
    uint8_t expected_val = *expected ? 1 : 0;
    bool result = _z_atomic_uint8_compare_exchange_weak(var, &expected_val, desired ? 1 : 0, success, failure);
    *expected = expected_val != 0;
    return result;
}

#undef _ZP_DEFINE_ATOMIC
#endif
