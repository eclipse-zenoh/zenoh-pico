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
#include "zenoh-pico/collections/pointer.h"

/*-------- Single-linked List --------*/

/**
 * A single-linked list holding pointers.
 *
 *  Members:
 *   void *_val: The pointer to the inner value.
 *   struct _z_list_t *_tail: A pointer to the next element in the list.
 */
typedef struct _z_l_t {
    void *_val;
    struct _z_l_t *_tail;
} _z_list_t;

_z_list_t *_z_list_of(void *x);

size_t _z_list_len(const _z_list_t *xs);
uint8_t _z_list_is_empty(const _z_list_t *xs);

void *_z_list_head(const _z_list_t *xs);
_z_list_t *_z_list_tail(const _z_list_t *xs);

_z_list_t *_z_list_push(_z_list_t *xs, void *x);
_z_list_t *_z_list_pop(_z_list_t *xs, z_element_free_f f_f);

_z_list_t *_z_list_find(const _z_list_t *xs, z_element_eq_f f_f, void *e);

_z_list_t *_z_list_drop_filter(_z_list_t *xs, z_element_free_f f_f, z_element_eq_f c_f, void *left);

_z_list_t *_z_list_clone(const _z_list_t *xs, z_element_clone_f d_f);
void _z_list_free(_z_list_t **xs, z_element_free_f f_f);

#define _Z_LIST_DEFINE(name, type)                                                                                   \
    typedef _z_list_t name##_list_t;                                                                                 \
    static inline name##_list_t *name##_list_new(void) { return NULL; }                                              \
    static inline size_t name##_list_len(const name##_list_t *l) { return _z_list_len(l); }                          \
    static inline uint8_t name##_list_is_empty(const name##_list_t *l) { return _z_list_is_empty(l); }               \
    static inline type *name##_list_head(const name##_list_t *l) { return (type *)_z_list_head(l); }                 \
    static inline name##_list_t *name##_list_tail(const name##_list_t *l) { return _z_list_tail(l); }                \
    static inline name##_list_t *name##_list_push(name##_list_t *l, type *e) { return _z_list_push(l, e); }          \
    static inline name##_list_t *name##_list_pop(name##_list_t *l) { return _z_list_pop(l, name##_elem_free); }      \
    static inline name##_list_t *name##_list_find(const name##_list_t *l, name##_eq_f c_f, type *e) {                \
        return _z_list_find(l, (z_element_eq_f)c_f, e);                                                              \
    }                                                                                                                \
    static inline name##_list_t *name##_list_drop_filter(name##_list_t *l, name##_eq_f c_f, type *e) {               \
        return _z_list_drop_filter(l, name##_elem_free, (z_element_eq_f)c_f, e);                                     \
    }                                                                                                                \
    static inline name##_list_t *name##_list_clone(name##_list_t *l) { return _z_list_clone(l, name##_elem_clone); } \
    static inline void name##_list_free(name##_list_t **l) { _z_list_free(l, name##_elem_free); }

/**
 * A single-linked list holding smart pointers.
 *
 *  Members:
 *   _z_elem_sptr_t _val: The smart pointer to the inner value.
 *   struct _z_sptr_list_t *_tail: A pointer to the next element in the list.
 */
typedef struct _z_sl_t {
    _z_elem_sptr_t _val;
    struct _z_sl_t *_tail;
} _z_sptr_list_t;

_z_sptr_list_t *_z_list_sptr_of(_z_elem_sptr_t x);

size_t _z_list_sptr_len(const _z_sptr_list_t *xs);
uint8_t _z_list_sptr_is_empty(const _z_sptr_list_t *xs);

_z_elem_sptr_t _z_list_sptr_head(const _z_sptr_list_t *xs);
_z_sptr_list_t *_z_sptr_list_tail(const _z_sptr_list_t *xs);

_z_sptr_list_t *_z_list_sptr_push(_z_sptr_list_t *xs, _z_elem_sptr_t x);
_z_sptr_list_t *_z_list_sptr_pop(_z_sptr_list_t *xs, z_element_free_f f_f);

_z_sptr_list_t *_z_list_sptr_find(const _z_sptr_list_t *xs, _z_elem_sptr_t *e);

_z_sptr_list_t *_z_list_sptr_drop_filter(_z_sptr_list_t *xs, z_element_free_f f_f, _z_elem_sptr_t *left);

_z_sptr_list_t *_z_list_sptr_clone(const _z_sptr_list_t *xs);
void _z_list_sptr_free(_z_sptr_list_t **xs, z_element_free_f f_f);

#define _Z_LIST_SPTR_DEFINE(name)                                                                                     \
    typedef _z_sptr_list_t name##_sptr_list_t;                                                                        \
    static inline name##_sptr_list_t *name##_sptr_list_new(void) { return NULL; }                                     \
    static inline size_t name##_sptr_list_len(const name##_sptr_list_t *l) { return _z_list_sptr_len(l); }            \
    static inline uint8_t name##_sptr_list_is_empty(const name##_sptr_list_t *l) { return _z_list_sptr_is_empty(l); } \
    static inline name##_sptr_t name##_sptr_list_head(const name##_sptr_list_t *l) {                                  \
        return (name##_sptr_t)_z_list_sptr_head(l);                                                                   \
    }                                                                                                                 \
    static inline name##_sptr_list_t *name##_sptr_list_tail(const name##_sptr_list_t *l) {                            \
        return _z_sptr_list_tail(l);                                                                                  \
    }                                                                                                                 \
    static inline name##_sptr_list_t *name##_sptr_list_push(name##_sptr_list_t *l, name##_sptr_t e) {                 \
        return _z_list_sptr_push(l, e);                                                                               \
    }                                                                                                                 \
    static inline name##_sptr_list_t *name##_sptr_list_pop(name##_sptr_list_t *l) {                                   \
        return _z_list_sptr_pop(l, name##_elem_free);                                                                 \
    }                                                                                                                 \
    static inline name##_sptr_list_t *name##_sptr_list_find(const name##_sptr_list_t *l, name##_sptr_t *e) {          \
        return _z_list_sptr_find(l, e);                                                                               \
    }                                                                                                                 \
    static inline name##_sptr_list_t *name##_sptr_list_drop_filter(name##_sptr_list_t *l, name##_sptr_t *e) {         \
        return _z_list_sptr_drop_filter(l, name##_elem_free, e);                                                      \
    }                                                                                                                 \
    static inline name##_sptr_list_t *name##_sptr_list_clone(name##_sptr_list_t *l) { return _z_list_sptr_clone(l); } \
    static inline void name##_sptr_list_free(name##_sptr_list_t **l) { _z_list_sptr_free(l, name##_elem_free); }

#endif /* ZENOH_PICO_COLLECTIONS_LIST_H */
