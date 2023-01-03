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

#ifndef ZENOH_PICO_COLLECTIONS_VECTOR_H
#define ZENOH_PICO_COLLECTIONS_VECTOR_H

#include <stdint.h>

#include "zenoh-pico/collections/element.h"

/*-------- Dynamically allocated vector --------*/
/**
 * A dynamically allocate vector.
 */
typedef struct {
    size_t _capacity;
    size_t _len;
    void **_val;
} _z_vec_t;

_z_vec_t _z_vec_make(size_t capacity);
void _z_vec_copy(_z_vec_t *dst, const _z_vec_t *src, z_element_clone_f f);

size_t _z_vec_len(const _z_vec_t *v);
_Bool _z_vec_is_empty(const _z_vec_t *v);

void _z_vec_append(_z_vec_t *v, void *e);
void *_z_vec_get(const _z_vec_t *v, size_t pos);
void _z_vec_set(_z_vec_t *sv, size_t pos, void *e, z_element_free_f f);
void _z_vec_remove(_z_vec_t *sv, size_t pos, z_element_free_f f);

void _z_vec_reset(_z_vec_t *v, z_element_free_f f);
void _z_vec_clear(_z_vec_t *v, z_element_free_f f);
void _z_vec_free(_z_vec_t **v, z_element_free_f f);

#define _Z_VEC_DEFINE(name, type)                                                                                  \
    typedef _z_vec_t name##_vec_t;                                                                                 \
    static inline name##_vec_t name##_vec_make(size_t capacity) { return _z_vec_make(capacity); }                  \
    static inline size_t name##_vec_len(const name##_vec_t *v) { return _z_vec_len(v); }                           \
    static inline _Bool name##_vec_is_empty(const name##_vec_t *v) { return _z_vec_is_empty(v); }                  \
    static inline void name##_vec_append(name##_vec_t *v, type *e) { _z_vec_append(v, e); }                        \
    static inline type *name##_vec_get(const name##_vec_t *v, size_t pos) { return (type *)_z_vec_get(v, pos); }   \
    static inline void name##_vec_set(name##_vec_t *v, size_t pos, type *e) {                                      \
        _z_vec_set(v, pos, e, name##_elem_free);                                                                   \
    }                                                                                                              \
    static inline void name##_vec_remove(name##_vec_t *v, size_t pos) { _z_vec_remove(v, pos, name##_elem_free); } \
    static inline void name##_vec_copy(name##_vec_t *dst, const name##_vec_t *src) {                               \
        _z_vec_copy(dst, src, name##_elem_clone);                                                                  \
    }                                                                                                              \
    static inline void name##_vec_reset(name##_vec_t *v) { _z_vec_reset(v, name##_elem_free); }                    \
    static inline void name##_vec_clear(name##_vec_t *v) { _z_vec_clear(v, name##_elem_free); }                    \
    static inline void name##_vec_free(name##_vec_t **v) { _z_vec_free(v, name##_elem_free); }

#endif /* ZENOH_PICO_COLLECTIONS_VECTOR_H */
