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

#include "zenoh-pico/collections/intmap.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

/*-------- int-void map --------*/
bool _z_int_void_map_entry_key_eq(const void *left, const void *right) {
    _z_int_void_map_entry_t *l = (_z_int_void_map_entry_t *)left;
    _z_int_void_map_entry_t *r = (_z_int_void_map_entry_t *)right;
    return l->_key == r->_key;
}

void _z_int_void_map_init(_z_int_void_map_t *map, size_t capacity) {
    map->_capacity = capacity;
    map->_vals = NULL;
}

_z_int_void_map_t _z_int_void_map_make(size_t capacity) {
    _z_int_void_map_t map;
    _z_int_void_map_init(&map, capacity);
    return map;
}

size_t _z_int_void_map_capacity(const _z_int_void_map_t *map) { return map->_capacity; }

size_t _z_int_void_map_len(const _z_int_void_map_t *map) {
    size_t len = 0;

    if (map->_vals != NULL) {
        for (size_t idx = 0; idx < map->_capacity; idx++) {
            len = len + _z_list_len(map->_vals[idx]);
        }
    }

    return len;
}

z_result_t _z_int_void_map_copy(_z_int_void_map_t *dst, const _z_int_void_map_t *src, z_element_clone_f f_c) {
    assert((dst != NULL) && (src != NULL) && (dst->_capacity == src->_capacity));
    for (size_t idx = 0; idx < src->_capacity; idx++) {
        const _z_list_t *src_list = src->_vals[idx];
        if (src_list == NULL) {
            continue;
        }
        // Allocate entry
        dst->_vals[idx] = _z_list_clone(src_list, f_c);
        if (dst->_vals[idx] == NULL) {
            return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        }
    }
    return _Z_RES_OK;
}

_z_int_void_map_t _z_int_void_map_clone(const _z_int_void_map_t *src, z_element_clone_f f_c, z_element_free_f f_f) {
    _z_int_void_map_t dst = {._capacity = src->_capacity, ._vals = NULL};
    if (src->_vals == NULL) {
        return dst;
    }
    // Lazily allocate and initialize to NULL all the pointers
    size_t len = dst._capacity * sizeof(_z_list_t *);
    dst._vals = (_z_list_t **)z_malloc(len);
    if (dst._vals == NULL) {
        return dst;
    }
    (void)memset(dst._vals, 0, len);
    // Copy elements
    if (_z_int_void_map_copy(&dst, src, f_c) != _Z_RES_OK) {
        // Free the map
        _z_int_void_map_clear(&dst, f_f);
    }
    return dst;
}

bool _z_int_void_map_is_empty(const _z_int_void_map_t *map) { return _z_int_void_map_len(map) == (size_t)0; }

void _z_int_void_map_remove(_z_int_void_map_t *map, size_t k, z_element_free_f f) {
    if (map->_vals != NULL) {
        size_t idx = k % map->_capacity;
        _z_int_void_map_entry_t e;
        e._key = k;
        e._val = NULL;

        map->_vals[idx] = _z_list_drop_filter(map->_vals[idx], f, _z_int_void_map_entry_key_eq, &e);
    }
}

void *_z_int_void_map_insert(_z_int_void_map_t *map, size_t k, void *v, z_element_free_f f_f, bool replace) {
    if (map->_vals == NULL) {
        // Lazily allocate and initialize to NULL all the pointers
        size_t len = map->_capacity * sizeof(_z_list_t *);
        map->_vals = (_z_list_t **)z_malloc(len);
        if (map->_vals != NULL) {
            (void)memset(map->_vals, 0, len);
        }
    }

    if (map->_vals != NULL) {
        if (replace) {
            // Free any old value
            _z_int_void_map_remove(map, k, f_f);
        }

        // Insert the element
        _z_int_void_map_entry_t *entry = (_z_int_void_map_entry_t *)z_malloc(sizeof(_z_int_void_map_entry_t));
        if (entry != NULL) {
            entry->_key = k;
            entry->_val = v;

            size_t idx = k % map->_capacity;
            map->_vals[idx] = _z_list_push(map->_vals[idx], entry);
        }
    }

    return v;
}

void *_z_int_void_map_get(const _z_int_void_map_t *map, size_t k) {
    void *ret = NULL;

    if (map->_vals != NULL) {
        size_t idx = k % map->_capacity;

        _z_int_void_map_entry_t e;
        e._key = k;
        e._val = NULL;

        _z_list_t *xs = _z_list_find(map->_vals[idx], _z_int_void_map_entry_key_eq, &e);
        if (xs != NULL) {
            _z_int_void_map_entry_t *h = (_z_int_void_map_entry_t *)_z_list_head(xs);
            ret = h->_val;
        }
    }

    return ret;
}

_z_list_t *_z_int_void_map_get_all(const _z_int_void_map_t *map, size_t k) {
    if (map->_vals != NULL) {
        size_t idx = k % map->_capacity;

        _z_int_void_map_entry_t e;
        e._key = k;
        e._val = NULL;

        return _z_list_find(map->_vals[idx], _z_int_void_map_entry_key_eq, &e);
    }
    return NULL;
}

_z_int_void_map_iterator_t _z_int_void_map_iterator_make(const _z_int_void_map_t *map) {
    _z_int_void_map_iterator_t iter = {0};

    iter._map = map;

    return iter;
}

bool _z_int_void_map_iterator_next(_z_int_void_map_iterator_t *iter) {
    if (iter->_map->_vals == NULL) {
        return false;
    }

    while (iter->_idx < iter->_map->_capacity) {
        if (iter->_list_ptr == NULL) {
            iter->_list_ptr = iter->_map->_vals[iter->_idx];
        } else {
            iter->_list_ptr = _z_list_tail(iter->_list_ptr);
        }
        if (iter->_list_ptr == NULL) {
            iter->_idx++;
            continue;
        }

        iter->_entry = iter->_list_ptr->_val;

        return true;
    }
    return false;
}

size_t _z_int_void_map_iterator_key(const _z_int_void_map_iterator_t *iter) { return iter->_entry->_key; }

void *_z_int_void_map_iterator_value(const _z_int_void_map_iterator_t *iter) { return iter->_entry->_val; }

void _z_int_void_map_clear(_z_int_void_map_t *map, z_element_free_f f_f) {
    if (map->_vals != NULL) {
        for (size_t idx = 0; idx < map->_capacity; idx++) {
            _z_list_free(&map->_vals[idx], f_f);
        }

        z_free(map->_vals);
        map->_vals = NULL;
    }
}

void _z_int_void_map_free(_z_int_void_map_t **map, z_element_free_f f) {
    _z_int_void_map_t *ptr = *map;
    if (ptr != NULL) {
        _z_int_void_map_clear(ptr, f);

        z_free(ptr);
        *map = NULL;
    }
}
