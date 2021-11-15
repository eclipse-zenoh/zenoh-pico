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

#ifndef ZENOH_PICO_UTILS_COLLECTION_H
#define ZENOH_PICO_UTILS_COLLECTION_H

#include <stdint.h>
#include <stdio.h>

/*-------- Bytes --------*/
/**
 * An array of bytes.
 *
 * Members:
 *   size_t len: The length of the bytes array.
 *   const uint8_t *val: A pointer to the bytes array.
 */
typedef struct z_bytes_t
{
    const uint8_t *val;
    size_t len;
} z_bytes_t;

z_bytes_t _z_bytes_make(size_t capacity);
void _z_bytes_init(z_bytes_t *bs, size_t capacity);
void _z_bytes_copy(z_bytes_t *dst, const z_bytes_t *src);
void _z_bytes_move(z_bytes_t *dst, z_bytes_t *src);
void _z_bytes_free(z_bytes_t *bs);
void _z_bytes_reset(z_bytes_t *bs);

/*-------- String --------*/
/**
 * A string with null terminator.
 */
typedef char *z_str_t;

/**
 * A string with no terminator.
 *
 * Members:
 *   size_t len: The length of the string.
 *   const z_str_t val: A pointer to the string.
 */
typedef struct
{
    z_str_t val;
    size_t len;
} z_string_t;

z_string_t z_string_make(const z_str_t value);
void z_string_free(z_string_t *s);
void _z_string_copy(z_string_t *dst, const z_string_t *src);
void _z_string_move(z_string_t *dst, z_string_t *src);
void _z_string_move_str(z_string_t *dst, z_str_t src);
void _z_string_free(z_string_t *str);
void _z_string_reset(z_string_t *str);
z_string_t _z_string_from_bytes(z_bytes_t *bs);

/*-------- StrArray --------*/
/**
 * An array of NULL terminated strings.
 *
 * Members:
 *   size_t len: The length of the array.
 *   z_str_t *val: A pointer to the array.
 */
typedef struct
{
    z_str_t *val;
    size_t len;
} z_str_array_t;

z_str_array_t _z_str_array_make(size_t len);
void _z_str_array_init(z_str_array_t *sa, size_t len);
void _z_str_array_copy(z_str_array_t *dst, const z_str_array_t *src);
void _z_str_array_move(z_str_array_t *dst, z_str_array_t *src);
void _z_str_array_free(z_str_array_t *sa);

/*-------- Dynamically allocated vector --------*/
/**
 * A dynamically allocate vector.
 */
typedef struct
{
    size_t _capacity;
    size_t _len;
    void **_val;
} z_vec_t;

z_vec_t z_vec_make(size_t capacity);
z_vec_t z_vec_clone(const z_vec_t *v);

size_t z_vec_len(const z_vec_t *v);
void z_vec_append(z_vec_t *v, void *e);
const void *z_vec_get(const z_vec_t *v, size_t i);
void z_vec_set(z_vec_t *sv, size_t i, void *e);

void z_vec_clear(z_vec_t *v);
void z_vec_free(z_vec_t *v);

/*-------- Linked List --------*/
/**
 * A single-linked list.
 *
 *  Members:
 *   void *val: The pointer to the inner value.
 *   struct z_list *tail: A pointer to the next element in the list.
 */
typedef int (*z_list_predicate)(void *, void *);
typedef struct _z_list
{
    void *val;
    struct _z_list *tail;
} z_list_t;

extern z_list_t *z_list_empty;

z_list_t *z_list_of(void *x);
z_list_t *z_list_cons(z_list_t *xs, void *x);

void *z_list_head(z_list_t *xs);
z_list_t *z_list_tail(z_list_t *xs);

size_t z_list_len(z_list_t *xs);
z_list_t *z_list_remove(z_list_t *xs, z_list_predicate p, void *arg);

z_list_t *z_list_pop(z_list_t *xs);
z_list_t *z_list_drop_val(z_list_t *xs, size_t position);
void z_list_free(z_list_t *xs);
void z_list_free_deep(z_list_t *xs);

/*-------- Int Map --------*/
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
} z_i_map_entry_t;

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
} z_i_map_t;

z_i_map_t z_i_map_make(size_t capacity);
int z_i_map_start(z_i_map_t *map, size_t capacity);

size_t z_i_map_capacity(const z_i_map_t *map);
size_t z_i_map_len(const z_i_map_t *map);
int z_i_map_is_empty(const z_i_map_t *map);

int z_i_map_set(z_i_map_t *map, size_t k, void *v);
void *z_i_map_get(const z_i_map_t *map, size_t k);
void z_i_map_remove(z_i_map_t *map, size_t k);
void z_i_map_clear(z_i_map_t *map);
void z_i_map_free(z_i_map_t **map);

#endif /* ZENOH_PICO_UTILS_COLLECTION_H */
