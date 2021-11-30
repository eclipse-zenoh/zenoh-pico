/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#ifndef ZENOH_PICO_COLLECTIONS_INTMAP_H
#define ZENOH_PICO_COLLECTIONS_INTMAP_H

#include <stdint.h>
#include <stdio.h>
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/string.h"
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
typedef struct
{
    size_t key;
    void *val;
} _z_int_void_map_entry_t;

/**
 * An hashmap with integer keys.
 *
 * Members:
 *   z_intmap_t **vals: the linked intmap containing the values
 *   size_t capacity: the capacity of the hashmap
 *   size_t len: the actual length of the hashmap
 */
typedef struct
{
    size_t capacity;
    _z_list_t **vals;
} _z_int_void_map_t;

void _z_int_void_map_init(_z_int_void_map_t *map, size_t capacity);
_z_int_void_map_t _z_int_void_map_make(size_t capacity);

void *_z_int_void_map_insert(_z_int_void_map_t *map, size_t k, void *v, z_element_free_f f);
void *_z_int_void_map_get(const _z_int_void_map_t *map, size_t k);
void _z_int_void_map_remove(_z_int_void_map_t *map, size_t k, z_element_free_f f);

size_t _z_int_void_map_capacity(const _z_int_void_map_t *map);
size_t _z_int_void_map_len(const _z_int_void_map_t *map);
int _z_int_void_map_is_empty(const _z_int_void_map_t *map);

void _z_int_void_map_clear(_z_int_void_map_t *map, z_element_free_f f);
void _z_int_void_map_free(_z_int_void_map_t **map, z_element_free_f f);

#define _Z_INT_MAP_DEFINE(name, type)                                                  \
    typedef _z_int_void_map_entry_t name##_intmap_entry_t;                             \
    static inline void name##_intmap_entry_elem_free(void **e)                         \
    {                                                                                  \
        name##_intmap_entry_t *ptr = (name##_intmap_entry_t *)*e;                      \
        name##_elem_free((void **)&ptr->val);                                          \
        free(ptr);                                                                     \
        *e = NULL;                                                                     \
    }                                                                                  \
    typedef _z_int_void_map_t name##_intmap_t;                                         \
    static inline void name##_intmap_init(name##_intmap_t *m)                          \
    {                                                                                  \
        _z_int_void_map_init(m, _Z_DEFAULT_INT_MAP_CAPACITY);                          \
    }                                                                                  \
    static inline name##_intmap_t name##_intmap_make()                                 \
    {                                                                                  \
        return _z_int_void_map_make(_Z_DEFAULT_INT_MAP_CAPACITY);                      \
    }                                                                                  \
    static inline type *name##_intmap_insert(name##_intmap_t *m, size_t k, type *v)    \
    {                                                                                  \
        return (type *)_z_int_void_map_insert(m, k, v, name##_intmap_entry_elem_free); \
    }                                                                                  \
    static inline type *name##_intmap_get(const name##_intmap_t *m, size_t k)          \
    {                                                                                  \
        return (type *)_z_int_void_map_get(m, k);                                      \
    }                                                                                  \
    static inline void name##_intmap_remove(name##_intmap_t *m, size_t k)              \
    {                                                                                  \
        _z_int_void_map_remove(m, k, name##_intmap_entry_elem_free);                   \
    }                                                                                  \
    static inline int name##_intmap_capacity(name##_intmap_t *m)                       \
    {                                                                                  \
        return _z_int_void_map_capacity(m);                                            \
    }                                                                                  \
    static inline size_t name##_intmap_len(name##_intmap_t *m)                         \
    {                                                                                  \
        return _z_int_void_map_len(m);                                                 \
    }                                                                                  \
    static inline int name##_intmap_is_empty(const name##_intmap_t *m)                 \
    {                                                                                  \
        return _z_int_void_map_is_empty(m);                                            \
    }                                                                                  \
    static inline void name##_intmap_clear(name##_intmap_t *m)                         \
    {                                                                                  \
        _z_int_void_map_clear(m, name##_intmap_entry_elem_free);                       \
    }                                                                                  \
    static inline void name##_intmap_free(name##_intmap_t **m)                         \
    {                                                                                  \
        _z_int_void_map_free(m, name##_intmap_entry_elem_free);                        \
    }

/*------------------ int-str map ----------------*/
_Z_INT_MAP_DEFINE(_z_str, char)

#define INT_STR_MAP_KEYVALUE_SEPARATOR '='
#define INT_STR_MAP_LIST_SEPARATOR ';'

typedef struct
{
    unsigned int key;
    z_str_t str;
} _z_str_intmapping_t;
_Z_RESULT_DECLARE(_z_str_intmap_t, str_intmap)

size_t _z_str_intmap_strlen(const _z_str_intmap_t *s, unsigned int argc, _z_str_intmapping_t argv[]);

void _z_str_intmap_onto_str(z_str_t dst, const _z_str_intmap_t *s, unsigned int argc, _z_str_intmapping_t argv[]);
z_str_t _z_str_intmap_to_str(const _z_str_intmap_t *s, unsigned int argc, _z_str_intmapping_t argv[]);

_z_str_intmap_result_t _z_str_intmap_from_str(const z_str_t s, unsigned int argc, _z_str_intmapping_t argv[]);
_z_str_intmap_result_t _z_str_intmap_from_strn(const z_str_t s, unsigned int argc, _z_str_intmapping_t argv[], size_t n);

#endif /* ZENOH_PICO_COLLECTIONS_INTMAP_H */
