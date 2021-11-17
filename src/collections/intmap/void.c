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
#include "zenoh-pico/collections/intmap.h"

/*-------- int-void map --------*/
_z_int_void_map_t _z_int_void_map_make(size_t capacity, _z_int_void_map_entry_f f)
{
    _z_int_void_map_t map;
    _z_int_void_map_init(&map, capacity, f);
    return map;
}

void _z_int_void_map_init(_z_int_void_map_t *map, size_t capacity, _z_int_void_map_entry_f f)
{
    map->capacity = capacity;
    map->len = 0;
    map->vals = NULL;

    map->f = f;
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

void *_z_int_void_map_insert(_z_int_void_map_t *map, size_t k, void *v)
{
    // Lazily allocate
    if (map->vals == NULL)
    {
        map->vals = (z_list_t **)malloc(map->capacity * sizeof(z_list_t *));
        for (size_t i = 0; i < map->capacity; i++)
            map->vals[i] = NULL;
    }

    _z_int_void_map_entry_t *entry = NULL;

    // Compute the hash
    size_t idx = k % map->capacity;
    // Get the list associated to the hash
    z_list_t *xs = map->vals[idx];

    if (xs == z_list_empty)
    {
        entry = (_z_int_void_map_entry_t *)malloc(sizeof(_z_int_void_map_entry_t));
        entry->key = k;
        entry->value = v;
        map->vals[idx] = z_list_cons(z_list_empty, entry);
        map->len++;
    }
    else
    {
        while (xs != z_list_empty)
        {
            entry = (_z_int_void_map_entry_t *)xs->val;
            if (entry->key == k)
            {
                // Free the pointer to the old value
                map->f.free(&entry->value);
                // Update the pointer to current value
                entry->value = v;
                break;
            }
            else
            {
                xs = xs->tail;
            }
        }

        if (xs == z_list_empty)
        {
            entry = (_z_int_void_map_entry_t *)malloc(sizeof(_z_int_void_map_entry_t));
            entry->key = k;
            entry->value = v;
            map->vals[idx] = z_list_cons(map->vals[idx], entry);
            map->len++;
        }
    }

    return v;
}

const void *_z_int_void_map_get(const _z_int_void_map_t *map, size_t k)
{
    if (map->vals == NULL)
        return NULL;

    _z_int_void_map_entry_t *entry = NULL;
    size_t idx = k % map->capacity;
    z_list_t *xs = map->vals[idx];

    while (xs != z_list_empty)
    {
        entry = (_z_int_void_map_entry_t *)xs->val;
        if (entry->key == k)
            return entry->value;
        xs = xs->tail;
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

void *_z_int_void_map_remove(_z_int_void_map_t *map, size_t k)
{
    if (map->vals == NULL)
        return NULL;

    size_t idx = k % map->capacity;
    _z_int_void_map_entry_t e;
    e.key = k;
    size_t l = z_list_len(map->vals[idx]);
    map->vals[idx] = z_list_remove(map->vals[idx], _z_int_void_map_key_predicate, &e);
    map->len -= l - z_list_len(map->vals[idx]);

    // @TODO: we should return the map entry value
    return NULL;
}

void _z_int_void_map_clear(_z_int_void_map_t *map)
{
    if (map->vals == NULL)
        return;

    for (size_t i = 0; i < map->capacity; i++)
    {
        while (map->vals[i])
        {
            _z_int_void_map_entry_t *e = (_z_int_void_map_entry_t *)z_list_head(map->vals[i]);
            map->f.free(&e->value);
            free(e);
            map->vals[i] = z_list_pop(map->vals[i]);
        }
    }

    free(map->vals);
    map->vals = NULL;
}

void _z_int_void_map_free(_z_int_void_map_t **map)
{
    _z_int_void_map_t *ptr = *map;
    _z_int_void_map_clear(ptr);
    *map = NULL;
}
