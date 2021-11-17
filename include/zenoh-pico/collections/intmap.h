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

#ifndef ZENOH_PICO_UTILS_COLLECTION_INTMAP_H
#define ZENOH_PICO_UTILS_COLLECTION_INTMAP_H

#include <stdint.h>
#include <stdio.h>
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/utils/result.h"

/*-------- int-void map --------*/
#define _Z_DEFAULT_I_MAP_CAPACITY 64

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
    void *value;
} _z_int_void_map_entry_t;

typedef void (*_zn_map_entry_free_f)(void **me);
typedef void *(*_zn_map_entry_dup_f)(const void *me);
typedef int (*_zn_map_entry_cmp_f)(const void *left, const void *right);

typedef struct
{
    _zn_map_entry_free_f free;
    _zn_map_entry_dup_f dup;
    _zn_map_entry_cmp_f cmp;
} _z_int_void_map_entry_f;

/**
 * An hashmap with integer keys.
 *
 * Members:
 *   z_list_t **vals: the linked list containing the values
 *   size_t capacity: the capacity of the hashmap
 *   size_t len: the actual length of the hashmap
 */
typedef struct
{
    z_list_t **vals;
    size_t capacity;
    size_t len;

    _z_int_void_map_entry_f f;
} _z_int_void_map_t;

_z_int_void_map_t _z_int_void_map_make(size_t capacity, _z_int_void_map_entry_f f);
void _z_int_void_map_init(_z_int_void_map_t *map, size_t capacity, _z_int_void_map_entry_f f);

void *_z_int_void_map_insert(_z_int_void_map_t *map, size_t k, void *v);
const void *_z_int_void_map_get(const _z_int_void_map_t *map, size_t k);
void *_z_int_void_map_remove(_z_int_void_map_t *map, size_t k);

size_t _z_int_void_map_capacity(const _z_int_void_map_t *map);
size_t _z_int_void_map_len(const _z_int_void_map_t *map);
int _z_int_void_map_is_empty(const _z_int_void_map_t *map);

void _z_int_void_map_clear(_z_int_void_map_t *map);
void _z_int_void_map_free(_z_int_void_map_t **map);

/*------------------ int-str map ----------------*/
#define INT_STR_MAP_KEYVALUE_SEPARATOR '='
#define INT_STR_MAP_LIST_SEPARATOR ';'

typedef _z_int_void_map_t zn_int_str_map_t;

typedef struct
{
    unsigned int key;
    z_str_t str;
} zn_int_str_mapping_t;

ZN_RESULT_DECLARE(zn_int_str_map_t, int_str_map)

zn_int_str_map_t zn_int_str_map_make(void);
void zn_int_str_map_init(zn_int_str_map_t *ism);

z_str_t zn_int_str_map_insert(zn_int_str_map_t *ps, unsigned int key, z_str_t value);
const z_str_t zn_int_str_map_get(const zn_int_str_map_t *ps, unsigned int key);
z_str_t zn_int_str_map_remove(zn_int_str_map_t *ps, unsigned int key);

#define zn_int_str_map_len _z_int_void_map_len
#define zn_int_str_map_is_empty _z_int_void_map_is_empty
#define zn_int_str_map_clear _z_int_void_map_clear
#define zn_int_str_map_free _z_int_void_map_free

size_t zn_int_str_map_strlen(const zn_int_str_map_t *s, unsigned int argc, zn_int_str_mapping_t argv[]);

void zn_int_str_map_onto_str(z_str_t dst, const zn_int_str_map_t *s, unsigned int argc, zn_int_str_mapping_t argv[]);
z_str_t zn_int_str_map_to_str(const zn_int_str_map_t *s, unsigned int argc, zn_int_str_mapping_t argv[]);

zn_int_str_map_result_t zn_int_str_map_from_str(const z_str_t s, unsigned int argc, zn_int_str_mapping_t argv[]);
zn_int_str_map_result_t zn_int_str_map_from_strn(const z_str_t s, unsigned int argc, zn_int_str_mapping_t argv[], size_t n);

/*------------------ int-list map ----------------*/
typedef _z_int_void_map_t zn_int_list_map_t;

zn_int_list_map_t zn_int_list_map_make(void);
void zn_int_list_map_init(zn_int_list_map_t *ism);

z_list_t *zn_int_list_map_insert(zn_int_list_map_t *ps, unsigned int key, z_list_t *value);
const z_list_t *zn_int_list_map_get(const zn_int_list_map_t *ps, unsigned int key);
z_list_t *zn_int_list_map_remove(zn_int_list_map_t *ps, unsigned int key);

#define zn_int_list_map_len _z_int_void_map_len
#define zn_int_list_map_is_empty _z_int_void_map_is_empty
#define zn_int_list_map_clear _z_int_void_map_clear
#define zn_int_list_map_free _z_int_void_map_free

#endif /* ZENOH_PICO_UTILS_COLLECTION_INTMAP_H */
