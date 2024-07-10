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
//

#ifndef ZENOH_PICO_COLLECTIONS_REFCOUNT_H
#define ZENOH_PICO_COLLECTIONS_REFCOUNT_H

#include <stdbool.h>
#include <stdint.h>

#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

#define _Z_RC_MAX_COUNT INT32_MAX  // Based on Rust lazy overflow check

#if Z_FEATURE_MULTI_THREAD == 1
#if ZENOH_C_STANDARD != 99

#ifndef __cplusplus
#include <stdatomic.h>
#define _z_atomic(X) _Atomic X
#define _z_atomic_store_explicit atomic_store_explicit
#define _z_atomic_fetch_add_explicit atomic_fetch_add_explicit
#define _z_atomic_fetch_sub_explicit atomic_fetch_sub_explicit
#define _z_atomic_load_explicit atomic_load_explicit
#define _z_atomic_compare_exchange_strong atomic_compare_exchange_strong
#define _z_atomic_compare_exchange_weak_explicit atomic_compare_exchange_weak_explicit
#define _z_memory_order_acquire memory_order_acquire
#define _z_memory_order_release memory_order_release
#define _z_memory_order_relaxed memory_order_relaxed
#else
#include <atomic>
#define _z_atomic(X) std::atomic<X>
#define _z_atomic_store_explicit std::atomic_store_explicit
#define _z_atomic_fetch_add_explicit std::atomic_fetch_add_explicit
#define _z_atomic_fetch_sub_explicit std::atomic_fetch_sub_explicit
#define _z_atomic_load_explicit std::atomic_load_explicit
#define _z_atomic_compare_exchange_strong std::atomic_compare_exchange_strong
#define _z_atomic_compare_exchange_weak_explicit std::atomic_compare_exchange_weak_explicit
#define _z_memory_order_acquire std::memory_order_acquire
#define _z_memory_order_release std::memory_order_release
#define _z_memory_order_relaxed std::memory_order_relaxed
#endif  // __cplusplus

// c11 atomic variant
#define _ZP_RC_CNT_TYPE _z_atomic(unsigned int)
#define _ZP_RC_OP_INIT_CNT                                                                  \
    _z_atomic_store_explicit(&p.in->_strong_cnt, (unsigned int)1, _z_memory_order_relaxed); \
    _z_atomic_store_explicit(&p.in->_weak_cnt, (unsigned int)1, _z_memory_order_relaxed);
#define _ZP_RC_OP_INCR_STRONG_CNT \
    _z_atomic_fetch_add_explicit(&p->in->_strong_cnt, (unsigned int)1, _z_memory_order_relaxed);
#define _ZP_RC_OP_INCR_AND_CMP_WEAK(x) \
    _z_atomic_fetch_add_explicit(&p->in->_weak_cnt, (unsigned int)1, _z_memory_order_relaxed) >= x
#define _ZP_RC_OP_DECR_AND_CMP_STRONG(x) \
    _z_atomic_fetch_sub_explicit(&p->in->_strong_cnt, (unsigned int)1, _z_memory_order_release) > (unsigned int)x
#define _ZP_RC_OP_DECR_AND_CMP_WEAK(x) \
    _z_atomic_fetch_sub_explicit(&p->in->_weak_cnt, (unsigned int)1, _z_memory_order_release) > (unsigned int)x
#define _ZP_RC_OP_CHECK_STRONG_CNT(x) _z_atomic_compare_exchange_strong(&p->in->_strong_cnt, &x, x)
#define _ZP_RC_OP_SYNC atomic_thread_fence(_z_memory_order_acquire);
#define _ZP_RC_OP_UPGRADE_CAS_LOOP                                                                                  \
    unsigned int prev = _z_atomic_load_explicit(&p->in->_strong_cnt, _z_memory_order_relaxed);                      \
    while ((prev != 0) && (prev < _Z_RC_MAX_COUNT)) {                                                               \
        if (_z_atomic_compare_exchange_weak_explicit(&p->in->_strong_cnt, &prev, prev + 1, _z_memory_order_acquire, \
                                                     _z_memory_order_relaxed)) {                                    \
            if (_ZP_RC_OP_INCR_AND_CMP_WEAK(_Z_RC_MAX_COUNT)) {                                                     \
                _Z_ERROR("Rc weak count overflow");                                                                 \
                c.in = NULL;                                                                                        \
                return c;                                                                                           \
            }                                                                                                       \
            c.in = p->in;                                                                                           \
            return c;                                                                                               \
        }                                                                                                           \
    }

#else  // ZENOH_C_STANDARD == 99
#ifdef ZENOH_COMPILER_GCC

// c99 gcc sync builtin variant
#define _ZP_RC_CNT_TYPE unsigned int
#define _ZP_RC_OP_INIT_CNT                                     \
    __sync_fetch_and_and(&p.in->_strong_cnt, (unsigned int)0); \
    __sync_fetch_and_add(&p.in->_strong_cnt, (unsigned int)1); \
    __sync_fetch_and_and(&p.in->_weak_cnt, (unsigned int)0);   \
    __sync_fetch_and_add(&p.in->_weak_cnt, (unsigned int)1);
#define _ZP_RC_OP_INCR_STRONG_CNT __sync_fetch_and_add(&p->in->_strong_cnt, (unsigned int)1);
#define _ZP_RC_OP_INCR_AND_CMP_WEAK(x) __sync_fetch_and_add(&p->in->_weak_cnt, (unsigned int)1) >= x
#define _ZP_RC_OP_DECR_AND_CMP_STRONG(x) __sync_fetch_and_sub(&p->in->_strong_cnt, (unsigned int)1) > (unsigned int)x
#define _ZP_RC_OP_DECR_AND_CMP_WEAK(x) __sync_fetch_and_sub(&p->in->_weak_cnt, (unsigned int)1) > (unsigned int)x
#define _ZP_RC_OP_CHECK_STRONG_CNT(x) __sync_bool_compare_and_swap(&p->in->_strong_cnt, x, x)
#define _ZP_RC_OP_SYNC __sync_synchronize();
#define _ZP_RC_OP_UPGRADE_CAS_LOOP                                                  \
    unsigned int prev = __sync_fetch_and_add(&p->in->_strong_cnt, (unsigned int)0); \
    while ((prev != 0) && (prev < _Z_RC_MAX_COUNT)) {                               \
        if (__sync_bool_compare_and_swap(&p->in->_strong_cnt, prev, prev + 1)) {    \
            if (_ZP_RC_OP_INCR_AND_CMP_WEAK(_Z_RC_MAX_COUNT)) {                     \
                _Z_ERROR("Rc weak count overflow");                                 \
                c.in = NULL;                                                        \
                return c;                                                           \
            }                                                                       \
            c.in = p->in;                                                           \
            return c;                                                               \
        } else {                                                                    \
            prev = __sync_fetch_and_add(&p->in->_strong_cnt, (unsigned int)0);      \
        }                                                                           \
    }

#else  // !ZENOH_COMPILER_GCC

// None variant
#error "Multi-thread refcount in C99 only exists for GCC, use GCC or C11 or deactivate multi-thread"
#define _ZP_RC_CNT_TYPE unsigned int
#define _ZP_RC_OP_INIT_CNT
#define _ZP_RC_OP_INCR_STRONG_CNT
#define _ZP_RC_OP_INCR_AND_CMP_WEAK(x) (x == 0)
#define _ZP_RC_OP_DECR_AND_CMP_STRONG(x) (x == 0)
#define _ZP_RC_OP_DECR_AND_CMP_WEAK(x) (x == 0)
#define _ZP_RC_OP_CHECK_STRONG_CNT(x) (x == 0) && (p != NULL)
#define _ZP_RC_OP_SYNC
#define _ZP_RC_OP_UPGRADE_CAS_LOOP (void)p;

#endif  // ZENOH_COMPILER_GCC
#endif  // ZENOH_C_STANDARD != 99
#else   // Z_FEATURE_MULTI_THREAD == 0

// Single thread variant
#define _ZP_RC_CNT_TYPE unsigned int
#define _ZP_RC_OP_INIT_CNT               \
    p.in->_strong_cnt = (unsigned int)1; \
    p.in->_weak_cnt = (unsigned int)1;
#define _ZP_RC_OP_INCR_STRONG_CNT p->in->_strong_cnt++;
#define _ZP_RC_OP_INCR_AND_CMP_WEAK(x) p->in->_weak_cnt++ >= x
#define _ZP_RC_OP_DECR_AND_CMP_STRONG(x) p->in->_strong_cnt-- > (unsigned int)x
#define _ZP_RC_OP_DECR_AND_CMP_WEAK(x) p->in->_weak_cnt-- > (unsigned int)x
#define _ZP_RC_OP_CHECK_STRONG_CNT(x) (p->in->_strong_cnt == x)
#define _ZP_RC_OP_SYNC
#define _ZP_RC_OP_UPGRADE_CAS_LOOP                                             \
    if ((p->in->_strong_cnt != 0) && (p->in->_strong_cnt < _Z_RC_MAX_COUNT)) { \
        if (_ZP_RC_OP_INCR_AND_CMP_WEAK(_Z_RC_MAX_COUNT)) {                    \
            _Z_ERROR("Rc weak count overflow");                                \
            c.in = NULL;                                                       \
            return c;                                                          \
        }                                                                      \
        _ZP_RC_OP_INCR_STRONG_CNT                                              \
        c.in = p->in;                                                          \
        return c;                                                              \
    }

#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Internal Array Macros ------------------*/
#define _Z_REFCOUNT_DEFINE(name, type)                                                          \
    typedef struct name##_inner_rc_t {                                                          \
        type##_t val;                                                                           \
        _ZP_RC_CNT_TYPE _strong_cnt;                                                            \
        _ZP_RC_CNT_TYPE _weak_cnt;                                                              \
    } name##_inner_rc_t;                                                                        \
                                                                                                \
    typedef struct name##_rc_t {                                                                \
        name##_inner_rc_t *in;                                                                  \
    } name##_rc_t;                                                                              \
                                                                                                \
    typedef struct name##_weak_t {                                                              \
        name##_inner_rc_t *in;                                                                  \
    } name##_weak_t;                                                                            \
                                                                                                \
    static inline name##_rc_t name##_rc_null(void) {                                            \
        name##_rc_t p;                                                                          \
        p.in = NULL;                                                                            \
        return p;                                                                               \
    }                                                                                           \
    static inline name##_rc_t name##_rc_new(void) {                                             \
        name##_rc_t p;                                                                          \
        p.in = (name##_inner_rc_t *)z_malloc(sizeof(name##_inner_rc_t));                        \
        if (p.in != NULL) {                                                                     \
            memset(&p.in->val, 0, sizeof(type##_t));                                            \
            _ZP_RC_OP_INIT_CNT                                                                  \
        }                                                                                       \
        return p;                                                                               \
    }                                                                                           \
    static inline name##_rc_t name##_rc_new_from_val(type##_t val) {                            \
        name##_rc_t p;                                                                          \
        p.in = (name##_inner_rc_t *)z_malloc(sizeof(name##_inner_rc_t));                        \
        if (p.in != NULL) {                                                                     \
            p.in->val = val;                                                                    \
            _ZP_RC_OP_INIT_CNT                                                                  \
        }                                                                                       \
        return p;                                                                               \
    }                                                                                           \
    static inline name##_rc_t name##_rc_clone(const name##_rc_t *p) {                           \
        name##_rc_t c;                                                                          \
        if (_ZP_RC_OP_INCR_AND_CMP_WEAK(_Z_RC_MAX_COUNT)) {                                     \
            _Z_ERROR("Rc weak count overflow");                                                 \
            c.in = NULL;                                                                        \
            return c;                                                                           \
        }                                                                                       \
        _ZP_RC_OP_INCR_STRONG_CNT                                                               \
        c.in = p->in;                                                                           \
        return c;                                                                               \
    }                                                                                           \
    static inline name##_rc_t *name##_rc_clone_as_ptr(const name##_rc_t *p) {                   \
        name##_rc_t *c = (name##_rc_t *)z_malloc(sizeof(name##_rc_t));                          \
        if (c != NULL) {                                                                        \
            if (_ZP_RC_OP_INCR_AND_CMP_WEAK(_Z_RC_MAX_COUNT)) {                                 \
                _Z_ERROR("Rc weak count overflow");                                             \
                z_free(c);                                                                      \
                return NULL;                                                                    \
            }                                                                                   \
            _ZP_RC_OP_INCR_STRONG_CNT                                                           \
            c->in = p->in;                                                                      \
        }                                                                                       \
        return c;                                                                               \
    }                                                                                           \
    static inline name##_weak_t name##_rc_clone_as_weak(const name##_rc_t *p) {                 \
        name##_weak_t c;                                                                        \
        if (_ZP_RC_OP_INCR_AND_CMP_WEAK(_Z_RC_MAX_COUNT)) {                                     \
            _Z_ERROR("Rc weak count overflow");                                                 \
            c.in = NULL;                                                                        \
            return c;                                                                           \
        }                                                                                       \
        c.in = p->in;                                                                           \
        return c;                                                                               \
    }                                                                                           \
    static inline name##_weak_t *name##_rc_clone_as_weak_ptr(const name##_rc_t *p) {            \
        name##_weak_t *c = (name##_weak_t *)z_malloc(sizeof(name##_weak_t));                    \
        if (c != NULL) {                                                                        \
            if (_ZP_RC_OP_INCR_AND_CMP_WEAK(_Z_RC_MAX_COUNT)) {                                 \
                _Z_ERROR("Rc weak count overflow");                                             \
                z_free(c);                                                                      \
                return NULL;                                                                    \
            }                                                                                   \
            c->in = p->in;                                                                      \
        }                                                                                       \
        return c;                                                                               \
    }                                                                                           \
    static inline void name##_rc_copy(name##_rc_t *dst, const name##_rc_t *p) {                 \
        if (_ZP_RC_OP_INCR_AND_CMP_WEAK(_Z_RC_MAX_COUNT)) {                                     \
            _Z_ERROR("Rc weak count overflow");                                                 \
            dst->in = NULL;                                                                     \
            return;                                                                             \
        }                                                                                       \
        _ZP_RC_OP_INCR_STRONG_CNT                                                               \
        dst->in = p->in;                                                                        \
    }                                                                                           \
    static inline _Bool name##_rc_eq(const name##_rc_t *left, const name##_rc_t *right) {       \
        return (left->in == right->in);                                                         \
    }                                                                                           \
    static inline _Bool name##_rc_drop(name##_rc_t *p) {                                        \
        if ((p == NULL) || (p->in == NULL)) {                                                   \
            return false;                                                                       \
        }                                                                                       \
        if (_ZP_RC_OP_DECR_AND_CMP_STRONG(1)) {                                                 \
            if (_ZP_RC_OP_DECR_AND_CMP_WEAK(1)) {                                               \
                return false;                                                                   \
            }                                                                                   \
            _ZP_RC_OP_SYNC                                                                      \
            z_free(p->in);                                                                      \
            return true;                                                                        \
        }                                                                                       \
        _ZP_RC_OP_SYNC                                                                          \
        type##_clear(&p->in->val);                                                              \
                                                                                                \
        if (_ZP_RC_OP_DECR_AND_CMP_WEAK(1)) {                                                   \
            return false;                                                                       \
        }                                                                                       \
        _ZP_RC_OP_SYNC                                                                          \
        z_free(p->in);                                                                          \
        return true;                                                                            \
    }                                                                                           \
    static inline name##_weak_t name##_weak_null(void) {                                        \
        name##_weak_t p;                                                                        \
        p.in = NULL;                                                                            \
        return p;                                                                               \
    }                                                                                           \
    static inline name##_weak_t name##_weak_clone(const name##_weak_t *p) {                     \
        name##_weak_t c;                                                                        \
        if (_ZP_RC_OP_INCR_AND_CMP_WEAK(_Z_RC_MAX_COUNT)) {                                     \
            _Z_ERROR("Rc weak count overflow");                                                 \
            c.in = NULL;                                                                        \
            return c;                                                                           \
        }                                                                                       \
        c.in = p->in;                                                                           \
        return c;                                                                               \
    }                                                                                           \
    static inline void name##_weak_copy(name##_weak_t *dst, const name##_weak_t *p) {           \
        if (_ZP_RC_OP_INCR_AND_CMP_WEAK(_Z_RC_MAX_COUNT)) {                                     \
            _Z_ERROR("Rc weak count overflow");                                                 \
            dst->in = NULL;                                                                     \
            return;                                                                             \
        }                                                                                       \
        dst->in = p->in;                                                                        \
    }                                                                                           \
    static inline name##_rc_t name##_weak_upgrade(name##_weak_t *p) {                           \
        name##_rc_t c;                                                                          \
        _ZP_RC_OP_UPGRADE_CAS_LOOP                                                              \
        c.in = NULL;                                                                            \
        return c;                                                                               \
    }                                                                                           \
    static inline _Bool name##_weak_eq(const name##_weak_t *left, const name##_weak_t *right) { \
        return (left->in == right->in);                                                         \
    }                                                                                           \
    static inline _Bool name##_weak_drop(name##_weak_t *p) {                                    \
        if ((p == NULL) || (p->in == NULL)) {                                                   \
            return false;                                                                       \
        }                                                                                       \
        if (_ZP_RC_OP_DECR_AND_CMP_WEAK(1)) {                                                   \
            return false;                                                                       \
        }                                                                                       \
        _ZP_RC_OP_SYNC                                                                          \
        z_free(p->in);                                                                          \
        return true;                                                                            \
    }                                                                                           \
    static inline size_t name##_rc_size(name##_rc_t *p) {                                       \
        _ZP_UNUSED(p);                                                                          \
        return sizeof(name##_rc_t);                                                             \
    }

#endif /* ZENOH_PICO_COLLECTIONS_REFCOUNT_H */
