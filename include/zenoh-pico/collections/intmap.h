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

#ifndef ZENOH_PICO_COLLECTIONS_INTMAP_H
#define ZENOH_PICO_COLLECTIONS_INTMAP_H

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/utils/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-------- int-void map --------*/
#define _Z_DEFAULT_INT_MAP_CAPACITY 16

/**
 * An entry of a hashmap with integer keys.
 *
 * Members:
 *   size_t key: the hashed key of the value
 *   void *value: the value
 */
typedef struct {
    size_t _key;
    void *_val;
} _z_int_void_map_entry_t;

/**
 * An hashmap with integer keys.
 *
 * Members:
 *   z_intmap_t **vals: the linked intmap containing the values
 *   size_t capacity: the capacity of the hashmap
 *   size_t len: the actual length of the hashmap
 */
typedef struct {
    size_t _capacity;
    _z_list_t **_vals;
} _z_int_void_map_t;

/**
 * An iterator of an hashmap with integer keys.
 */
typedef struct {
    _z_int_void_map_entry_t *_entry;

    const _z_int_void_map_t *_map;
    size_t _idx;
    _z_list_t *_list_ptr;
} _z_int_void_map_iterator_t;

void _z_int_void_map_init(_z_int_void_map_t *map, size_t capacity);
_z_int_void_map_t _z_int_void_map_make(size_t capacity);

void *_z_int_void_map_insert(_z_int_void_map_t *map, size_t k, void *v, z_element_free_f f, bool replace);
void *_z_int_void_map_get(const _z_int_void_map_t *map, size_t k);
_z_list_t *_z_int_void_map_get_all(const _z_int_void_map_t *map, size_t k);
void _z_int_void_map_remove(_z_int_void_map_t *map, size_t k, z_element_free_f f);

size_t _z_int_void_map_capacity(const _z_int_void_map_t *map);
size_t _z_int_void_map_len(const _z_int_void_map_t *map);
bool _z_int_void_map_is_empty(const _z_int_void_map_t *map);

z_result_t _z_int_void_map_copy(_z_int_void_map_t *dst, const _z_int_void_map_t *src, z_element_clone_f f_c);
_z_int_void_map_t _z_int_void_map_clone(const _z_int_void_map_t *src, z_element_clone_f f_c, z_element_free_f f_f);

void _z_int_void_map_clear(_z_int_void_map_t *map, z_element_free_f f);
void _z_int_void_map_free(_z_int_void_map_t **map, z_element_free_f f);
static inline void _z_int_void_map_move(_z_int_void_map_t *dst, _z_int_void_map_t *src) {
    *dst = *src;
    *src = _z_int_void_map_make(src->_capacity);
}

_z_int_void_map_iterator_t _z_int_void_map_iterator_make(const _z_int_void_map_t *map);
bool _z_int_void_map_iterator_next(_z_int_void_map_iterator_t *iter);
size_t _z_int_void_map_iterator_key(const _z_int_void_map_iterator_t *iter);
void *_z_int_void_map_iterator_value(const _z_int_void_map_iterator_t *iter);

#define _Z_INT_MAP_DEFINE(name, type)                                                                           \
    typedef _z_int_void_map_entry_t name##_intmap_entry_t;                                                      \
    static inline void name##_intmap_entry_elem_free(void **e) {                                                \
        name##_intmap_entry_t *ptr = (name##_intmap_entry_t *)*e;                                               \
        if (ptr != NULL) {                                                                                      \
            name##_elem_free(&ptr->_val);                                                                       \
            z_free(ptr);                                                                                        \
            *e = NULL;                                                                                          \
        }                                                                                                       \
    }                                                                                                           \
    static inline void *name##_intmap_entry_elem_clone(const void *e) {                                         \
        const name##_intmap_entry_t *src = (name##_intmap_entry_t *)e;                                          \
        name##_intmap_entry_t *dst = (name##_intmap_entry_t *)z_malloc(sizeof(name##_intmap_entry_t));          \
        dst->_key = src->_key;                                                                                  \
        dst->_val = name##_elem_clone(src->_val);                                                               \
        return dst;                                                                                             \
    }                                                                                                           \
    typedef _z_int_void_map_t name##_intmap_t;                                                                  \
    typedef _z_int_void_map_iterator_t name##_intmap_iterator_t;                                                \
    static inline void name##_intmap_init(name##_intmap_t *m) {                                                 \
        _z_int_void_map_init(m, _Z_DEFAULT_INT_MAP_CAPACITY);                                                   \
    }                                                                                                           \
    static inline name##_intmap_t name##_intmap_make(void) {                                                    \
        return _z_int_void_map_make(_Z_DEFAULT_INT_MAP_CAPACITY);                                               \
    }                                                                                                           \
    static inline type *name##_intmap_insert(name##_intmap_t *m, size_t k, type *v) {                           \
        return (type *)_z_int_void_map_insert(m, k, v, name##_intmap_entry_elem_free, true);                    \
    }                                                                                                           \
    static inline type *name##_intmap_insert_push(name##_intmap_t *m, size_t k, type *v) {                      \
        return (type *)_z_int_void_map_insert(m, k, v, name##_intmap_entry_elem_free, false);                   \
    }                                                                                                           \
    static inline type *name##_intmap_get(const name##_intmap_t *m, size_t k) {                                 \
        return (type *)_z_int_void_map_get(m, k);                                                               \
    }                                                                                                           \
    static inline _z_list_t *name##_intmap_get_all(const name##_intmap_t *m, size_t k) {                        \
        return _z_int_void_map_get_all(m, k);                                                                   \
    }                                                                                                           \
    static inline name##_intmap_t name##_intmap_clone(const name##_intmap_t *m) {                               \
        return _z_int_void_map_clone(m, name##_intmap_entry_elem_clone, name##_intmap_entry_elem_free);         \
    }                                                                                                           \
    static inline void name##_intmap_remove(name##_intmap_t *m, size_t k) {                                     \
        _z_int_void_map_remove(m, k, name##_intmap_entry_elem_free);                                            \
    }                                                                                                           \
    static inline size_t name##_intmap_capacity(name##_intmap_t *m) { return _z_int_void_map_capacity(m); }     \
    static inline size_t name##_intmap_len(name##_intmap_t *m) { return _z_int_void_map_len(m); }               \
    static inline bool name##_intmap_is_empty(const name##_intmap_t *m) { return _z_int_void_map_is_empty(m); } \
    static inline void name##_intmap_clear(name##_intmap_t *m) {                                                \
        _z_int_void_map_clear(m, name##_intmap_entry_elem_free);                                                \
    }                                                                                                           \
    static inline void name##_intmap_free(name##_intmap_t **m) {                                                \
        _z_int_void_map_free(m, name##_intmap_entry_elem_free);                                                 \
    }                                                                                                           \
    static inline z_result_t name##_intmap_move(name##_intmap_t *dst, name##_intmap_t *src) {                   \
        _z_int_void_map_move(dst, src);                                                                         \
        return _Z_RES_OK;                                                                                       \
    }                                                                                                           \
    static inline name##_intmap_iterator_t name##_intmap_iterator_make(const name##_intmap_t *m) {              \
        return _z_int_void_map_iterator_make(m);                                                                \
    }                                                                                                           \
    static inline bool name##_intmap_iterator_next(name##_intmap_iterator_t *iter) {                            \
        return _z_int_void_map_iterator_next(iter);                                                             \
    }                                                                                                           \
    static inline size_t name##_intmap_iterator_key(const name##_intmap_iterator_t *iter) {                     \
        return _z_int_void_map_iterator_key(iter);                                                              \
    }                                                                                                           \
    static inline type *name##_intmap_iterator_value(const name##_intmap_iterator_t *iter) {                    \
        return (type *)_z_int_void_map_iterator_value(iter);                                                    \
    }

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_COLLECTIONS_INTMAP_H */
