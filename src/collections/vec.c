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
#include "zenoh-pico/utils/collections.h"

/*-------- vec --------*/
inline z_vec_t z_vec_make(size_t capacity)
{
    z_vec_t v;
    v._capacity = capacity;
    v._len = 0;
    v._val = (void **)malloc(sizeof(void *) * capacity);
    return v;
}

z_vec_t z_vec_clone(const z_vec_t *v)
{
    z_vec_t u = z_vec_make(v->_capacity);
    for (size_t i = 0; i < v->_len; ++i)
        z_vec_append(&u, v->_val[i]);
    return u;
}

void z_vec_clear(z_vec_t *v)
{
    free(v->_val);
    v->_len = 0;
    v->_capacity = 0;
    v->_val = NULL;
}

void z_vec_free(z_vec_t *v)
{
    for (size_t i = 0; i < v->_len; ++i)
        free(v->_val[i]);
    z_vec_clear(v);
}

inline size_t z_vec_len(const z_vec_t *v)
{
    return v->_len;
}

void z_vec_append(z_vec_t *v, void *e)
{
    if (v->_len == v->_capacity)
    {
        // Allocate a new vector
        size_t _capacity = 2 * v->_capacity;
        void **_val = (void **)malloc(_capacity * sizeof(void *));
        memcpy(_val, v->_val, v->_capacity * sizeof(void *));
        // Free the old vector
        free(v->_val);
        // Update the current vector
        v->_val = _val;
        v->_capacity = _capacity;
    }

    v->_val[v->_len] = e;
    v->_len++;
}

const void *z_vec_get(const z_vec_t *v, size_t i)
{
    assert(i < v->_len);
    return v->_val[i];
}

void z_vec_set(z_vec_t *v, size_t i, void *e)
{
    assert(i < v->_capacity);
    v->_val[i] = e;
}
