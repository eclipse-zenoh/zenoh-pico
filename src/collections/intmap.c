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

#include <stdlib.h>
#include <string.h>
#include "zenoh-pico/collections/intmap.h"

/*-------- int-void map --------*/
int _z_int_void_map_entry_key_eq(const void *left, const void *right)
{
    _z_int_void_map_entry_t *l = (_z_int_void_map_entry_t *)left;
    _z_int_void_map_entry_t *r = (_z_int_void_map_entry_t *)right;
    return l->key == r->key;
}

void _z_int_void_map_init(_z_int_void_map_t *map, size_t capacity)
{
    map->capacity = capacity;
    map->vals = NULL;
}

_z_int_void_map_t _z_int_void_map_make(size_t capacity)
{
    _z_int_void_map_t map;
    _z_int_void_map_init(&map, capacity);
    return map;
}

size_t _z_int_void_map_capacity(const _z_int_void_map_t *map)
{
    return map->capacity;
}

size_t _z_int_void_map_len(const _z_int_void_map_t *map)
{
    size_t len = 0;

    if (map->vals == NULL)
        return len;

    for (size_t idx = 0; idx < map->capacity; idx++)
        len += _z_list_len(map->vals[idx]);

    return len;
}

int _z_int_void_map_is_empty(const _z_int_void_map_t *map)
{
    return _z_int_void_map_len(map) == 0;
}

void _z_int_void_map_remove(_z_int_void_map_t *map, size_t k, z_element_free_f f)
{
    if (map->vals == NULL)
        return;

    size_t idx = k % map->capacity;
    _z_int_void_map_entry_t e;
    e.key = k;
    e.val = NULL;

    map->vals[idx] = _z_list_drop_filter(map->vals[idx], f, _z_int_void_map_entry_key_eq, &e);
}

void *_z_int_void_map_insert(_z_int_void_map_t *map, size_t k, void *v, z_element_free_f f_f)
{
    if (map->vals == NULL)
    {
        // Lazily allocate and initialize to NULL all the pointers
        map->vals = (_z_list_t **)malloc(map->capacity * sizeof(_z_list_t *));
        // memset(map->vals, 0, map->capacity * sizeof(_z_list_t *));

        for (size_t idx = 0; idx < map->capacity; idx++)
            map->vals[idx] = NULL;
    }

    // Free any old value
    _z_int_void_map_remove(map, k, f_f);

    // Insert the element
    _z_int_void_map_entry_t *entry = (_z_int_void_map_entry_t *)malloc(sizeof(_z_int_void_map_entry_t));
    entry->key = k;
    entry->val = v;

    size_t idx = k % map->capacity;
    map->vals[idx] = _z_list_push(map->vals[idx], entry);

    return v;
}

void *_z_int_void_map_get(const _z_int_void_map_t *map, size_t k)
{
    if (map->vals == NULL)
        return NULL;

    size_t idx = k % map->capacity;

    _z_int_void_map_entry_t e;
    e.key = k;
    e.val = NULL;

    _z_list_t *xs = _z_list_find(map->vals[idx], _z_int_void_map_entry_key_eq, &e);
    if (xs != NULL)
    {
        _z_int_void_map_entry_t *h = (_z_int_void_map_entry_t *)_z_list_head(xs);
        return h->val;
    }

    return NULL;
}

void _z_int_void_map_clear(_z_int_void_map_t *map, z_element_free_f f_f)
{
    if (map->vals == NULL)
        return;

    for (size_t idx = 0; idx < map->capacity; idx++)
        _z_list_free(&map->vals[idx], f_f);
}

void _z_int_void_map_free(_z_int_void_map_t **map, z_element_free_f f)
{
    _z_int_void_map_t *ptr = *map;
    _z_int_void_map_clear(ptr, f);
    free(ptr->vals);
    *map = NULL;
}
