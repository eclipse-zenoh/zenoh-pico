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

// Nodes are chained both as a binary tree for lookup and as double linked list for lru insertion/deletion.
typedef struct _z_lru_cache_node_data_t {
    _z_lru_cache_node_t *prev;  // List previous node
    _z_lru_cache_node_t *next;  // List next node
#if Z_FEATURE_CACHE_TREE == 1
    _z_lru_cache_node_t *left;   // Tree left child
    _z_lru_cache_node_t *right;  // Tree right child
#endif
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

// Tree functions
#if Z_FEATURE_CACHE_TREE == 1
static _z_lru_cache_node_t *_z_lru_cache_insert_tree_node_rec(_z_lru_cache_node_t *previous, _z_lru_cache_node_t *node,
                                                              _z_lru_val_cmp_f compare) {
    if (previous == NULL) {
        return node;
    }
    _z_lru_cache_node_data_t *prev_data = _z_lru_cache_node_data(previous);
    int res = compare(_z_lru_cache_node_value(previous), _z_lru_cache_node_value(node));
    if (res < 0) {
        prev_data->left = _z_lru_cache_insert_tree_node_rec(prev_data->left, node, compare);
    } else if (res > 0) {
        prev_data->right = _z_lru_cache_insert_tree_node_rec(prev_data->right, node, compare);
    }
    // == 0 shouldn't happen
    else {
        _Z_ERROR("Comparison equality shouldn't happen during tree insertion");
    }
    return previous;
}

static void _z_lru_cache_insert_tree_node(_z_lru_cache_t *cache, _z_lru_cache_node_t *node, _z_lru_val_cmp_f compare) {
    cache->root = _z_lru_cache_insert_tree_node_rec(cache->root, node, compare);
}

static _z_lru_cache_node_t *_z_lru_cache_search_tree_rec(_z_lru_cache_node_t *node, void *value,
                                                         _z_lru_val_cmp_f compare) {
    // Check value
    if (node == NULL) {
        return NULL;
    }
    // Compare keys
    int res = compare(_z_lru_cache_node_value(node), value);
    // Node has key
    if (res == 0) {
        return node;
    }
    _z_lru_cache_node_data_t *node_data = _z_lru_cache_node_data(node);
    // Key is greater than node's key
    if (res > 0) {
        return _z_lru_cache_search_tree_rec(node_data->right, value, compare);
    }
    // Key is smaller than node's key
    return _z_lru_cache_search_tree_rec(node_data->left, value, compare);
}

static _z_lru_cache_node_t *_z_lru_cache_search_tree(_z_lru_cache_t *cache, void *value, _z_lru_val_cmp_f compare) {
    return _z_lru_cache_search_tree_rec(cache->root, value, compare);
}

static _z_lru_cache_node_t *_z_lru_cache_get_tree_node_successor(_z_lru_cache_node_t *node) {
    _z_lru_cache_node_data_t *node_data = _z_lru_cache_node_data(node);
    node = node_data->right;
    node_data = _z_lru_cache_node_data(node);
    while ((node != NULL) && (node_data->left != NULL)) {
        node = node_data->left;
        node_data = _z_lru_cache_node_data(node);
    }
    return node;
}

static void _z_lru_cache_transfer_tree_node(_z_lru_cache_node_t *dst, _z_lru_cache_node_t *src, size_t value_size) {
    void *src_val = _z_lru_cache_node_value(src);
    void *dst_val = _z_lru_cache_node_value(dst);
    _z_lru_cache_node_data_t *src_data = _z_lru_cache_node_data(src);
    _z_lru_cache_node_data_t *dst_data = _z_lru_cache_node_data(dst);
    // Copy content
    memcpy(dst_val, src_val, value_size);
    dst_data->prev = src_data->prev;
    dst_data->next = src_data->next;
    // Update list
    if (src_data->prev != NULL) {
        _z_lru_cache_node_data_t *prev_data = _z_lru_cache_node_data(src_data->prev);
        prev_data->next = dst;
    }
    if (src_data->next != NULL) {
        _z_lru_cache_node_data_t *next_data = _z_lru_cache_node_data(src_data->next);
        next_data->prev = dst;
    }
}

static _z_lru_cache_node_t *_z_lru_cache_delete_tree_node_rec(_z_lru_cache_node_t *node, void *value, size_t value_size,
                                                              _z_lru_val_cmp_f compare) {
    // No node
    if (node == NULL) {
        return NULL;
    }
    // Compare value
    _z_lru_cache_node_data_t *node_data = _z_lru_cache_node_data(node);
    void *node_val = _z_lru_cache_node_value(node);
    int res = compare(node_val, value);
    if (res < 0) {
        node_data->left = _z_lru_cache_delete_tree_node_rec(node_data->left, value, value_size, compare);
    } else if (res > 0) {
        node_data->right = _z_lru_cache_delete_tree_node_rec(node_data->right, value, value_size, compare);
    } else {  // Node found
        // No child / one child
        if (node_data->left == NULL) {
            _z_lru_cache_node_t *temp = node_data->right;
            z_free(node);
            return temp;
        } else if (node_data->right == NULL) {
            _z_lru_cache_node_t *temp = node_data->left;
            z_free(node);
            return temp;
        }
        // Two children
        _z_lru_cache_node_t *succ = _z_lru_cache_get_tree_node_successor(node);
        // Transfer successor data to node
        _z_lru_cache_transfer_tree_node(node, succ, value_size);
        node_data->right =
            _z_lru_cache_delete_tree_node_rec(node_data->right, _z_lru_cache_node_value(succ), value_size, compare);
    }
    return node;
}

static void _z_lru_cache_delete_tree_node(_z_lru_cache_t *cache, _z_lru_cache_node_t *node, size_t value_size,
                                          _z_lru_val_cmp_f compare) {
    cache->root = _z_lru_cache_delete_tree_node_rec(cache->root, _z_lru_cache_node_value(node), value_size, compare);
}
#else
_z_lru_cache_node_t *_z_lru_cache_search_list(_z_lru_cache_t *cache, void *value, _z_lru_val_cmp_f compare) {
    if (cache->head == NULL) {
        return NULL;
    }
    _z_lru_cache_node_data_t *node = cache->head;
    //  Parse list
    while (node != NULL) {
        _z_lru_cache_node_data_t *node_data = _z_lru_cache_node_data(node);
        void *node_val = _z_lru_cache_node_value(node);
        if (compare(node_val, value) == 0) {
            return node;
        }
        node = node_data->next;
    }
    return NULL;
}
#endif

// Main static functions
static void _z_lru_cache_delete_last(_z_lru_cache_t *cache, size_t value_size, _z_lru_val_cmp_f compare) {
    _z_lru_cache_node_t *last = cache->tail;
    assert(last != NULL);
    _z_lru_cache_remove_list_node(cache, last);
#if Z_FEATURE_CACHE_TREE == 1
    _z_lru_cache_delete_tree_node(cache, last, value_size, compare);
#else
    _ZP_UNUSED(compare);
    _ZP_UNUSED(value_size);
    z_free(last);
#endif
    cache->len--;
}

static void _z_lru_cache_insert_node(_z_lru_cache_t *cache, _z_lru_cache_node_t *node, _z_lru_val_cmp_f compare) {
    _z_lru_cache_insert_list_node(cache, node);
#if Z_FEATURE_CACHE_TREE == 1
    _z_lru_cache_insert_tree_node(cache, node, compare);
#else
    _ZP_UNUSED(compare);
#endif
    cache->len++;
}

static _z_lru_cache_node_t *_z_lru_cache_search_node(_z_lru_cache_t *cache, void *value, _z_lru_val_cmp_f compare) {
#if Z_FEATURE_CACHE_TREE == 1
    return _z_lru_cache_search_tree(cache, value, compare);
#else
    return _z_lru_cache_search_list(cache, value, compare);
#endif
}

// Public functions
_z_lru_cache_t _z_lru_cache_init(size_t capacity) {
    _z_lru_cache_t cache = _z_lru_cache_null();
    cache.capacity = capacity;
    return cache;
}

void *_z_lru_cache_get(_z_lru_cache_t *cache, void *value, _z_lru_val_cmp_f compare) {
    // Lookup tree if node exists.
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
    // Create node
    _z_lru_cache_node_t *node = _z_lru_cache_node_create(value, value_size);
    if (node == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Check capacity
    if (cache->len == cache->capacity) {
        // Delete lru entry
        _z_lru_cache_delete_last(cache, value_size, compare);
    }
    // Update the cache
    _z_lru_cache_insert_node(cache, node, compare);
    return _Z_RES_OK;
}

void _z_lru_cache_delete(_z_lru_cache_t *cache) {
    _z_lru_cache_node_data_t *node = cache->head;
    //  Parse list
    while (node != NULL) {
        _z_lru_cache_node_t *tmp = node;
        _z_lru_cache_node_data_t *node_data = _z_lru_cache_node_data(node);
        node = node_data->next;
        z_free(tmp);
    }
}
