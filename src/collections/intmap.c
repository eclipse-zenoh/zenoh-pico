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

/*-------- int-void entry --------*/
_z_int_void_map_entry_t *_z_int_void_map_entry_make(unsigned int key, void *value)
{
    _z_int_void_map_entry_t *entry = (_z_int_void_map_entry_t *)malloc(sizeof(_z_int_void_map_entry_t));
    entry->key = key;
    entry->value = value;
    return entry;
}

void _z_int_void_map_entry_free(_z_int_void_map_entry_t **e, z_element_free_f f)
{
    _z_int_void_map_entry_t *entry = (_z_int_void_map_entry_t *)*e;
    f(&entry->value);
    free(entry);
    *e = NULL;
}

/*-------- int-void map --------*/
void _z_int_void_map_init(_z_int_void_map_t *map, size_t capacity)
{
    map->capacity = capacity;
    map->len = 0;
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
    return map->len;
}

int _z_int_void_map_is_empty(const _z_int_void_map_t *map)
{
    return _z_int_void_map_len(map) == 0;
}

void *_z_int_void_map_insert(_z_int_void_map_t *map, size_t k, void *v, z_element_free_f f)
{
    if (map->vals == NULL)
    {
        // Lazily allocate and initialize to NULL all the pointers
        map->vals = (_z_list_t **)malloc(map->capacity * sizeof(_z_list_t *));
        // memset(map->vals, 0, map->capacity * sizeof(_z_list_t *));

        for (size_t idx = 0; idx < map->capacity; idx++)
            map->vals[idx] = NULL;
    }

    // Compute the hash
    size_t idx = k % map->capacity;

    // Get the list associated to the hash
    _z_list_t *xs = map->vals[idx];

    while (xs != NULL)
    {
        _z_int_void_map_entry_t *entry = (_z_int_void_map_entry_t *)_z_list_head(xs);
        if (entry->key == k)
        {
            // Free the pointer to the old value
            f(&entry->value);
            // Update the pointer to current value
            entry->value = v;

            return v;
        }
        else
        {
            xs = _z_list_tail(xs);
        }
    }

    if (xs == NULL)
    {
        _z_int_void_map_entry_t *entry = _z_int_void_map_entry_make(k, v);
        map->vals[idx] = _z_list_cons(map->vals[idx], entry);
        map->len++;
    }

    return v;
}

void *_z_int_void_map_get(const _z_int_void_map_t *map, size_t k)
{
    if (map->vals == NULL)
        return NULL;

    size_t idx = k % map->capacity;
    _z_list_t *xs = map->vals[idx];

    while (xs != NULL)
    {
        _z_int_void_map_entry_t *entry = (_z_int_void_map_entry_t *)_z_list_head(xs);
        if (entry->key == k)
            return entry->value;
        xs = _z_list_tail(xs);
    }

    return NULL;
}

int _z_int_void_map_key_predicate(void *current, void *desired)
{
    _z_int_void_map_entry_t *c = (_z_int_void_map_entry_t *)current;
    _z_int_void_map_entry_t *d = (_z_int_void_map_entry_t *)desired;
    if (c->key == d->key)
        return 1;
    return 0;
}

void _z_int_void_map_remove(_z_int_void_map_t *map, size_t k, z_element_free_f f)
{
    if (map->vals == NULL)
        return;

    size_t idx = k % map->capacity;
    _z_int_void_map_entry_t e;
    e.key = k;

    size_t l = _z_list_len(map->vals[idx]);

    map->vals[idx] = _z_list_drop_filter(map->vals[idx], _z_int_void_map_key_predicate, &e, f);
    map->len -= l - _z_list_len(map->vals[idx]);
}

void _z_int_void_map_clear(_z_int_void_map_t *map, z_element_free_f f)
{
    if (map->vals == NULL)
        return;

    for (size_t idx = 0; idx < map->capacity; idx++)
    {
        _z_list_t *xs = map->vals[idx];
        while (xs != NULL)
        {
            _z_int_void_map_entry_t *entry = (_z_int_void_map_entry_t *)_z_list_head(xs);
            _z_int_void_map_entry_free(&entry, f);
            xs = _z_list_tail(xs);
        }
        map->vals[idx] = NULL;
    }
}

void _z_int_void_map_free(_z_int_void_map_t **map, z_element_free_f f)
{
    _z_int_void_map_t *ptr = *map;
    _z_int_void_map_clear(ptr, f);
    free(ptr->vals);
    *map = NULL;
}
