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

#ifndef ZENOH_PICO_COLLECTIONS_POINTER_H
#define ZENOH_PICO_COLLECTIONS_POINTER_H

#include <stdbool.h>
#include <stdint.h>

#if ZENOH_C_STANDARD != 99 && !defined(ZENOH_NO_STDATOMIC)

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

/*------------------ Internal Array Macros ------------------*/
#define _Z_POINTER_DEFINE(name, type)                                                                       \
    typedef struct {                                                                                        \
        type##_t *ptr;                                                                                      \
        _z_atomic(unsigned int) * _cnt;                                                                     \
    } name##_sptr_t;                                                                                        \
    static inline name##_sptr_t name##_sptr_new(type##_t val) {                                             \
        name##_sptr_t p;                                                                                    \
        p.ptr = (type##_t *)z_malloc(sizeof(type##_t));                                                     \
        if (p.ptr != NULL) {                                                                                \
            p._cnt = (_z_atomic(unsigned int) *)z_malloc(sizeof(_z_atomic(unsigned int) *));                \
            if (p._cnt != NULL) {                                                                           \
                *p.ptr = val;                                                                               \
                _z_atomic_store_explicit((volatile unsigned int *)p._cnt, 1, _z_memory_order_relaxed);      \
            } else {                                                                                        \
                z_free(p.ptr);                                                                              \
            }                                                                                               \
        }                                                                                                   \
        return p;                                                                                           \
    }                                                                                                       \
    static inline name##_sptr_t name##_sptr_clone(name##_sptr_t *p) {                                       \
        name##_sptr_t c;                                                                                    \
        c._cnt = p->_cnt;                                                                                   \
        c.ptr = p->ptr;                                                                                     \
        _z_atomic_fetch_add_explicit((volatile unsigned int *)p->_cnt, 1, _z_memory_order_relaxed);         \
        return c;                                                                                           \
    }                                                                                                       \
    static inline name##_sptr_t *name##_sptr_clone_as_ptr(name##_sptr_t *p) {                               \
        name##_sptr_t *c = (name##_sptr_t *)z_malloc(sizeof(name##_sptr_t));                                \
        if (c != NULL) {                                                                                    \
            c->_cnt = p->_cnt;                                                                              \
            c->ptr = p->ptr;                                                                                \
            _z_atomic_fetch_add_explicit((volatile unsigned int *)p->_cnt, 1, _z_memory_order_relaxed);     \
        }                                                                                                   \
        return c;                                                                                           \
    }                                                                                                       \
    static inline _Bool name##_sptr_eq(const name##_sptr_t *left, const name##_sptr_t *right) {             \
        return (left->ptr == right->ptr);                                                                   \
    }                                                                                                       \
    static inline _Bool name##_sptr_drop(name##_sptr_t *p) {                                                \
        _Bool dropped = false;                                                                              \
        if (p->_cnt != NULL) {                                                                              \
            unsigned int c =                                                                                \
                _z_atomic_fetch_sub_explicit((volatile unsigned int *)p->_cnt, 1, _z_memory_order_release); \
            dropped = c == 1;                                                                               \
            if (dropped == true) {                                                                          \
                atomic_thread_fence(_z_memory_order_acquire);                                               \
                if (p->ptr != NULL) {                                                                       \
                    type##_clear(p->ptr);                                                                   \
                    z_free(p->ptr);                                                                         \
                    z_free((void *)p->_cnt);                                                                \
                }                                                                                           \
            }                                                                                               \
        }                                                                                                   \
        return dropped;                                                                                     \
    }
#else
/*------------------ Internal Array Macros ------------------*/
#define _Z_POINTER_DEFINE(name, type)                                                           \
    typedef struct {                                                                            \
        type##_t *ptr;                                                                          \
        volatile uint8_t *_cnt;                                                                 \
    } name##_sptr_t;                                                                            \
    static inline name##_sptr_t name##_sptr_new(type##_t val) {                                 \
        name##_sptr_t p;                                                                        \
        p.ptr = (type##_t *)z_malloc(sizeof(type##_t));                                         \
        if (p.ptr != NULL) {                                                                    \
            p._cnt = (uint8_t *)z_malloc(sizeof(uint8_t));                                      \
            if (p._cnt != NULL) {                                                               \
                *p.ptr = val;                                                                   \
                *p._cnt = 1;                                                                    \
            } else {                                                                            \
                z_free(p.ptr);                                                                  \
            }                                                                                   \
        }                                                                                       \
        return p;                                                                               \
    }                                                                                           \
    static inline name##_sptr_t name##_sptr_clone(name##_sptr_t *p) {                           \
        name##_sptr_t c;                                                                        \
        c._cnt = p->_cnt;                                                                       \
        c.ptr = p->ptr;                                                                         \
        *p->_cnt = *p->_cnt + (uint8_t)1;                                                       \
        return c;                                                                               \
    }                                                                                           \
    static inline name##_sptr_t *name##_sptr_clone_as_ptr(name##_sptr_t *p) {                   \
        name##_sptr_t *c = (name##_sptr_t *)z_malloc(sizeof(name##_sptr_t));                    \
        if (c != NULL) {                                                                        \
            c->_cnt = p->_cnt;                                                                  \
            c->ptr = p->ptr;                                                                    \
            *p->_cnt = *p->_cnt + (uint8_t)1;                                                   \
        }                                                                                       \
        return c;                                                                               \
    }                                                                                           \
    static inline _Bool name##_sptr_eq(const name##_sptr_t *left, const name##_sptr_t *right) { \
        return (left->ptr == right->ptr);                                                       \
    }                                                                                           \
    static inline _Bool name##_sptr_drop(name##_sptr_t *p) {                                    \
        _Bool dropped = true;                                                                   \
        if (p->_cnt != NULL) {                                                                  \
            *p->_cnt = *p->_cnt - 1;                                                            \
            dropped = *p->_cnt == 0;                                                            \
            if (dropped == true) {                                                              \
                if (p->ptr != NULL) {                                                           \
                    type##_clear(p->ptr);                                                       \
                    z_free(p->ptr);                                                             \
                    z_free((void *)p->_cnt);                                                    \
                }                                                                               \
            }                                                                                   \
        }                                                                                       \
        return dropped;                                                                         \
    }
#endif  // ZENOH_C_STANDARD != 99 && !defined(ZENOH_NO_STDATOMIC)

#endif /* ZENOH_PICO_COLLECTIONS_POINTER_H */
