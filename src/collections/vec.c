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

#include "zenoh-pico/collections/vec.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

/*-------- vec --------*/
_z_vec_t _z_vec_make(size_t capacity) {
    _z_vec_t v = {._capacity = capacity, ._len = 0, ._val = NULL};
    if (capacity != 0) {
        v._val = (void **)z_malloc(sizeof(void *) * capacity);
    }
    if (v._val != NULL) {
        v._capacity = capacity;
    }
    return v;
}

void _z_vec_copy(_z_vec_t *dst, const _z_vec_t *src, z_element_clone_f d_f) {
    dst->_capacity = src->_capacity;
    dst->_len = src->_len;
    dst->_val = (void **)z_malloc(sizeof(void *) * src->_capacity);
    if (dst->_val != NULL) {
        for (size_t i = 0; i < src->_len; i++) {
            dst->_val[i] = d_f(src->_val[i]);
        }
    }
}

void _z_vec_reset(_z_vec_t *v, z_element_free_f free_f) {
    for (size_t i = 0; i < v->_len; i++) {
        free_f(&v->_val[i]);
    }

    v->_len = 0;
}

void _z_vec_clear(_z_vec_t *v, z_element_free_f free_f) {
    for (size_t i = 0; i < v->_len; i++) {
        free_f(&v->_val[i]);
    }
    _z_vec_release(v);
}

void _z_vec_release(_z_vec_t *v) {
    z_free(v->_val);
    v->_val = NULL;

    v->_capacity = 0;
    v->_len = 0;
}

void _z_vec_free(_z_vec_t **v, z_element_free_f free_f) {
    _z_vec_t *ptr = (_z_vec_t *)*v;

    if (ptr != NULL) {
        _z_vec_clear(ptr, free_f);

        z_free(ptr);
        *v = NULL;
    }
}

size_t _z_vec_len(const _z_vec_t *v) { return v->_len; }

_Bool _z_vec_is_empty(const _z_vec_t *v) { return v->_len == 0; }

void _z_vec_append(_z_vec_t *v, void *e) {
    if (v->_len == v->_capacity) {
        // Allocate a new vector
        size_t _capacity = (v->_capacity << 1) | 0x01;
        void **_val = (void **)z_malloc(_capacity * sizeof(void *));
        if (_val != NULL) {
            (void)memcpy(_val, v->_val, v->_capacity * sizeof(void *));

            // Free the old vector
            z_free(v->_val);

            // Update the current vector
            v->_val = _val;
            v->_capacity = _capacity;

            v->_val[v->_len] = e;
            v->_len = v->_len + 1;
        }
    } else {
        v->_val[v->_len] = e;
        v->_len = v->_len + 1;
    }
}

void *_z_vec_get(const _z_vec_t *v, size_t i) {
    assert(i < v->_len);

    return v->_val[i];
}

void _z_vec_set(_z_vec_t *v, size_t i, void *e, z_element_free_f free_f) {
    assert(i < v->_len);

    if (v->_val[i] != NULL) {
        free_f(&v->_val[i]);
    }
    v->_val[i] = e;
}

void _z_vec_remove(_z_vec_t *v, size_t pos, z_element_free_f free_f) {
    free_f(&v->_val[pos]);
    for (size_t i = pos; i < v->_len; i++) {
        v->_val[pos] = v->_val[pos + (size_t)1];
    }

    v->_val[v->_len] = NULL;
    v->_len = v->_len - 1;
}

/*-------- svec --------*/
_z_svec_t _z_svec_make(size_t capacity, size_t element_size) {
    _z_svec_t v = {._capacity = 0, ._len = 0, ._val = NULL};
    if (capacity != 0) {
        v._val = z_malloc(element_size * capacity);
    }
    if (v._val != NULL) {
        v._capacity = capacity;
    }
    return v;
}

void __z_svec_copy_inner(void *dst, const void *src, z_element_copy_f copy, size_t num_elements, size_t element_size) {
    if (copy == NULL) {
        memcpy(dst, src, num_elements * element_size);
    } else {
        size_t offset = 0;
        for (size_t i = 0; i < num_elements; i++) {
            copy((uint8_t *)dst + offset, (uint8_t *)src + offset);
            offset += element_size;
        }
    }
}

void __z_svec_move_inner(void *dst, void *src, z_element_move_f move, size_t num_elements, size_t element_size) {
    if (move == NULL) {
        memcpy(dst, src, num_elements * element_size);
    } else {
        size_t offset = 0;
        for (size_t i = 0; i < num_elements; i++) {
            move((uint8_t *)dst + offset, (uint8_t *)src + offset);
            offset += element_size;
        }
    }
}

_Bool _z_svec_copy(_z_svec_t *dst, const _z_svec_t *src, z_element_copy_f copy, size_t element_size) {
    dst->_capacity = 0;
    dst->_len = 0;
    dst->_val = z_malloc(element_size * src->_capacity);
    if (dst->_val != NULL) {
        dst->_capacity = src->_capacity;
        dst->_len = src->_len;
        __z_svec_copy_inner(dst->_val, src->_val, copy, src->_len, element_size);
    }
    return dst->_len == src->_len;
}

void _z_svec_reset(_z_svec_t *v, z_element_clear_f clear, size_t element_size) {
    if (clear != NULL) {
        size_t offset = 0;
        for (size_t i = 0; i < v->_len; i++) {
            clear((uint8_t *)v->_val + offset);
            offset += element_size;
        }
    }
    v->_len = 0;
}

void _z_svec_clear(_z_svec_t *v, z_element_clear_f clear_f, size_t element_size) {
    _z_svec_reset(v, clear_f, element_size);
    _z_svec_release(v);
}

void _z_svec_release(_z_svec_t *v) {
    z_free(v->_val);
    v->_val = NULL;

    v->_capacity = 0;
    v->_len = 0;
}

void _z_svec_free(_z_svec_t **v, z_element_clear_f clear, size_t element_size) {
    _z_svec_t *ptr = (_z_svec_t *)*v;

    if (ptr != NULL) {
        _z_svec_clear(ptr, clear, element_size);

        z_free(ptr);
        *v = NULL;
    }
}

size_t _z_svec_len(const _z_svec_t *v) { return v->_len; }

_Bool _z_svec_is_empty(const _z_svec_t *v) { return v->_len == 0; }

bool _z_svec_append(_z_svec_t *v, const void *e, z_element_move_f move, size_t element_size) {
    if (v->_len == v->_capacity) {
        // Allocate a new vector
        size_t _capacity = v->_capacity == 0 ? 1 : (v->_capacity << 1);
        void *_val = (void *)z_malloc(_capacity * element_size);
        if (_val != NULL) {
            __z_svec_move_inner(_val, v->_val, move, v->_len, element_size);
            // Free the old data
            z_free(v->_val);

            // Update the current vector
            v->_val = _val;
            v->_capacity = _capacity;
            memcpy((uint8_t *)v->_val + v->_len * element_size, e, element_size);
            v->_len++;
        } else {
            return false;
        }
    } else {
        memcpy((uint8_t *)v->_val + v->_len * element_size, e, element_size);
        v->_len++;
    }
    return true;
}

void *_z_svec_get(const _z_svec_t *v, size_t i, size_t element_size) {
    assert(i < v->_len);
    return (uint8_t *)v->_val + i * element_size;
}

void _z_svec_set(_z_svec_t *v, size_t i, void *e, z_element_clear_f clear, size_t element_size) {
    assert(i < v->_len);
    clear((uint8_t *)v->_val + i * element_size);
    memcpy((uint8_t *)v->_val + i * element_size, e, element_size);
}

void _z_svec_remove(_z_svec_t *v, size_t pos, z_element_clear_f clear, z_element_move_f move, size_t element_size) {
    assert(pos < v->_len);
    clear((uint8_t *)v->_val + pos * element_size);
    __z_svec_move_inner((uint8_t *)v->_val + pos * element_size, (uint8_t *)v->_val + (pos + 1) * element_size, move,
                        (v->_len - pos - 1) * element_size, element_size);

    v->_len--;
}
