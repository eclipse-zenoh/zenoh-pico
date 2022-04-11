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

#include <assert.h>
#include <string.h>
#include "zenoh-pico/collections/vec.h"

/*-------- vec --------*/
inline _z_vec_t _z_vec_make(size_t capacity)
{
    _z_vec_t v;
    v.capacity = capacity;
    v.len = 0;
    v.val = (void **)malloc(sizeof(void *) * capacity);
    return v;
}

void _z_vec_copy(_z_vec_t *dst, const _z_vec_t *src, z_element_clone_f d_f)
{
    dst->capacity = src->capacity;
    dst->len = src->len;
    dst->val = (void **)malloc(sizeof(void *) * src->capacity);
    for (size_t i = 0; i < src->len; i++)
        _z_vec_append(dst, d_f(src->val[i]));
}

void _z_vec_reset(_z_vec_t *v, z_element_free_f free_f)
{
    for (size_t i = 0; i < v->len; i++)
        free_f(&v->val[i]);

    v->len = 0;
}

void _z_vec_clear(_z_vec_t *v, z_element_free_f free_f)
{
    for (size_t i = 0; i < v->len; i++)
        free_f(&v->val[i]);

    free(v->val);
    v->val = NULL;

    v->capacity = 0;
    v->len = 0;
}

void _z_vec_free(_z_vec_t **v, z_element_free_f free_f)
{
    _z_vec_t *ptr = (_z_vec_t *)*v;
    _z_vec_clear(ptr, free_f);

    free(ptr);
    *v = NULL;
}

size_t _z_vec_len(const _z_vec_t *v)
{
    return v->len;
}

int _z_vec_is_empty(const _z_vec_t *v)
{
    return _z_vec_len(v) == 0;
}

void _z_vec_append(_z_vec_t *v, void *e)
{
    if (v->len == v->capacity)
    {
        // Allocate a new vector
        size_t _capacity = (v->capacity << 1) | 0x01;
        void **_val = (void **)malloc(_capacity * sizeof(void *));
        memcpy(_val, v->val, v->capacity * sizeof(void *));

        // Free the old vector
        free(v->val);

        // Update the current vector
        v->val = _val;
        v->capacity = _capacity;
    }

    v->val[v->len] = e;
    v->len++;
}

void *_z_vec_get(const _z_vec_t *v, size_t i)
{
    assert(i < v->len);

    return v->val[i];
}

void _z_vec_set(_z_vec_t *v, size_t i, void *e, z_element_free_f free_f)
{
    assert(i < v->len);

    if (v->val[i] != NULL)
        free_f(&v->val[i]);
    v->val[i] = e;
}

void _z_vec_remove(_z_vec_t *v, size_t pos, z_element_free_f free_f)
{
    free_f(&v->val[pos]);
    for (size_t i = pos; i < v->len; i++)
        v->val[pos] = v->val[pos + 1];

    v->val[v->len] = NULL;
    v->len--;
}
