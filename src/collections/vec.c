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
 *     ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "zenoh-pico/collections/element.h"
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

_z_vec_t _z_vec_clone(const _z_vec_t *v, z_element_clone_f clone_f)
{
    _z_vec_t u = _z_vec_make(v->capacity);
    for (size_t i = 0; i < v->len; i++)
        _z_vec_append(&u, clone_f(v->val[i]));
    return u;
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

    v->capacity = 0;
    v->len = 0;
    v->val = NULL;
}

void _z_vec_free(_z_vec_t **v, z_element_free_f free_f)
{
    _z_vec_t *ptr = (_z_vec_t *)*v;
    _z_vec_clear(ptr, free_f);
    *v = NULL;
}

inline size_t _z_vec_len(const _z_vec_t *v)
{
    return v->len;
}

void _z_vec_append(_z_vec_t *v, void *e)
{
    if (v->len == v->capacity)
    {
        // Allocate a new vector
        size_t _capacity = 2 * v->capacity;
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
