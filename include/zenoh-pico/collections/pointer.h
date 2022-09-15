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

#include <stdint.h>

#if ZENOH_C_STANDARD != 99
#include <stdatomic.h>

/*------------------ Internal Array Macros ------------------*/
#define _Z_POINTER_DEFINE(name, type)                                                         \
    typedef struct {                                                                          \
        type##_t *ptr;                                                                        \
        atomic_uint *_cnt;                                                                    \
    } name##_sptr_t;                                                                          \
    static inline name##_sptr_t name##_sptr_new(type##_t val) {                               \
        name##_sptr_t p;                                                                      \
        p.ptr = (type##_t *)z_malloc(sizeof(type##_t));                                       \
        *p.ptr = val;                                                                         \
        p._cnt = (atomic_uint *)z_malloc(sizeof(atomic_uint));                                \
        atomic_store_explicit(p._cnt, 1, memory_order_relaxed);                               \
        return p;                                                                             \
    }                                                                                         \
    static inline name##_sptr_t name##_sptr_clone(name##_sptr_t *p) {                         \
        name##_sptr_t c;                                                                      \
        c._cnt = p->_cnt;                                                                     \
        c.ptr = p->ptr;                                                                       \
        atomic_fetch_add_explicit(p->_cnt, 1, memory_order_relaxed);                          \
        return c;                                                                             \
    }                                                                                         \
    static inline name##_sptr_t *name##_sptr_clone_as_ptr(name##_sptr_t *p) {                 \
        name##_sptr_t *c = (name##_sptr_t *)z_malloc(sizeof(name##_sptr_t));                  \
        c->_cnt = p->_cnt;                                                                    \
        c->ptr = p->ptr;                                                                      \
        atomic_fetch_add_explicit(p->_cnt, 1, memory_order_relaxed);                          \
        return c;                                                                             \
    }                                                                                         \
    static inline int name##_sptr_eq(const name##_sptr_t *left, const name##_sptr_t *right) { \
        return (left->ptr == right->ptr);                                                     \
    }                                                                                         \
    static inline _Bool name##_sptr_drop(name##_sptr_t *p) {                                  \
        unsigned int c = atomic_fetch_sub_explicit(p->_cnt, 1, memory_order_release);         \
        _Bool dropped = c == 1;                                                               \
        if (dropped) {                                                                        \
            atomic_thread_fence(memory_order_acquire);                                        \
            type##_clear(p->ptr);                                                             \
            z_free(p->ptr);                                                                   \
            z_free(p->_cnt);                                                                  \
        }                                                                                     \
        return dropped;                                                                       \
    }
#else
/*------------------ Internal Array Macros ------------------*/
#define _Z_POINTER_DEFINE(name, type)                                                         \
    typedef struct {                                                                          \
        type##_t *ptr;                                                                        \
        volatile uint8_t *_cnt;                                                               \
    } name##_sptr_t;                                                                          \
    static inline name##_sptr_t name##_sptr_new(type##_t val) {                               \
        name##_sptr_t p;                                                                      \
        p.ptr = (type##_t *)z_malloc(sizeof(type##_t));                                       \
        *p.ptr = val;                                                                         \
        p._cnt = (uint8_t *)z_malloc(sizeof(uint8_t));                                        \
        *p._cnt = 1;                                                                          \
        return p;                                                                             \
    }                                                                                         \
    static inline name##_sptr_t name##_sptr_clone(name##_sptr_t *p) {                         \
        name##_sptr_t c;                                                                      \
        c._cnt = p->_cnt;                                                                     \
        c.ptr = p->ptr;                                                                       \
        *p->_cnt += 1;                                                                        \
        return c;                                                                             \
    }                                                                                         \
    static inline name##_sptr_t *name##_sptr_clone_as_ptr(name##_sptr_t *p) {                 \
        name##_sptr_t *c = (name##_sptr_t *)z_malloc(sizeof(name##_sptr_t));                  \
        c->_cnt = p->_cnt;                                                                    \
        c->ptr = p->ptr;                                                                      \
        *p->_cnt += 1;                                                                        \
        return c;                                                                             \
    }                                                                                         \
    static inline int name##_sptr_eq(const name##_sptr_t *left, const name##_sptr_t *right) { \
        return (left->ptr == right->ptr);                                                     \
    }                                                                                         \
    static inline _Bool name##_sptr_drop(name##_sptr_t *p) {                                  \
        *p->_cnt -= 1;                                                                        \
        _Bool dropped = *p->_cnt == 0;                                                        \
        if (dropped) {                                                                        \
            type##_clear(p->ptr);                                                             \
            z_free(p->ptr);                                                                   \
            z_free((void *)p->_cnt);                                                          \
        }                                                                                     \
        return dropped;                                                                       \
    }
#endif

#endif /* ZENOH_PICO_COLLECTIONS_POINTER_H */
