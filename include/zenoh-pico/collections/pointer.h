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
#define _Z_POINTER_DEFINE(name, type)                                        \
    typedef struct                                                           \
    {                                                                        \
        atomic_uint _cnt;                                                    \
        type *ptr;                                                           \
    } name##_sptr_t;                                                         \
    static inline name##_sptr_t name##_sptr_new(type ptr)                    \
    {                                                                        \
        name##_sptr_t p;                                                     \
        p.ptr = ptr;                                                         \
        atomic_store_explicit(&p._cnt, 1, memory_order_relaxed);             \
        return p;                                                            \
    }                                                                        \
    static inline name##_sptr_t name##_sptr_clone(name##_sptr_t p)           \
    {                                                                        \
        atomic_fetch_add(&p._cnt, 1, memory_order_relaxed);                  \
        return p;                                                            \
    }                                                                        \
    static inline _Bool name##_sptr_drop(name##_sptr_t p)                    \
    {                                                                        \
        unsigned int c = atomic_fetch_sub(&p._cnt, 1, memory_order_release); \
        _Bool dropped = c == 1;                                              \
        if (dropped)                                                         \
        {                                                                    \
            atomic_thread_fence(memory_order_acquire);                       \
            name##_elem_free(p.ptr);                                         \
        }                                                                    \
        return dropped;                                                      \
    }
#else
/*------------------ Internal Array Macros ------------------*/
#define _Z_POINTER_DEFINE(name, type)                              \
    typedef struct                                                 \
    {                                                              \
        volatile unsigned int _cnt;                                \
        type *ptr;                                                 \
    } name##_sptr_t;                                               \
    static inline name##_sptr_t name##_sptr_new(type ref)          \
    {                                                              \
        name##_sptr_t p;                                           \
        p.ptr = ref;                                               \
        p._cnt = 1;                                                \
        return p;                                                  \
    }                                                              \
    static inline name##_sptr_t name##_sptr_clone(name##_sptr_t p) \
    {                                                              \
        p._cnt += 1;                                               \
        return p;                                                  \
    }                                                              \
    static inline _Bool name##_sptr_drop(name##_sptr_t p)          \
    {                                                              \
        p._cnt -= 1;                                               \
        _Bool dropped = p._cnt == 0;                               \
        if (dropped)                                               \
            name##_elem_free(p.ptr);                               \
        return dropped;                                            \
    }
#endif

#endif /* ZENOH_PICO_COLLECTIONS_POINTER_H */
