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

/*-------- int-void map --------*/
#define _Z_DEFAULT_INT_MAP_CAPACITY 16

/**
 * An entry of an hashmap with integer keys.
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

void _z_int_void_map_init(_z_int_void_map_t *map, size_t capacity);
_z_int_void_map_t _z_int_void_map_make(size_t capacity);

void *_z_int_void_map_insert(_z_int_void_map_t *map, size_t k, void *v, z_element_free_f f);
void *_z_int_void_map_get(const _z_int_void_map_t *map, size_t k);
void _z_int_void_map_remove(_z_int_void_map_t *map, size_t k, z_element_free_f f);

size_t _z_int_void_map_capacity(const _z_int_void_map_t *map);
size_t _z_int_void_map_len(const _z_int_void_map_t *map);
_Bool _z_int_void_map_is_empty(const _z_int_void_map_t *map);

void _z_int_void_map_clear(_z_int_void_map_t *map, z_element_free_f f);
void _z_int_void_map_free(_z_int_void_map_t **map, z_element_free_f f);

#define _Z_INT_MAP_DEFINE(name, type)                                                                            \
    typedef _z_int_void_map_entry_t name##_intmap_entry_t;                                                       \
    static inline void name##_intmap_entry_elem_free(void **e) {                                                 \
        name##_intmap_entry_t *ptr = (name##_intmap_entry_t *)*e;                                                \
        if (ptr != NULL) {                                                                                       \
            name##_elem_free(&ptr->_val);                                                                        \
            z_free(ptr);                                                                                         \
            *e = NULL;                                                                                           \
        }                                                                                                        \
    }                                                                                                            \
    typedef _z_int_void_map_t name##_intmap_t;                                                                   \
    static inline void name##_intmap_init(name##_intmap_t *m) {                                                  \
        _z_int_void_map_init(m, _Z_DEFAULT_INT_MAP_CAPACITY);                                                    \
    }                                                                                                            \
    static inline name##_intmap_t name##_intmap_make(void) {                                                     \
        return _z_int_void_map_make(_Z_DEFAULT_INT_MAP_CAPACITY);                                                \
    }                                                                                                            \
    static inline type *name##_intmap_insert(name##_intmap_t *m, size_t k, type *v) {                            \
        return (type *)_z_int_void_map_insert(m, k, v, name##_intmap_entry_elem_free);                           \
    }                                                                                                            \
    static inline type *name##_intmap_get(const name##_intmap_t *m, size_t k) {                                  \
        return (type *)_z_int_void_map_get(m, k);                                                                \
    }                                                                                                            \
    static inline void name##_intmap_remove(name##_intmap_t *m, size_t k) {                                      \
        _z_int_void_map_remove(m, k, name##_intmap_entry_elem_free);                                             \
    }                                                                                                            \
    static inline size_t name##_intmap_capacity(name##_intmap_t *m) { return _z_int_void_map_capacity(m); }      \
    static inline size_t name##_intmap_len(name##_intmap_t *m) { return _z_int_void_map_len(m); }                \
    static inline _Bool name##_intmap_is_empty(const name##_intmap_t *m) { return _z_int_void_map_is_empty(m); } \
    static inline void name##_intmap_clear(name##_intmap_t *m) {                                                 \
        _z_int_void_map_clear(m, name##_intmap_entry_elem_free);                                                 \
    }                                                                                                            \
    static inline void name##_intmap_free(name##_intmap_t **m) {                                                 \
        _z_int_void_map_free(m, name##_intmap_entry_elem_free);                                                  \
    }

#endif /* ZENOH_PICO_COLLECTIONS_INTMAP_H */
