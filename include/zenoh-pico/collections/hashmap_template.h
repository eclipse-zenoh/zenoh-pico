//
// Copyright (c) 2026 ZettaScale Technology
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

// Hashmap with separate chaining using a fixed-size node pool.
//
// Each bucket is the head of an intrusive singly-linked list.  Nodes are
// allocated from a flat pool of _ZP_HASHMAP_TEMPLATE_CAPACITY
// elements — no heap allocation is performed.
//
// User must define the following macros before including this file:
//
// Required:
//   _ZP_HASHMAP_TEMPLATE_KEY_TYPE
//       type of the key
//   _ZP_HASHMAP_TEMPLATE_VAL_TYPE
//       type of the value
//   _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN_NAME(key_ptr) -> size_t
//       hash function for the key
//
// Optional:
//   _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN_NAME(key_a_ptr, key_b_ptr) -> bool
//       equality function for keys (default: memcmp == 0)
//   _ZP_HASHMAP_TEMPLATE_NAME
//       base name for all generated symbols
//       (default: _ZP_CAT(key_type, _ZP_CAT(val_type, hmap)))
//   _ZP_HASHMAP_TEMPLATE_BUCKET_COUNT
//       number of hash buckets
//       (default: 16)
//   _ZP_HASHMAP_TEMPLATE_CAPACITY
//       maximum total number of entries that can be stored
//       (default: _ZP_HASHMAP_TEMPLATE_BUCKET_COUNT)
//   _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN_NAME(key_ptr)
//       destroy a key (default: no-op)
//   _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN_NAME(val_ptr)
//       destroy a value (default: no-op)
//   _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN_NAME(dst_ptr, src_ptr)
//       move a key (default: copy then destroy src)
//   _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN_NAME(dst_ptr, src_ptr)
//       move a value (default: copy then destroy src)

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/collections/cat.h"

// ── Required macros ──────────────────────────────────────────────────────────

#ifndef _ZP_HASHMAP_TEMPLATE_KEY_TYPE
#error "_ZP_HASHMAP_TEMPLATE_KEY_TYPE must be defined before including hashmap_template.h"
#define _ZP_HASHMAP_TEMPLATE_KEY_TYPE int
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_VAL_TYPE
#error "_ZP_HASHMAP_TEMPLATE_VAL_TYPE must be defined before including hashmap_template.h"
#define _ZP_HASHMAP_TEMPLATE_VAL_TYPE int
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN_NAME
#error "_ZP_HASHMAP_TEMPLATE_KEY_HASH_FN_NAME must be defined before including hashmap_template.h"
#endif

// ── Optional macros with defaults ────────────────────────────────────────────
#ifndef _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN_NAME
#define _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN_NAME(key_a_ptr, key_b_ptr) (*(key_a_ptr) == *(key_b_ptr))
#endif

#ifndef _ZP_HASHMAP_TEMPLATE_BUCKET_COUNT
#define _ZP_HASHMAP_TEMPLATE_BUCKET_COUNT 16
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_CAPACITY
#define _ZP_HASHMAP_TEMPLATE_CAPACITY _ZP_HASHMAP_TEMPLATE_BUCKET_COUNT
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_NAME
#define _ZP_HASHMAP_TEMPLATE_NAME _ZP_CAT(_ZP_HASHMAP_TEMPLATE_KEY_TYPE, _ZP_CAT(_ZP_HASHMAP_TEMPLATE_VAL_TYPE, hmap))
#endif

#ifndef _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN_NAME
#define _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN_NAME(x) (void)(x)
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN_NAME
#define _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN_NAME(x) (void)(x)
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN_NAME
#define _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN_NAME(dst, src) \
    *(dst) = *(src);                                    \
    _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN_NAME(src);
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN_NAME
#define _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN_NAME(dst, src) \
    *(dst) = *(src);                                    \
    _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN_NAME(src);
#endif

// ── Internal name helpers ─────────────────────────────────────────────────────

#define _ZP_HASHMAP_TEMPLATE_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, t)
#define _ZP_HASHMAP_TEMPLATE_NODE_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, node_t)

// ── Node ──────────────────────────────────────────────────────────────────────
//
// Each node lives in the flat pool.  _next is an index into the pool,
// or _ZP_HASHMAP_TEMPLATE_CAPACITY when the node is the list tail.
// _occupied == false means the slot is free in the pool.

typedef struct _ZP_HASHMAP_TEMPLATE_NODE_TYPE {
    _ZP_HASHMAP_TEMPLATE_KEY_TYPE key;
    _ZP_HASHMAP_TEMPLATE_VAL_TYPE val;
    size_t _next;  // index of next node in chain, CAPACITY == end-of-list
} _ZP_HASHMAP_TEMPLATE_NODE_TYPE;

// ── Map type ──────────────────────────────────────────────────────────────────
//
// _buckets[b] holds the pool index of the first node in bucket b,
// or CAPACITY when the bucket is empty.

typedef struct _ZP_HASHMAP_TEMPLATE_TYPE {
    _ZP_HASHMAP_TEMPLATE_NODE_TYPE _pool[_ZP_HASHMAP_TEMPLATE_CAPACITY];
    size_t _buckets[_ZP_HASHMAP_TEMPLATE_BUCKET_COUNT];
    size_t _size;       // number of live entries
    size_t _free_head;  // index of first free pool slot (free list via _next)
} _ZP_HASHMAP_TEMPLATE_TYPE;

// ── new ───────────────────────────────────────────────────────────────────────

static inline _ZP_HASHMAP_TEMPLATE_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, new)(void) {
    _ZP_HASHMAP_TEMPLATE_TYPE map;
    // Mark every bucket as empty
    for (size_t b = 0; b < _ZP_HASHMAP_TEMPLATE_BUCKET_COUNT; b++) {
        map._buckets[b] = _ZP_HASHMAP_TEMPLATE_CAPACITY;
    }
    // Build the free list through the pool
    for (size_t i = 0; i < _ZP_HASHMAP_TEMPLATE_CAPACITY; i++) {
        map._pool[i]._next = i + 1;  // points to next free slot; last wraps to CAPACITY
    }
    map._free_head = 0;
    map._size = 0;
    return map;
}

// ── Internal: allocate / free pool node ──────────────────────────────────────

static inline size_t _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, pool_alloc)(_ZP_HASHMAP_TEMPLATE_TYPE *map) {
    if (map->_free_head == _ZP_HASHMAP_TEMPLATE_CAPACITY) {
        return _ZP_HASHMAP_TEMPLATE_CAPACITY;  // pool full
    }
    size_t idx = map->_free_head;
    map->_free_head = map->_pool[idx]._next;
    map->_pool[idx]._next = _ZP_HASHMAP_TEMPLATE_CAPACITY;
    return idx;
}

static inline void _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, pool_free)(_ZP_HASHMAP_TEMPLATE_TYPE *map, size_t idx) {
    map->_pool[idx]._next = map->_free_head;
    map->_free_head = idx;
}

// ── get ───────────────────────────────────────────────────────────────────────
// Returns a pointer to the value for key, or NULL if not found.

static inline _ZP_HASHMAP_TEMPLATE_VAL_TYPE *_ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME,
                                                     get)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                          const _ZP_HASHMAP_TEMPLATE_KEY_TYPE *key) {
    size_t b = _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN_NAME(key) % _ZP_HASHMAP_TEMPLATE_BUCKET_COUNT;
    size_t idx = map->_buckets[b];
    while (idx != _ZP_HASHMAP_TEMPLATE_CAPACITY) {
        _ZP_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_pool[idx];
        if (_ZP_HASHMAP_TEMPLATE_KEY_EQ_FN_NAME(&n->key, key)) {
            return &n->val;
        }
        idx = n->_next;
    }
    return NULL;
}

// ── contains ─────────────────────────────────────────────────────────────────

static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, contains)(const _ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                                const _ZP_HASHMAP_TEMPLATE_KEY_TYPE *key) {
    return _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, get)((_ZP_HASHMAP_TEMPLATE_TYPE *)(uintptr_t)map, key) != NULL;
}

// ── size / is_empty ───────────────────────────────────────────────────────────

static inline size_t _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, size)(const _ZP_HASHMAP_TEMPLATE_TYPE *map) {
    return map->_size;
}

static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, is_empty)(const _ZP_HASHMAP_TEMPLATE_TYPE *map) {
    return map->_size == 0;
}

// ── insert ────────────────────────────────────────────────────────────────────
// Takes ownership of *key and *val via move.
// If key already exists: old value is destroyed, new value is moved in.
// The new key is destroyed (existing key kept).
// Returns a pointer to the new location of inserted node, NULL only when the pool is exhausted and key is not already
// present.

static inline _ZP_HASHMAP_TEMPLATE_NODE_TYPE *_ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME,
                                                      insert)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                              _ZP_HASHMAP_TEMPLATE_KEY_TYPE *key,
                                                              _ZP_HASHMAP_TEMPLATE_VAL_TYPE *val) {
    size_t b = _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN_NAME(key) % _ZP_HASHMAP_TEMPLATE_BUCKET_COUNT;
    // Walk the chain looking for an existing entry with the same key
    size_t idx = map->_buckets[b];
    while (idx != _ZP_HASHMAP_TEMPLATE_CAPACITY) {
        _ZP_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_pool[idx];
        if (_ZP_HASHMAP_TEMPLATE_KEY_EQ_FN_NAME(&n->key, key)) {
            // Update: destroy incoming key, replace value in-place
            _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN_NAME(key);
            _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN_NAME(&n->val);
            _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN_NAME(&n->val, val);
            return n;
        }
        idx = n->_next;
    }
    // New entry — allocate a pool node
    size_t new_idx = _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, pool_alloc)(map);
    if (new_idx == _ZP_HASHMAP_TEMPLATE_CAPACITY) {
        return NULL;  // pool exhausted
    }
    _ZP_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_pool[new_idx];
    _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN_NAME(&n->key, key);
    _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN_NAME(&n->val, val);
    // Prepend to bucket chain (O(1))
    n->_next = map->_buckets[b];
    map->_buckets[b] = new_idx;
    map->_size++;
    return n;
}

// ── remove ────────────────────────────────────────────────────────────────────
// If out_val != NULL the value is moved out; otherwise it is destroyed.
// Returns true if the key was found.

static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, remove)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                              const _ZP_HASHMAP_TEMPLATE_KEY_TYPE *key,
                                                              _ZP_HASHMAP_TEMPLATE_VAL_TYPE *out_val) {
    size_t b = _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN_NAME(key) % _ZP_HASHMAP_TEMPLATE_BUCKET_COUNT;
    size_t prev = _ZP_HASHMAP_TEMPLATE_CAPACITY;  // sentinel: no previous
    size_t idx = map->_buckets[b];
    while (idx != _ZP_HASHMAP_TEMPLATE_CAPACITY) {
        _ZP_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_pool[idx];
        if (_ZP_HASHMAP_TEMPLATE_KEY_EQ_FN_NAME(&n->key, key)) {
            // Unlink from chain
            if (prev == _ZP_HASHMAP_TEMPLATE_CAPACITY) {
                map->_buckets[b] = n->_next;  // was the head
            } else {
                map->_pool[prev]._next = n->_next;
            }
            _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN_NAME(&n->key);
            if (out_val != NULL) {
                _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN_NAME(out_val, &n->val);
            } else {
                _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN_NAME(&n->val);
            }
            _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, pool_free)(map, idx);
            map->_size--;
            return true;
        }
        prev = idx;
        idx = n->_next;
    }
    return false;
}

// ── destroy ─────────────────────────────────────────────────────────────────────
// Destroys all entries and resets the map for reuse (does not free the map).

static inline void _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, destroy)(_ZP_HASHMAP_TEMPLATE_TYPE *map) {
    // Walk every bucket chain and destroy live entries
    for (size_t b = 0; b < _ZP_HASHMAP_TEMPLATE_BUCKET_COUNT; b++) {
        size_t idx = map->_buckets[b];
        while (idx != _ZP_HASHMAP_TEMPLATE_CAPACITY) {
            _ZP_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_pool[idx];
            _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN_NAME(&n->key);
            _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN_NAME(&n->val);
            idx = n->_next;
        }
        map->_buckets[b] = _ZP_HASHMAP_TEMPLATE_CAPACITY;
    }
    // Rebuild the free list
    for (size_t i = 0; i < _ZP_HASHMAP_TEMPLATE_CAPACITY; i++) {
        map->_pool[i]._next = i + 1;
    }
    map->_free_head = 0;
    map->_size = 0;
}

// ── Undef all macros ──────────────────────────────────────────────────────────

#undef _ZP_HASHMAP_TEMPLATE_KEY_TYPE
#undef _ZP_HASHMAP_TEMPLATE_VAL_TYPE
#undef _ZP_HASHMAP_TEMPLATE_NAME
#undef _ZP_HASHMAP_TEMPLATE_BUCKET_COUNT
#undef _ZP_HASHMAP_TEMPLATE_CAPACITY
#undef _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN_NAME
#undef _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN_NAME
#undef _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN_NAME
#undef _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN_NAME
#undef _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN_NAME
#undef _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN_NAME
#undef _ZP_HASHMAP_TEMPLATE_TYPE
#undef _ZP_HASHMAP_TEMPLATE_NODE_TYPE
