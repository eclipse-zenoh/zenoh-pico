//
// Copyright (c) 2024 ZettaScale Technology
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

#include "zenoh-pico/collections/lru_cache.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/system/common/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/result.h"

// Nodes are chained as double linked list for lru insertion/deletion.
typedef struct _z_lru_cache_node_data_t {
    _z_lru_cache_node_t *prev;  // List previous node
    _z_lru_cache_node_t *next;  // List next node
} _z_lru_cache_node_data_t;

#define NODE_DATA_SIZE sizeof(_z_lru_cache_node_data_t)

// Generic static functions
static inline _z_lru_cache_t _z_lru_cache_null(void) { return (_z_lru_cache_t){0}; }

static inline _z_lru_cache_node_data_t *_z_lru_cache_node_data(_z_lru_cache_node_t *node) {
    return (_z_lru_cache_node_data_t *)node;
}

static inline void *_z_lru_cache_node_value(_z_lru_cache_node_t *node) {
    return (void *)_z_ptr_u8_offset((uint8_t *)node, (ptrdiff_t)NODE_DATA_SIZE);
}

static _z_lru_cache_node_t *_z_lru_cache_node_create(void *value, size_t value_size) {
    size_t node_size = NODE_DATA_SIZE + value_size;
    _z_lru_cache_node_t *node = z_malloc(node_size);
    if (node == NULL) {
        return node;
    }
    memset(node, 0, NODE_DATA_SIZE);
    memcpy(_z_lru_cache_node_value(node), value, value_size);
    return node;
}

// List functions
static void _z_lru_cache_insert_list_node(_z_lru_cache_t *cache, _z_lru_cache_node_t *node) {
    _z_lru_cache_node_data_t *node_data = _z_lru_cache_node_data(node);
    node_data->prev = NULL;
    node_data->next = cache->head;

    if (cache->head != NULL) {
        _z_lru_cache_node_data_t *head_data = _z_lru_cache_node_data(cache->head);
        head_data->prev = node;
    }
    cache->head = node;
    if (cache->tail == NULL) {
        cache->tail = node;
    }
}

static void _z_lru_cache_remove_list_node(_z_lru_cache_t *cache, _z_lru_cache_node_t *node) {
    _z_lru_cache_node_data_t *node_data = _z_lru_cache_node_data(node);

    // Nominal case
    if ((node_data->prev != NULL) && (node_data->next != NULL)) {
        _z_lru_cache_node_data_t *prev_data = _z_lru_cache_node_data(node_data->prev);
        _z_lru_cache_node_data_t *next_data = _z_lru_cache_node_data(node_data->next);
        prev_data->next = node_data->next;
        next_data->prev = node_data->prev;
    }
    if (node_data->prev == NULL) {
        assert(cache->head == node);
        cache->head = node_data->next;
        if (node_data->next != NULL) {
            _z_lru_cache_node_data_t *next_data = _z_lru_cache_node_data(node_data->next);
            next_data->prev = NULL;
        }
    }
    if (node_data->next == NULL) {
        assert(cache->tail == node);
        cache->tail = node_data->prev;
        if (node_data->prev != NULL) {
            _z_lru_cache_node_data_t *prev_data = _z_lru_cache_node_data(node_data->prev);
            prev_data->next = NULL;
        }
    }
}

static void _z_lru_cache_update_list(_z_lru_cache_t *cache, _z_lru_cache_node_t *node) {
    _z_lru_cache_remove_list_node(cache, node);
    _z_lru_cache_insert_list_node(cache, node);
}

static void _z_lru_cache_clear_list(_z_lru_cache_t *cache, z_element_clear_f clear) {
    _z_lru_cache_node_data_t *node = cache->head;
    while (node != NULL) {
        _z_lru_cache_node_t *tmp = node;
        _z_lru_cache_node_data_t *node_data = _z_lru_cache_node_data(node);
        void *node_value = _z_lru_cache_node_value(node);
        node = node_data->next;
        clear(node_value);
        z_free(tmp);
    }
}

// Sorted list function
static _z_lru_cache_node_t *_z_lru_cache_search_slist(_z_lru_cache_t *cache, void *value, _z_lru_val_cmp_f compare,
                                                      size_t *idx) {
    int l_idx = 0;
    int h_idx = (int)cache->len - 1;
    while (l_idx <= h_idx) {
        int curr_idx = (l_idx + h_idx) / 2;
        int res = compare(_z_lru_cache_node_value(cache->slist[curr_idx]), value);
        if (res == 0) {
            *idx = (size_t)curr_idx;
            return cache->slist[curr_idx];
        } else if (res < 0) {
            l_idx = curr_idx + 1;
        } else {
            h_idx = curr_idx - 1;
        }
    }
    return NULL;
}

static void _z_lru_cache_sort_slist(_z_lru_cache_node_t **slist, size_t slist_size, _z_lru_val_cmp_f compare) {
    for (size_t i = 1; i < slist_size; i++) {
        _z_lru_cache_node_t *node = slist[i];
        void *node_val = _z_lru_cache_node_value(node);
        int j = (int)i - 1;
        while ((j >= 0) && (compare(_z_lru_cache_node_value(slist[j]), node_val) > 0)) {
            slist[j + 1] = slist[j];
            j--;
        }
        slist[j + 1] = node;
    }
}

static void _z_lru_cache_insert_slist(_z_lru_cache_t *cache, _z_lru_cache_node_t *node, _z_lru_val_cmp_f compare,
                                      size_t *idx) {
    if (idx != NULL) {
        assert(*idx < cache->capacity);
        cache->slist[*idx] = node;
    } else {
        assert(cache->len < cache->capacity);
        assert(cache->slist[cache->len] == NULL);
        cache->slist[cache->len] = node;
    }
    _z_lru_cache_sort_slist(cache->slist, cache->len + 1, compare);
}

static size_t _z_lru_cache_delete_slist(_z_lru_cache_t *cache, _z_lru_cache_node_t *node, _z_lru_val_cmp_f compare) {
    size_t idx = 0;
    (void)_z_lru_cache_search_slist(cache, _z_lru_cache_node_value(node), compare, &idx);
    return idx;
}

// Main static functions
static size_t _z_lru_cache_delete_last(_z_lru_cache_t *cache, _z_lru_val_cmp_f compare) {
    _z_lru_cache_node_t *last = cache->tail;
    assert(last != NULL);
    _z_lru_cache_remove_list_node(cache, last);
    size_t idx = _z_lru_cache_delete_slist(cache, last, compare);
    z_free(last);
    cache->len--;
    return idx;
}

static void _z_lru_cache_insert_node(_z_lru_cache_t *cache, _z_lru_cache_node_t *node, _z_lru_val_cmp_f compare,
                                     size_t *idx) {
    _z_lru_cache_insert_list_node(cache, node);
    _z_lru_cache_insert_slist(cache, node, compare, idx);
    cache->len++;
}

static _z_lru_cache_node_t *_z_lru_cache_search_node(_z_lru_cache_t *cache, void *value, _z_lru_val_cmp_f compare) {
    size_t idx = 0;
    return _z_lru_cache_search_slist(cache, value, compare, &idx);
}

// Public functions
_z_lru_cache_t _z_lru_cache_init(size_t capacity) {
    _z_lru_cache_t cache = _z_lru_cache_null();
    cache.capacity = capacity;
    return cache;
}

void *_z_lru_cache_get(_z_lru_cache_t *cache, void *value, _z_lru_val_cmp_f compare) {
    // Lookup if node exists.
    _z_lru_cache_node_t *node = _z_lru_cache_search_node(cache, value, compare);
    if (node == NULL) {
        return NULL;
    }
    // Update list with node as most recent
    _z_lru_cache_update_list(cache, node);
    return _z_lru_cache_node_value(node);
}

z_result_t _z_lru_cache_insert(_z_lru_cache_t *cache, void *value, size_t value_size, _z_lru_val_cmp_f compare) {
    assert(cache->capacity > 0);
    // Init slist
    if (cache->slist == NULL) {
        cache->slist = (_z_lru_cache_node_t **)z_malloc(cache->capacity * sizeof(void *));
        if (cache->slist == NULL) {
            return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        }
        memset(cache->slist, 0, cache->capacity * sizeof(void *));
    }
    // Create node
    _z_lru_cache_node_t *node = _z_lru_cache_node_create(value, value_size);
    if (node == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Check capacity
    size_t *idx = NULL;
    size_t del_idx = 0;
    if (cache->len == cache->capacity) {
        // Delete lru entry
        del_idx = _z_lru_cache_delete_last(cache, compare);
        idx = &del_idx;
    }
    // Update the cache
    _z_lru_cache_insert_node(cache, node, compare, idx);
    return _Z_RES_OK;
}

void _z_lru_cache_clear(_z_lru_cache_t *cache, z_element_clear_f clear) {
    // Reset slist
    if (cache->slist != NULL) {
        memset(cache->slist, 0, cache->capacity * sizeof(void *));
    }
    // Clear list
    _z_lru_cache_clear_list(cache, clear);
    // Reset cacge
    cache->len = 0;
    cache->head = NULL;
    cache->tail = NULL;
}

void _z_lru_cache_delete(_z_lru_cache_t *cache, z_element_clear_f clear) {
    _z_lru_cache_clear(cache, clear);
    z_free(cache->slist);
    cache->slist = NULL;
}
