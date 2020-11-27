/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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

#ifndef _ZENOH_PICO_COLLECTION_H
#define _ZENOH_PICO_COLLECTION_H

#include <stdint.h>
#include <stdio.h>
#include "zenoh-pico/private/types.h"
#include "zenoh-pico/types.h"

/*------------------ Internal Array Macros ------------------*/
#define _ARRAY_DECLARE(type, name, prefix) \
    typedef struct                         \
    {                                      \
        size_t len;                        \
        type *val;                         \
    } prefix##name##_array_t;

#define _ARRAY_P_DECLARE(type, name, prefix) \
    typedef struct                           \
    {                                        \
        size_t len;                          \
        type **val;                          \
    } prefix##name##_p_array_t;

#define _ARRAY_S_DEFINE(type, name, prefix, arr, len) \
    prefix##name##_array_t arr = {len, (type *)malloc(len * sizeof(type))};

#define _ARRAY_P_S_DEFINE(type, name, prefix, arr, len) \
    prefix##name##_array_t arr = {len, (type **)malloc(len * sizeof(type *))};

#define _ARRAY_H_DEFINE(T, arr, len)                               \
    z_array_##T *arr = (z_array_##T *)malloc(sizeof(z_array_##T)); \
    arr->len = len;                                                \
    arr->val = (T *)malloc(len * sizeof(T));

#define _ARRAY_S_INIT(T, arr, len) \
    arr.len = len;                 \
    arr.val = (T *)malloc(len * sizeof(T));

#define _ARRAY_P_S_INIT(T, arr, len) \
    arr.len = len;                   \
    arr.val = (T **)malloc(len * sizeof(T *));

#define _ARRAY_H_INIT(T, arr, len) \
    arr->len = len;                \
    arr->val = (T *)malloc(len * sizeof(T))

#define _ARRAY_S_COPY(T, dst, src)              \
    dst.len = src.len;                          \
    dst.val = (T *)malloc(dst.len * sizeof(T)); \
    memcpy(dst.val, src.val, dst.len);

#define _ARRAY_H_COPY(T, dst, src)                \
    dst->len = src->len;                          \
    dst->val = (T *)malloc(dst->len * sizeof(T)); \
    memcpy(dst->val, src->val, dst->len);

#define _ARRAY_S_FREE(arr) \
    free(arr.val);         \
    arr.val = 0;           \
    arr.len = 0;

#define _ARRAY_H_FREE(arr) \
    free(arr->val);        \
    arr->val = 0;          \
    arr->len = 0

/*-------- Dynamically allocated vector --------*/
_z_vec_t _z_vec_make(size_t capacity);
_z_vec_t _z_vec_clone(const _z_vec_t *v);

size_t _z_vec_len(const _z_vec_t *v);
void _z_vec_append(_z_vec_t *v, void *e);
const void *_z_vec_get(const _z_vec_t *v, size_t i);
void _z_vec_set(_z_vec_t *sv, size_t i, void *e);

void _z_vec_free_inner(_z_vec_t *v);
void _z_vec_free(_z_vec_t *v);

/*-------- Linked List --------*/
extern _z_list_t *_z_list_empty;

_z_list_t *_z_list_of(void *x);
_z_list_t *_z_list_cons(_z_list_t *xs, void *x);

void *_z_list_head(_z_list_t *xs);
_z_list_t *_z_list_tail(_z_list_t *xs);

size_t _z_list_len(_z_list_t *xs);
_z_list_t *_z_list_remove(_z_list_t *xs, _z_list_predicate p, void *arg);

_z_list_t *_z_list_pop(_z_list_t *xs);
_z_list_t *_z_list_drop_val(_z_list_t *xs, size_t position);
void _z_list_free(_z_list_t *xs);
void _z_list_free_deep(_z_list_t *xs);

/*-------- Int Map --------*/
#define _Z_DEFAULT_I_MAP_CAPACITY 64

extern _z_i_map_t *_z_i_map_empty;
_z_i_map_t *_z_i_map_make(size_t capacity);

size_t _z_i_map_capacity(_z_i_map_t *map);
size_t _z_i_map_len(_z_i_map_t *map);

void _z_i_map_set(_z_i_map_t *map, size_t k, void *v);
void *_z_i_map_get(_z_i_map_t *map, size_t k);
void _z_i_map_remove(_z_i_map_t *map, size_t k);

void _z_i_map_free(_z_i_map_t *map);

/*-------- Mvar --------*/
_z_mvar_t *_z_mvar_empty(void);
int _z_mvar_is_empty(_z_mvar_t *mv);

_z_mvar_t *_z_mvar_of(void *e);
void *_z_mvar_get(_z_mvar_t *mv);
void _z_mvar_put(_z_mvar_t *mv, void *e);

/*-------- Operations on Bytes --------*/
z_bytes_t _z_bytes_make(size_t capacity);
void _z_bytes_init(z_bytes_t *bs, size_t capacity);
void _z_bytes_copy(z_bytes_t *dst, const z_bytes_t *src);
void _z_bytes_move(z_bytes_t *dst, z_bytes_t *src);
void _z_bytes_free(z_bytes_t *bs);
void _z_bytes_reset(z_bytes_t *bs);

/*-------- Operations on String --------*/
void _z_string_copy(z_string_t *dst, const z_string_t *src);
void _z_string_move(z_string_t *dst, z_string_t *src);
void _z_string_free(z_string_t *str);
void _z_string_reset(z_string_t *str);
z_string_t _z_string_from_bytes(z_bytes_t *bs);

/*-------- Operations on StrArray --------*/
z_str_array_t _z_str_array_make(size_t len);
void _z_str_array_init(z_str_array_t *sa, size_t len);
void _z_str_array_copy(z_str_array_t *dst, const z_str_array_t *src);
void _z_str_array_move(z_str_array_t *dst, z_str_array_t *src);
void _z_str_array_free(z_str_array_t *sa);

#endif /* _ZENOH_PICO_COLLECTION_H */
