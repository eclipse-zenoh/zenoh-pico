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

#ifndef _ZENOH_C_COLLECTION_H
#define _ZENOH_C_COLLECTION_H

#include <stddef.h>

extern const int _z_dummy_arg;

#define Z_UNUSED_ARG(z) (void)(z)
#define Z_UNUSED_ARG_2(z1, z2) \
    (void)(z1);                \
    (void)(z2)
#define Z_UNUSED_ARG_3(z1, z2, z3) \
    (void)(z1);                    \
    (void)(z2);                    \
    (void)(z3)
#define Z_UNUSED_ARG_4(z1, z2, z3, z4) \
    (void)(z1);                        \
    (void)(z2);                        \
    (void)(z3);                        \
    (void)(z4)
#define Z_UNUSED_ARG_5(z1, z2, z3, z4, z5) \
    (void)(z1);                            \
    (void)(z2);                            \
    (void)(z3);                            \
    (void)(z4);                            \
    (void)(z5)

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

/*-------- Internal Zenoh Array Macros --------*/
#define _Z_ARRAY_DECLARE(name) ARRAY_DECLARE(_z_##name##_t, name, _z_)
#define _Z_ARRAY_P_DECLARE(name) ARRAY_P_DECLARE(_z_##name##_t, name, _z_)

#define _Z_ARRAY_S_DEFINE(name, arr, len) ARRAY_S_DEFINE(_z_##name##_t, name, _z_, arr, len)
#define _Z_ARRAY_P_S_DEFINE(name, arr, len) ARRAY_P_S_DEFINE(_z_##name##_t, name, _z_, arr, len)

#define _Z_ARRAY_H_DEFINE(name, arr, len) ARRAY_H_DEFINE(_z_##name##_t, name, _z_, arr, len)
#define _Z_ARRAY_S_INIT(name, arr, len) ARRAY_S_INIT(_z_##name##_t, arr, len)

#define _Z_ARRAY_P_S_INIT(name, arr, len) ARRAY_P_S_INIT(_z_##name##_t, arr, len)
#define _Z_ARRAY_H_INIT(name, arr, len) ARRAY_H_INIT(_z_##name##_t, arr, len)

#define _Z_ARRAY_S_COPY(name, dst, src) ARRAY_S_COPY(_z_##name##_t, dst, src)
#define _Z_ARRAY_H_COPY(name, dst, src) ARRAY_H_COPY(_z_##name##_t, dst, src)

/*-------- Dynamically allocated vector --------*/
typedef struct
{
    size_t _capacity;
    size_t _len;
    void **_val;
} _z_vec_t;

_z_vec_t _z_vec_make(size_t capacity);
_z_vec_t _z_vec_clone(const _z_vec_t *v);

size_t _z_vec_len(const _z_vec_t *v);
void _z_vec_append(_z_vec_t *v, void *e);
const void *_z_vec_get(const _z_vec_t *v, size_t i);
void _z_vec_set(_z_vec_t *sv, size_t i, void *e);

void _z_vec_free_inner(_z_vec_t *v);
void _z_vec_free(_z_vec_t *v);

/*-------- Linked List --------*/
typedef int (*_z_list_predicate)(void *, void *);
typedef struct z_list
{
    void *val;
    struct z_list *tail;
} _z_list_t;

extern _z_list_t *_z_list_empty;

_z_list_t *_z_list_of(void *x);
_z_list_t *_z_list_cons(_z_list_t *xs, void *x);

void *_z_list_head(_z_list_t *xs);
_z_list_t *_z_list_tail(_z_list_t *xs);

size_t _z_list_len(_z_list_t *xs);
_z_list_t *_z_list_remove(_z_list_t *xs, _z_list_predicate p, void *arg);

_z_list_t *_z_list_drop_val(_z_list_t *xs, size_t position);
void _z_list_free(_z_list_t **xs);

/*-------- Int Map --------*/
#define DEFAULT_I_MAP_CAPACITY 64

typedef struct
{
    size_t key;
    void *value;
} _z_i_map_entry_t;

typedef struct
{
    _z_list_t **vals;
    size_t capacity;
    size_t len;
} _z_i_map_t;

_z_i_map_t *_z_i_map_empty;
_z_i_map_t *_z_i_map_make(size_t capacity);

size_t _z_i_map_capacity(_z_i_map_t *map);
size_t _z_i_map_len(_z_i_map_t *map);

void _z_i_map_set(_z_i_map_t *map, size_t k, void *v);
void *_z_i_map_get(_z_i_map_t *map, size_t k);
void _z_i_map_remove(_z_i_map_t *map, size_t k);

void _z_i_map_free(_z_i_map_t *map);

#endif /* _ZENOH_C_COLLECTION_H */
