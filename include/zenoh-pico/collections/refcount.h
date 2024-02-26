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

#if Z_FEATURE_MULTI_THREAD == 1
#if ZENOH_C_STANDARD != 99

#ifndef __cplusplus
#include <stdatomic.h>
#define _z_atomic(X) _Atomic X
#define _z_atomic_store_explicit atomic_store_explicit
#define _z_atomic_fetch_add_explicit atomic_fetch_add_explicit
#define _z_atomic_fetch_sub_explicit atomic_fetch_sub_explicit
#define _z_memory_order_acquire memory_order_acquire
#define _z_memory_order_release memory_order_release
#define _z_memory_order_relaxed memory_order_relaxed
#else
#include <atomic>
#define _z_atomic(X) std::atomic<X>
#define _z_atomic_store_explicit std::atomic_store_explicit
#define _z_atomic_fetch_add_explicit std::atomic_fetch_add_explicit
#define _z_atomic_fetch_sub_explicit std::atomic_fetch_sub_explicit
#define _z_memory_order_acquire std::memory_order_acquire
#define _z_memory_order_release std::memory_order_release
#define _z_memory_order_relaxed std::memory_order_relaxed
#endif  // __cplusplus

// c11 atomic variant
#define _ZP_RC_CNT_TYPE _z_atomic(unsigned int)
#define _ZP_RC_OP_INIT_CNT _z_atomic_store_explicit(&p.in->_cnt, (unsigned int)1, _z_memory_order_relaxed);
#define _ZP_RC_OP_INCR_CNT _z_atomic_fetch_add_explicit(&p->in->_cnt, (unsigned int)1, _z_memory_order_relaxed);
#define _ZP_RC_OP_DECR_AND_CMP _z_atomic_fetch_sub_explicit(&p->in->_cnt, (unsigned int)1, _z_memory_order_release) > 1
#define _ZP_RC_OP_SYNC atomic_thread_fence(_z_memory_order_acquire);

#else  // ZENOH_C_STANDARD == 99
#ifdef ZENOH_COMPILER_GCC

// c99 gcc sync builtin variant
#define _ZP_RC_CNT_TYPE unsigned int
#define _ZP_RC_OP_INIT_CNT                \
    __sync_fetch_and_and(&p.in->_cnt, 0); \
    __sync_fetch_and_add(&p.in->_cnt, 1);
#define _ZP_RC_OP_INCR_CNT __sync_fetch_and_add(&p->in->_cnt, 1);
#define _ZP_RC_OP_DECR_AND_CMP __sync_fetch_and_sub(&p->in->_cnt, 1) > 1
#define _ZP_RC_OP_SYNC __sync_synchronize();

#else  // !ZENOH_COMPILER_GCC

// None variant
#error "Multi-thread refcount in C99 only exists for GCC, use GCC or C11 or deactivate multi-thread"
#define _ZP_RC_CNT_TYPE unsigned int
#define _ZP_RC_OP_INIT_CNT
#define _ZP_RC_OP_INCR_CNT
#define _ZP_RC_OP_DECR_AND_CMP
#define _ZP_RC_OP_SYNC

#endif  // ZENOH_COMPILER_GCC
#endif  // ZENOH_C_STANDARD != 99
#else   // Z_FEATURE_MULTI_THREAD == 0

// Single thread variant
#define _ZP_RC_CNT_TYPE unsigned int
#define _ZP_RC_OP_INIT_CNT p.in->_cnt = 1;
#define _ZP_RC_OP_INCR_CNT p->in->_cnt += 1;
#define _ZP_RC_OP_DECR_AND_CMP p->in->_cnt-- > 1
#define _ZP_RC_OP_SYNC

#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Internal Array Macros ------------------*/
#define _Z_REFCOUNT_DEFINE(name, type)                                                    \
    typedef struct name##_inner_rc_t {                                                    \
        type##_t val;                                                                     \
        _ZP_RC_CNT_TYPE _cnt;                                                             \
    } name##_inner_rc_t;                                                                  \
    typedef struct name##_rc_t {                                                          \
        name##_inner_rc_t *in;                                                            \
    } name##_rc_t;                                                                        \
    static inline name##_rc_t name##_rc_new(void) {                                       \
        name##_rc_t p;                                                                    \
        p.in = (name##_inner_rc_t *)z_malloc(sizeof(name##_inner_rc_t));                  \
        if (p.in != NULL) {                                                               \
            memset(&p.in->val, 0, sizeof(type##_t));                                      \
            _ZP_RC_OP_INIT_CNT                                                            \
        }                                                                                 \
        return p;                                                                         \
    }                                                                                     \
    static inline name##_rc_t name##_rc_new_from_val(type##_t val) {                      \
        name##_rc_t p;                                                                    \
        p.in = (name##_inner_rc_t *)z_malloc(sizeof(name##_inner_rc_t));                  \
        if (p.in != NULL) {                                                               \
            p.in->val = val;                                                              \
            _ZP_RC_OP_INIT_CNT                                                            \
        }                                                                                 \
        return p;                                                                         \
    }                                                                                     \
    static inline name##_rc_t name##_rc_clone(name##_rc_t *p) {                           \
        name##_rc_t c;                                                                    \
        c.in = p->in;                                                                     \
        _ZP_RC_OP_INCR_CNT                                                                \
        return c;                                                                         \
    }                                                                                     \
    static inline name##_rc_t *name##_rc_clone_as_ptr(name##_rc_t *p) {                   \
        name##_rc_t *c = (name##_rc_t *)z_malloc(sizeof(name##_rc_t));                    \
        if (c != NULL) {                                                                  \
            c->in = p->in;                                                                \
            _ZP_RC_OP_INCR_CNT                                                            \
        }                                                                                 \
        return c;                                                                         \
    }                                                                                     \
    static inline _Bool name##_rc_eq(const name##_rc_t *left, const name##_rc_t *right) { \
        return (left->in == right->in);                                                   \
    }                                                                                     \
    static inline _Bool name##_rc_drop(name##_rc_t *p) {                                  \
        if ((p == NULL) || (p->in == NULL)) {                                             \
            return false;                                                                 \
        }                                                                                 \
        if (_ZP_RC_OP_DECR_AND_CMP) {                                                     \
            return false;                                                                 \
        }                                                                                 \
        _ZP_RC_OP_SYNC                                                                    \
        type##_clear(&p->in->val);                                                        \
        z_free(p->in);                                                                    \
        return true;                                                                      \
    }

#endif /* ZENOH_PICO_COLLECTIONS_REFCOUNT_H */
