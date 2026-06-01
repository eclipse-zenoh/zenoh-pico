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
// allocated from a flat pool of _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY
// elements — no heap allocation is performed.
//
// User must define the following macros before including this file:
//
// Required:
//   _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE
//       type of the key
//   _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE
//       type of the value
//   _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN(key_ptr) -> size_t
//       hash function for the key
//
// Optional:
//   _ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN(key_a_ptr, key_b_ptr) -> bool
//       equality function for keys (default: *key_a_ptr == *key_b_ptr)
//   _ZP_STATIC_HASHMAP_TEMPLATE_NAME
//       base name for all generated symbols
//       (default: _ZP_CAT(key_type, _ZP_CAT(val_type, hmap)))
//   _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT
//       number of hash buckets
//       (default: 16)
//   _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY
//       maximum total number of entries that can be stored
//       (default: _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT)
//   _ZP_STATIC_HASHMAP_TEMPLATE_KEY_DESTROY_FN(key_ptr)
//       destroy a key (default: no-op)
//   _ZP_STATIC_HASHMAP_TEMPLATE_VAL_DESTROY_FN(val_ptr)
//       destroy a value (default: no-op)
//   _ZP_STATIC_HASHMAP_TEMPLATE_KEY_MOVE_FN(dst_ptr, src_ptr)
//       move a key (default: copy without destroying src)
//   _ZP_STATIC_HASHMAP_TEMPLATE_VAL_MOVE_FN(dst_ptr, src_ptr)
//       move a value (default: copy without destroying src)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zenoh-pico/collections/cat.h"

#ifndef _ZP_HASHMAP_ITER_INVALID
#define _ZP_HASHMAP_ITER_INVALID 0
#endif

// ── Required macros ──────────────────────────────────────────────────────────

#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE
#error "_ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE must be defined before including static_hashmap_template.h"
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE int
#endif
#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE
#error "_ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE must be defined before including static_hashmap_template.h"
#define _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE int
#endif
#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN
#error "_ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN must be defined before including static_hashmap_template.h"
#endif

// ── Optional macros with defaults ────────────────────────────────────────────
#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN(key_a_ptr, key_b_ptr) (*(key_a_ptr) == *(key_b_ptr))
#endif

#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT
#define _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT 16
#endif
#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY
#define _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT
#endif
#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_NAME
#define _ZP_STATIC_HASHMAP_TEMPLATE_NAME \
    _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE, _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE, hmap))
#endif

// ── Index type selection ──────────────────────────────────────────────────────
//
// Choose the smallest unsigned type whose maximum representable value is
// strictly greater than half-CAPACITY.
// Given that we use msb bit for indicating presence,
// the following mappings are used:
//
//   CAPACITY ≤ 254   → uint8_t   (sentinel = 255)
//   CAPACITY ≤ 65534 → uint16_t  (sentinel = 65535)
//   otherwise        → uint32_t  (sentinel = UINT32_MAX)
#if _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY <= 254
#define _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE uint8_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE ((uint8_t)0xFF)
#elif _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY <= 65534
#define _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE uint16_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE ((uint16_t)0xFFFF)
#else
#define _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE uint32_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE ((uint32_t)0xFFFFFFFF)
#endif

#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_KEY_DESTROY_FN
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_DESTROY_FN(x) (void)(x)
#endif
#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_VAL_DESTROY_FN
#define _ZP_STATIC_HASHMAP_TEMPLATE_VAL_DESTROY_FN(x) (void)(x)
#endif
#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_KEY_MOVE_FN
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_MOVE_FN(dst, src) *(dst) = *(src);
#endif
#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_VAL_MOVE_FN
#define _ZP_STATIC_HASHMAP_TEMPLATE_VAL_MOVE_FN(dst, src) *(dst) = *(src);
#endif

// ── Internal name helpers ─────────────────────────────────────────────────────

#define _ZP_STATIC_HASHMAP_TEMPLATE_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, t)
#define _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, node_t)
#define _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPEDEF _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, iter_t)

// ── Node ──────────────────────────────────────────────────────────────────────
//
// Nodes live in a flat pool. The singly-linked-list "next" pointers + presence bits are stored
// in a separate parallel array (_info) rather than inside the node struct.
// This avoids the tail-padding that the compiler would otherwise insert after
// a small index field to satisfy the alignment requirement of the key/value
// types.

typedef struct _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE {
    _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE key;
    _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE val;
} _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE;

// Public typedef for the index type so callers can store/declare indices without
// spelling out the internal macro.
typedef _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPEDEF;
#define _ZP_HMAP_IDX_TO_ITER(idx) ((idx) + 1u)
#define _ZP_HMAP_ITER_TO_IDX(iter) ((iter) - 1u)
#define _ZP_HMAP_SET_PRESENT(map, idx) (map->_present[(idx) >> 3] |= (1u << ((idx) & 7)))
#define _ZP_HMAP_SET_MISSING(map, idx) (map->_present[(idx) >> 3] &= ~(1u << ((idx) & 7)))
#define _ZP_HMAP_IS_PRESENT(map, idx) ((map->_present[(idx) >> 3] & (1u << ((idx) & 7))) != 0)

// ── Map type ──────────────────────────────────────────────────────────────────
//
// _info[i]     : node info type (index of the next node in the chain and presence bit).
// _buckets[b]  : index of the first node in bucket b, INDEX_NONE = empty.
// _free_head   : index of the first free pool slot (free list via _info).

typedef struct _ZP_STATIC_HASHMAP_TEMPLATE_TYPE {
    _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE _pool[_ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY];
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _next[_ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY];
    uint8_t _present[(_ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY + 1) /
                     sizeof(uint8_t)];  // bitfield presence flags for each node
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _buckets[_ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT];
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _free_head;
    size_t _size;  // number of live entries
} _ZP_STATIC_HASHMAP_TEMPLATE_TYPE;

// ── new ───────────────────────────────────────────────────────────────────────

static inline _ZP_STATIC_HASHMAP_TEMPLATE_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, new)(void) {
    _ZP_STATIC_HASHMAP_TEMPLATE_TYPE map;
    for (size_t b = 0; b < _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT; b++) {
        map._buckets[b] = _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;
    }
    for (_ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE i = 0; i + 1 < _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY; i++) {
        map._next[i] = (_ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE)(i + 1);
    }
    map._next[_ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY - 1] = _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;  // end of free list
    memset(map._present, 0, sizeof(map._present));
    map._free_head = 0;
    map._size = 0;
    return map;
}

// ── Internal: allocate / free pool node ──────────────────────────────────────

static inline _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                                                            pool_alloc)(_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map) {
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx = map->_free_head;
    if (idx == _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE) {
        return _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;  // pool full
    }
    map->_free_head = map->_next[idx];
    _ZP_HMAP_SET_PRESENT(map, idx);
    return idx;
}

static inline void _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, pool_free)(_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                                                        _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx) {
    map->_next[idx] = map->_free_head;
    map->_free_head = idx;
    _ZP_HMAP_SET_MISSING(map, idx);
}

// ── get_iter ──────────────────────────────────────────────────────────────────
// Returns an iterator to the node for key, or _ZP_HASHMAP_ITER_INVALID if not found.
// Note: iterators are stable across insertions and removals of other keys, but become invalid if the same key is
// removed.

static inline _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                                                            get_iter)(const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                                                      const _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE *key) {
    size_t b = _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN(key) % _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT;
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx = map->_buckets[b];
    while (idx != _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE) {
        const _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_pool[idx];
        if (_ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN(&n->key, key)) {
            return _ZP_HMAP_IDX_TO_ITER(idx);
        }
        idx = map->_next[idx];
    }
    return _ZP_HASHMAP_ITER_INVALID;
}

// ── get ───────────────────────────────────────────────────────────────────────
// Returns a pointer to the value for key, or NULL if not found.

static inline _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE *_ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                                                            get)(_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                                                 const _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE *key) {
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx = _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, get_iter)(map, key);
    if (idx != _ZP_HASHMAP_ITER_INVALID) {
        return &map->_pool[_ZP_HMAP_ITER_TO_IDX(idx)].val;
    }
    return NULL;
}

// ── cget ───────────────────────────────────────────────────────────────────────
// Returns a const pointer to the value for key, or NULL if not found.

static inline const _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE *_ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, cget)(
    const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map, const _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE *key) {
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx = _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, get_iter)(map, key);
    if (idx != _ZP_HASHMAP_ITER_INVALID) {
        return &map->_pool[_ZP_HMAP_ITER_TO_IDX(idx)].val;
    }
    return NULL;
}

// ── contains ─────────────────────────────────────────────────────────────────

static inline bool _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                           contains)(const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                     const _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE *key) {
    return _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, get_iter)((_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *)(uintptr_t)map,
                                                               key) != _ZP_HASHMAP_ITER_INVALID;
}

// ── size / is_empty ───────────────────────────────────────────────────────────

static inline size_t _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, size)(const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map) {
    return map->_size;
}

static inline bool _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, is_empty)(const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map) {
    return map->_size == 0;
}

// ── node_at ──────────────────────────────────────────────────────────────────
// Converts a valid index to a pointer to its node.
// Behaviour is undefined if idx is not a valid iterator.
static inline _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *_ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                                                             node_at)(_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                                                      _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx) {
    return &map->_pool[_ZP_HMAP_ITER_TO_IDX(idx)];
}

// ── cnode_at ──────────────────────────────────────────────────────────────────
// Converts a valid index to a const pointer to its node.
// Behaviour is undefined if idx is not a valid iterator.
static inline const _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *_ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, cnode_at)(
    const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map, _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx) {
    return &map->_pool[_ZP_HMAP_ITER_TO_IDX(idx)];
}

// ── insert ────────────────────────────────────────────────────────────────────
// Takes ownership of *key and *val via move.
// If key already exists: old value is destroyed, new value is moved in.
// The new key is destroyed (existing key kept).
// Returns the iterator to the inserted/updated node.
// Returns _ZP_HASHMAP_ITER_INVALID only when the pool is exhausted and the key is not already
// present.

static inline _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                                                            insert)(_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                                                    _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE *key,
                                                                    _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE *val) {
    size_t b = _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN(key) % _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT;
    // Walk the chain looking for an existing entry with the same key
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx = map->_buckets[b];
    while (idx != _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE) {
        _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_pool[idx];
        if (_ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN(&n->key, key)) {
            // Update: destroy incoming key, replace value in-place
            _ZP_STATIC_HASHMAP_TEMPLATE_KEY_DESTROY_FN(key);
            _ZP_STATIC_HASHMAP_TEMPLATE_VAL_DESTROY_FN(&n->val);
            _ZP_STATIC_HASHMAP_TEMPLATE_VAL_MOVE_FN(&n->val, val);
            return _ZP_HMAP_IDX_TO_ITER(idx);
        }
        idx = map->_next[idx];
    }
    // New entry — allocate a pool node
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE new_idx = _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, pool_alloc)(map);
    if (new_idx == _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE) {
        return _ZP_HASHMAP_ITER_INVALID;  // pool exhausted
    }
    _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_pool[new_idx];
    _ZP_STATIC_HASHMAP_TEMPLATE_KEY_MOVE_FN(&n->key, key);
    _ZP_STATIC_HASHMAP_TEMPLATE_VAL_MOVE_FN(&n->val, val);
    // Prepend to bucket chain (O(1))
    map->_next[new_idx] = map->_buckets[b];
    map->_buckets[b] = new_idx;
    map->_size++;
    return _ZP_HMAP_IDX_TO_ITER(new_idx);
}

// ── Iteration ─────────────────────────────────────────────────────────────────
//
// Pattern:
//   for (size_t i = map_iter(&map); i != _ZP_HASHMAP_ITER_INVALID; i = map_iter_next(&map, i)) {
//       map_node_t *n = map_node_at(&map, i);
//       // use n->key, n->val
//   }

// Returns the index of the next live slot after 'pos', or _ZP_HASHMAP_ITER_INVALID.
static inline _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                                                            iter_next)(const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                                                       _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE pos) {
    for (size_t i = pos; i < _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY; i++) {
        if (_ZP_HMAP_IS_PRESENT(map, i)) {
            return _ZP_HMAP_IDX_TO_ITER(i);
        }
    }
    return _ZP_HASHMAP_ITER_INVALID;
}

// Returns the index of the first live slot, or _ZP_HASHMAP_ITER_INVALID if the map is empty.
static inline _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                                                            iter)(const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map) {
    return _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, iter_next)(map, 0);
}

// ── remove_at ────────────────────────────────────────────────────────────────
// Remove the node at the given pool index (obtained from insert or a prior
// lookup).  Behaviour is undefined if idx is _ZP_HASHMAP_ITER_INVALID or has already been
// freed.  If out_val != NULL the value is moved out; otherwise it is destroyed.
// If next_idx != NULL it is set to the iterator of the next node.

static inline void _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                           remove_at)(_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map, _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx,
                                      _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *out_val,
                                      _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE *next_idx) {
    idx = _ZP_HMAP_ITER_TO_IDX(idx);
    _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_pool[idx];
    // Re-derive the bucket from the node's own key so the caller does not need
    // to supply it separately.
    size_t b = _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN(&n->key) % _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT;
    // Walk the chain to find the predecessor and unlink idx.
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE prev = _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE cur = map->_buckets[b];
    while (cur != idx) {
        prev = cur;
        cur = map->_next[cur];
    }
    if (prev == _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE) {
        map->_buckets[b] = map->_next[idx];
    } else {
        map->_next[prev] = map->_next[idx];
    }
    if (out_val != NULL) {
        _ZP_STATIC_HASHMAP_TEMPLATE_KEY_MOVE_FN(&out_val->key, &n->key);
        _ZP_STATIC_HASHMAP_TEMPLATE_VAL_MOVE_FN(&out_val->val, &n->val);
    } else {
        _ZP_STATIC_HASHMAP_TEMPLATE_KEY_DESTROY_FN(&n->key);
        _ZP_STATIC_HASHMAP_TEMPLATE_VAL_DESTROY_FN(&n->val);
    }
    _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, pool_free)(map, idx);
    map->_size--;
    if (next_idx != NULL) {
        *next_idx = _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, iter_next)(map, _ZP_HMAP_IDX_TO_ITER(idx));
    }
}
// ── remove ────────────────────────────────────────────────────────────────────
// If out_val != NULL the value is moved out; otherwise it is destroyed.
// Returns true if the key was found.

static inline bool _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, remove)(_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                                                     const _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE *key,
                                                                     _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE *out_val) {
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx = _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, get_iter)(map, key);
    if (idx == _ZP_HASHMAP_ITER_INVALID) {
        return false;  // not found
    }
    if (out_val != NULL) {
        _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE temp;
        _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, remove_at)(map, idx, &temp, NULL);
        _ZP_STATIC_HASHMAP_TEMPLATE_KEY_DESTROY_FN(&temp.key);  // key is not returned to caller, so destroy it
        _ZP_STATIC_HASHMAP_TEMPLATE_VAL_MOVE_FN(out_val, &temp.val);
    } else {
        _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, remove_at)(map, idx, NULL, NULL);
    }
    return true;
}

// ── destroy ─────────────────────────────────────────────────────────────────────
// Destroys all entries and resets the map for reuse (does not free the map).

static inline void _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, destroy)(_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map) {
    // Walk every bucket chain and destroy live entries
    for (size_t b = 0; b < _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT; b++) {
        _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx = map->_buckets[b];
        while (idx != _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE) {
            _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_pool[idx];
            _ZP_STATIC_HASHMAP_TEMPLATE_KEY_DESTROY_FN(&n->key);
            _ZP_STATIC_HASHMAP_TEMPLATE_VAL_DESTROY_FN(&n->val);
            idx = map->_next[idx];
        }
        map->_buckets[b] = _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;
    }
    // Rebuild the free list
    for (_ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE i = 0; i + 1 < _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY; i++) {
        map->_next[i] = (_ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE)(i + 1);
    }
    map->_next[_ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY - 1] = _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;  // end of free list
    map->_free_head = 0;
    map->_size = 0;
    memset(map->_present, 0, sizeof(map->_present));
}

// ── Undef all macros ──────────────────────────────────────────────────────────

#undef _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE
#undef _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE
#undef _ZP_STATIC_HASHMAP_TEMPLATE_NAME
#undef _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT
#undef _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY
#undef _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN
#undef _ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN
#undef _ZP_STATIC_HASHMAP_TEMPLATE_KEY_DESTROY_FN
#undef _ZP_STATIC_HASHMAP_TEMPLATE_VAL_DESTROY_FN
#undef _ZP_STATIC_HASHMAP_TEMPLATE_KEY_MOVE_FN
#undef _ZP_STATIC_HASHMAP_TEMPLATE_VAL_MOVE_FN
#undef _ZP_STATIC_HASHMAP_TEMPLATE_TYPE
#undef _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE
#undef _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE
#undef _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE
#undef _ZP_STATIC_HASHMAP_TEMPLATE_PRESENCE_MASK
#undef _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_MASK
#undef _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPEDEF
#undef _ZP_HMAP_IDX_TO_ITER
#undef _ZP_HMAP_ITER_TO_IDX
#undef _ZP_HMAP_SET_PRESENT
#undef _ZP_HMAP_SET_MISSING
#undef _ZP_HMAP_IS_PRESENT
