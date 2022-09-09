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

#include "zenoh-pico/collections/element.h"

#if ZENOH_C_STANDARD != 99
#include <stdatomic.h>

typedef struct {
    void *_ptr;
    atomic_uint *_cnt;
} _z_elem_sptr_t;

_z_elem_sptr_t _z_elem_sptr_null();
_z_elem_sptr_t _z_elem_sptr_new(void *val);
void *_z_elem_sptr_get(_z_elem_sptr_t *p);
_z_elem_sptr_t _z_elem_sptr_clone(const _z_elem_sptr_t *p);
int _z_elem_sptr_eq(const _z_elem_sptr_t *left, const _z_elem_sptr_t *right);
_Bool _z_elem_sptr_drop(_z_elem_sptr_t *p, z_element_free_f f_f);

/*------------------ Internal Array Macros ------------------*/
#define _Z_POINTER_DEFINE(name, type)                                                                               \
    typedef _z_elem_sptr_t name##_sptr_t;                                                                           \
    static inline name##_sptr_t name##_sptr_new(type *val) { return (name##_sptr_t)_z_elem_sptr_new((void *)val); } \
    static inline type *name##_sptr_get(name##_sptr_t *val) {                                                       \
        return (type *)_z_elem_sptr_get((_z_elem_sptr_t *)val);                                                     \
    }                                                                                                               \
    static inline name##_sptr_t name##_sptr_clone(const name##_sptr_t *p) {                                         \
        return (name##_sptr_t)_z_elem_sptr_clone(p);                                                                \
    }                                                                                                               \
    static inline name##_sptr_t *name##_sptr_clone_as_ptr(const name##_sptr_t *p) {                                 \
        /* TODO */                                                                                                  \
        return (name##_sptr_t *)p;                                                                                  \
    }                                                                                                               \
    static inline int name##_sptr_eq(const name##_sptr_t *left, const name##_sptr_t *right) {                       \
        return _z_elem_sptr_eq(left, right);                                                                        \
    }                                                                                                               \
    static inline _Bool name##_sptr_drop(name##_sptr_t *p) {                                                        \
        return _z_elem_sptr_drop((_z_elem_sptr_t *)p, name##_elem_free);                                            \
    }
#else
/*------------------ Internal Array Macros ------------------*/
#define _Z_POINTER_DEFINE(name, type)                                                         \
    typedef struct {                                                                          \
        type *ptr;                                                                            \
        volatile uint8_t *_cnt;                                                               \
    } name##_sptr_t;                                                                          \
    static inline name##_sptr_t name##_sptr_new(type val) {                                   \
        name##_sptr_t p;                                                                      \
        p.ptr = (type *)z_malloc(sizeof(type));                                               \
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
            name##_clear(p->ptr);                                                             \
            z_free(p->ptr);                                                                   \
            z_free((void *)p->_cnt);                                                          \
        }                                                                                     \
        return dropped;                                                                       \
    }
#endif

#endif /* ZENOH_PICO_COLLECTIONS_POINTER_H */
