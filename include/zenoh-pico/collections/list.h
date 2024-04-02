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

#ifndef ZENOH_PICO_COLLECTIONS_LIST_H
#define ZENOH_PICO_COLLECTIONS_LIST_H

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/collections/element.h"

/*-------- Single-linked List --------*/
/**
 * A single-linked list.
 *
 *  Members:
 *   void *lal: The pointer to the inner value.
 *   struct z_list *tail: A pointer to the next element in the list.
 */
typedef struct _z_l_t {
    void *_val;
    struct _z_l_t *_tail;
} _z_list_t;

_z_list_t *_z_list_of(void *x);

size_t _z_list_len(const _z_list_t *xs);
_Bool _z_list_is_empty(const _z_list_t *xs);

void *_z_list_head(const _z_list_t *xs);
_z_list_t *_z_list_tail(const _z_list_t *xs);

_z_list_t *_z_list_push(_z_list_t *xs, void *x);
_z_list_t *_z_list_push_back(_z_list_t *xs, void *x);
_z_list_t *_z_list_pop(_z_list_t *xs, z_element_free_f f_f, void **x);

_z_list_t *_z_list_find(const _z_list_t *xs, z_element_eq_f f_f, void *e);

_z_list_t *_z_list_drop_filter(_z_list_t *xs, z_element_free_f f_f, z_element_eq_f c_f, void *left);

_z_list_t *_z_list_clone(const _z_list_t *xs, z_element_clone_f d_f);
void _z_list_free(_z_list_t **xs, z_element_free_f f_f);

#define _Z_LIST_DEFINE(name, type)                                                                                   \
    typedef _z_list_t name##_list_t;                                                                                 \
    static inline name##_list_t *name##_list_new(void) { return NULL; }                                              \
    static inline size_t name##_list_len(const name##_list_t *l) { return _z_list_len(l); }                          \
    static inline _Bool name##_list_is_empty(const name##_list_t *l) { return _z_list_is_empty(l); }                 \
    static inline type *name##_list_head(const name##_list_t *l) { return (type *)_z_list_head(l); }                 \
    static inline name##_list_t *name##_list_tail(const name##_list_t *l) { return _z_list_tail(l); }                \
    static inline name##_list_t *name##_list_push(name##_list_t *l, type *e) { return _z_list_push(l, e); }          \
    static inline name##_list_t *name##_list_pop(name##_list_t *l, type **x) {                                       \
        return _z_list_pop(l, name##_elem_free, (void **)x);                                                         \
    }                                                                                                                \
    static inline name##_list_t *name##_list_find(const name##_list_t *l, name##_eq_f c_f, type *e) {                \
        return _z_list_find(l, (z_element_eq_f)c_f, e);                                                              \
    }                                                                                                                \
    static inline name##_list_t *name##_list_drop_filter(name##_list_t *l, name##_eq_f c_f, type *e) {               \
        return _z_list_drop_filter(l, name##_elem_free, (z_element_eq_f)c_f, e);                                     \
    }                                                                                                                \
    static inline name##_list_t *name##_list_clone(name##_list_t *l) { return _z_list_clone(l, name##_elem_clone); } \
    static inline void name##_list_free(name##_list_t **l) { _z_list_free(l, name##_elem_free); }

#endif /* ZENOH_PICO_COLLECTIONS_LIST_H */
