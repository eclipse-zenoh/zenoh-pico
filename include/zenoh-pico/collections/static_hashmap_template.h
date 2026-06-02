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
//   _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY
//       maximum total number of entries that can be stored;
//       this is also used as the number of hash buckets
//       (default: 16)
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

#include "zenoh-pico/collections/cat.h"

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

#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY
#define _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY 16
#endif
// Bucket count is always equal to the capacity. Any user-provided value is ignored.
#ifdef _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT
#undef _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT
#endif
#define _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY
#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_NAME
#define _ZP_STATIC_HASHMAP_TEMPLATE_NAME \
    _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE, _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE, hmap))
#endif

// ── Index type selection ──────────────────────────────────────────────────────
//
// Choose the smallest unsigned type able to represent every valid index plus a
// dedicated sentinel value (INDEX_NONE) used to mark empty buckets / the end of
// the free list:
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
#define _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, elem_t)
#define _ZP_STATIC_HASHMAP_TEMPLATE_SLOT_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, slot_t)
#define _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPEDEF _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, iter_t)

// ── Node ──────────────────────────────────────────────────────────────────────
//
// The public node type holds just the key/value payload exposed to callers.

typedef struct _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE {
    _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE key;
    _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE val;
} _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE;

// Public typedef for the index type so callers can store/declare indices without
// spelling out the internal macro.
typedef _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPEDEF;

// ── Slot ───────────────────────────────────────────────────────────────────────
//
// All per-index state is merged into a single slot array. Because the bucket
// count is always equal to the capacity, slot i simultaneously serves as:
//   _node     : key/value payload of node i
//   _next     : index of the next node in the chain, or the next free slot
//   _bucket   : index of the first node in bucket i, INDEX_NONE = empty
//   _present  : whether node i currently holds a live entry

typedef struct _ZP_STATIC_HASHMAP_TEMPLATE_SLOT_TYPE {
    _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE _node;
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _next;
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _bucket;
    bool _present;
} _ZP_STATIC_HASHMAP_TEMPLATE_SLOT_TYPE;

// ── Map type ──────────────────────────────────────────────────────────────────
//
// _slots[i]   : merged per-index state (see slot definition above).
// _free_head  : index of the first free slot (free list via _slots[i]._next).

typedef struct _ZP_STATIC_HASHMAP_TEMPLATE_TYPE {
    _ZP_STATIC_HASHMAP_TEMPLATE_SLOT_TYPE _slots[_ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY];
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _free_head;
    size_t _size;  // number of live entries
} _ZP_STATIC_HASHMAP_TEMPLATE_TYPE;

// ── new ───────────────────────────────────────────────────────────────────────

static inline _ZP_STATIC_HASHMAP_TEMPLATE_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, new)(void) {
    _ZP_STATIC_HASHMAP_TEMPLATE_TYPE map;
    for (size_t b = 0; b < _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT; b++) {
        map._slots[b]._bucket = _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;
    }
    for (_ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE i = 0; i < _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY; i++) {
        map._slots[i]._present = false;
    }
    for (_ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE i = 0; i + 1 < _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY; i++) {
        map._slots[i]._next = (_ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE)(i + 1);
    }
    map._slots[_ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY - 1]._next =
        _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;  // end of free list
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
    map->_free_head = map->_slots[idx]._next;
    map->_slots[idx]._present = true;
    return idx;
}

static inline void _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, pool_free)(_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                                                        _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx) {
    map->_slots[idx]._next = map->_free_head;
    map->_free_head = idx;
    map->_slots[idx]._present = false;
}

// ── get_iter ──────────────────────────────────────────────────────────────────
// Returns an iterator to the node for key, or an invalid iterator if not found.
// Note: iterators are stable across insertions and removals of other keys, but become invalid if the same key is
// removed.

static inline _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                                                            get_iter)(const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                                                      const _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE *key) {
    size_t b = _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN(key) % _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT;
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx = map->_slots[b]._bucket;
    while (idx != _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE) {
        const _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_slots[idx]._node;
        if (_ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN(&n->key, key)) {
            return idx;
        }
        idx = map->_slots[idx]._next;
    }
    return _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;
}

// ── get ───────────────────────────────────────────────────────────────────────
// Returns a pointer to the value for key, or NULL if not found.

static inline _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE *_ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                                                            get)(_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                                                 const _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE *key) {
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx = _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, get_iter)(map, key);
    if (idx != _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE) {
        return &map->_slots[idx]._node.val;
    }
    return NULL;
}

// ── const_get ────────────────────────────────────────────────────────────────
// Returns a const pointer to the value for key, or NULL if not found.

static inline const _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE *_ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, const_get)(
    const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map, const _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE *key) {
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx = _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, get_iter)(map, key);
    if (idx != _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE) {
        return &map->_slots[idx]._node.val;
    }
    return NULL;
}

// ── contains ─────────────────────────────────────────────────────────────────

static inline bool _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                           contains)(const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                     const _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE *key) {
    return _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, get_iter)((_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *)(uintptr_t)map,
                                                               key) != _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;
}

// ── size / is_empty ───────────────────────────────────────────────────────────

static inline size_t _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, size)(const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map) {
    return map->_size;
}

static inline bool _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, is_empty)(const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map) {
    return map->_size == 0;
}

// ── at ──────────────────────────────────────────────────────────────────
// Converts a valid index to a pointer to its node.
// Behaviour is undefined if idx is not a valid iterator.
static inline _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *_ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                                                             at)(_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                                                 _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx) {
    return &map->_slots[idx]._node;
}

// ── const_at ──────────────────────────────────────────────────────────────────
// Converts a valid index to a const pointer to its node.
// Behaviour is undefined if idx is not a valid iterator.
static inline const _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *_ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, const_at)(
    const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map, _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx) {
    return &map->_slots[idx]._node;
}

// ── insert ────────────────────────────────────────────────────────────────────
// Takes ownership of *key and *val via move.
// If key already exists: old value is destroyed, new value is moved in.
// The new key is destroyed (existing key kept).
// Returns the iterator to the inserted/updated node.
// Returns an invalid iterator only when the pool is exhausted and the key is not already
// present.

static inline _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                                                            insert)(_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                                                    _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE *key,
                                                                    _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE *val) {
    size_t b = _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN(key) % _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT;
    // Walk the chain looking for an existing entry with the same key
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx = map->_slots[b]._bucket;
    while (idx != _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE) {
        _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_slots[idx]._node;
        if (_ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN(&n->key, key)) {
            // Update: destroy incoming key, replace value in-place
            _ZP_STATIC_HASHMAP_TEMPLATE_KEY_DESTROY_FN(key);
            _ZP_STATIC_HASHMAP_TEMPLATE_VAL_DESTROY_FN(&n->val);
            _ZP_STATIC_HASHMAP_TEMPLATE_VAL_MOVE_FN(&n->val, val);
            return idx;
        }
        idx = map->_slots[idx]._next;
    }
    // New entry — allocate a pool node
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE new_idx = _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, pool_alloc)(map);
    if (new_idx == _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE) {
        return _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;  // pool exhausted
    }
    _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_slots[new_idx]._node;
    _ZP_STATIC_HASHMAP_TEMPLATE_KEY_MOVE_FN(&n->key, key);
    _ZP_STATIC_HASHMAP_TEMPLATE_VAL_MOVE_FN(&n->val, val);
    // Prepend to bucket chain (O(1))
    map->_slots[new_idx]._next = map->_slots[b]._bucket;
    map->_slots[b]._bucket = new_idx;
    map->_size++;
    return new_idx;
}

// ── Iteration ─────────────────────────────────────────────────────────────────
//
// Pattern:
//   for (map_iter_t i = map_begin(&map); i != map_end(&map); i = map_iter_next(&map, i)) {
//       map_elem_t *n = map_at(&map, i);
//       // use n->key, n->val
//   }

// Returns the index of the next live slot after 'pos', or an invalid iterator.
static inline _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                                                            iter_next)(const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                                                       _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE pos) {
    for (size_t i = pos + 1; i < _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY; i++) {
        if (map->_slots[i]._present) {
            return i;
        }
    }
    return _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;
}

// Returns the iterator of the first live slot, or an end iterator if the map is empty.
static inline _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                                                            begin)(const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map) {
    return map->_slots[0]._present ? 0 : _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, iter_next)(map, 0);
}

// Returns an invalid post-end iterator.
static inline _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                                                            end)(const _ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map) {
    return _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;
}

// ── remove_at ────────────────────────────────────────────────────────────────
// Remove the node at the given iterator (obtained from insert or a prior
// lookup).  Behaviour is undefined if iterator is invalid or has already been
// freed.  If out_val != NULL the value is moved out; otherwise it is destroyed.
// If next_idx != NULL it is set to the iterator of the next node.

static inline void _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME,
                           remove_at)(_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map, _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx,
                                      _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *out_val,
                                      _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE *next_idx) {
    _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_slots[idx]._node;
    // Re-derive the bucket from the node's own key so the caller does not need
    // to supply it separately.
    size_t b = _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN(&n->key) % _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT;
    // Walk the chain to find the predecessor and unlink idx.
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE prev = _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE cur = map->_slots[b]._bucket;
    while (cur != idx) {
        prev = cur;
        cur = map->_slots[cur]._next;
    }
    if (prev == _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE) {
        map->_slots[b]._bucket = map->_slots[idx]._next;
    } else {
        map->_slots[prev]._next = map->_slots[idx]._next;
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
        *next_idx = _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, iter_next)(map, idx);
    }
}
// ── remove ────────────────────────────────────────────────────────────────────
// If out_val != NULL the value is moved out; otherwise it is destroyed.
// Returns true if the key was found.

static inline bool _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, remove)(_ZP_STATIC_HASHMAP_TEMPLATE_TYPE *map,
                                                                     const _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE *key,
                                                                     _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE *out_val) {
    _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx = _ZP_CAT(_ZP_STATIC_HASHMAP_TEMPLATE_NAME, get_iter)(map, key);
    if (idx == _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE) {
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
        _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE idx = map->_slots[b]._bucket;
        while (idx != _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE) {
            _ZP_STATIC_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_slots[idx]._node;
            _ZP_STATIC_HASHMAP_TEMPLATE_KEY_DESTROY_FN(&n->key);
            _ZP_STATIC_HASHMAP_TEMPLATE_VAL_DESTROY_FN(&n->val);
            idx = map->_slots[idx]._next;
        }
        map->_slots[b]._bucket = _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;
    }
    // Mark all slots as free
    for (_ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE i = 0; i < _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY; i++) {
        map->_slots[i]._present = false;
    }
    // Rebuild the free list
    for (_ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE i = 0; i + 1 < _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY; i++) {
        map->_slots[i]._next = (_ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE)(i + 1);
    }
    map->_slots[_ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY - 1]._next =
        _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE;  // end of free list
    map->_free_head = 0;
    map->_size = 0;
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
#undef _ZP_STATIC_HASHMAP_TEMPLATE_SLOT_TYPE
#undef _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPE
#undef _ZP_STATIC_HASHMAP_TEMPLATE_INDEX_NONE
#undef _ZP_STATIC_HASHMAP_TEMPLATE_ITER_TYPEDEF
