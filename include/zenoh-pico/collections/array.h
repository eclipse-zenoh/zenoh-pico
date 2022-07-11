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

#ifndef ZENOH_PICO_COLLECTIONS_ARRAY_H
#define ZENOH_PICO_COLLECTIONS_ARRAY_H

/*------------------ Internal Array Macros ------------------*/
#define _Z_ARRAY_DEFINE(name, type)                                                \
    typedef struct                                                                 \
    {                                                                              \
        size_t len;                                                                \
        type *val;                                                                 \
    } name##_array_t;                                                              \
    static inline name##_array_t name##_array_make(size_t capacity)                \
    {                                                                              \
        name##_array_t a;                                                          \
        a.len = capacity;                                                          \
        a.val = (type *)z_malloc(capacity * sizeof(type));                         \
        return a;                                                                  \
    }                                                                              \
    static inline void name##_array_move(name##_array_t *dst, name##_array_t *src) \
    {                                                                              \
        dst->len = src->len;                                                       \
        dst->val = src->val;                                                       \
        src->len = 0;                                                              \
        src->val = NULL;                                                           \
    }                                                                              \
    static inline int name##_array_is_empty(const name##_array_t *a)               \
    {                                                                              \
        return a->len == 0;                                                        \
    }                                                                              \
    static inline void name##_array_clear(name##_array_t *a)                       \
    {                                                                              \
        for (size_t i = 0; i < a->len; i++)                                        \
            name##_elem_clear(&a->val[i]);                                         \
        z_free(a->val);                                                            \
        a->len = 0;                                                                \
        a->val = NULL;                                                             \
    }                                                                              \
    static inline void name##_array_free(name##_array_t **a)                       \
    {                                                                              \
        name##_array_t *ptr = *a;                                                  \
        name##_array_clear(ptr);                                                   \
        *a = NULL;                                                                 \
    }

#endif /* ZENOH_PICO_COLLECTIONS_ARRAY_H */
