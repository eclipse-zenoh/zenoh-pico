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

// Internal template engine for heap-allocated hashmap/hashset with separate
// chaining using a contiguous node pool.
//
// This is the shared implementation used by hashmap_template.h and
// hashset_template.h.  Prefer including those wrappers directly; only include
// this file if you need fine-grained control over the generation process.
//
// This is the dynamically-growable counterpart of
// static_hashmap_template_internal.h.  It exposes the same interface but stores
// its nodes and buckets in heap memory that grows on demand instead of in a
// fixed-size embedded array.
//
// Design highlights:
//   * Nodes are kept in a single flat pool (one contiguous allocation) instead
//     of being individually heap-allocated. This keeps the entries packed
//     together for good cache locality while iterating or walking chains.
//   * Each bucket head is merged into the same pool slot array (slot i doubles
//     as the head of bucket i), so the whole map lives in one allocation.
//   * Each bucket is the head of an intrusive singly-linked list whose links
//     are pool indices (not pointers).
//   * Iterators are pool indices. They remain STABLE across rehashing and pool
//     growth: when the map grows, an existing entry keeps the very same index,
//     so iterators obtained before a growth/insertion are still valid
//     afterwards (an iterator only becomes invalid when its own entry is
//     removed).
//   * The index type is configurable (uint8_t / uint16_t / uint32_t, default
//     uint32_t), trading addressable capacity for per-entry memory overhead.
//
// This file expects the following macros to be defined by the caller (or by
// the hashmap_template.h / hashset_template.h wrappers) before inclusion:
//
// Required:
//   _ZP_HASHMAP_TEMPLATE_KEY_TYPE
//       type of the key
//   _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN(key_ptr) -> size_t
//       hash function for the key
//   _ZP_HASHMAP_TEMPLATE_IS_SET
//       Define to 1 (or any non-empty token) to generate a hashset instead of
//       a hashmap.  When generating a hashmap, also define _VAL_TYPE below.
//
// Optional:
//   _ZP_HASHMAP_TEMPLATE_VAL_TYPE
//       type of the value (required when _IS_SET is not defined)
//   _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN(key_a_ptr, key_b_ptr) -> bool
//       equality function for keys (default: *key_a_ptr == *key_b_ptr)
//   _ZP_HASHMAP_TEMPLATE_NAME
//       base name for all generated symbols
//       (default: auto-derived from key/val types)
//   _ZP_HASHMAP_TEMPLATE_INDEX_TYPE
//       unsigned integer type used for pool indices / iterators. The largest
//       value of this type is reserved as a sentinel, so the map can hold at
//       most (max_value_of_type - 0) entries. Must be one of uint8_t,
//       uint16_t, uint32_t or another unsigned integer type.
//       (default: uint32_t)
//   _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY
//       number of entries (and buckets) reserved on the first insertion
//       (default: 16)
//   _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN(key_ptr)
//       destroy a key (default: no-op)
//   _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN(val_ptr)
//       destroy a value (default: no-op)
//   _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN(dst_ptr, src_ptr)
//       move a key (default: copy without destroying src)
//   _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(dst_ptr, src_ptr)
//       move a value (default: copy without destroying src)
//   _ZP_HASHMAP_TEMPLATE_ALLOC_FN(bytes) -> void *
//       allocate memory (default: malloc)
//   _ZP_HASHMAP_TEMPLATE_FREE_FN(ptr)
//       free memory (default: free)
//   _ZP_HASHMAP_TEMPLATE_REALLOC_FN(ptr, bytes) -> void *
//       reallocate memory (unused by default). When provided AND the key and
//       value are both trivially movable (no custom MOVE_FN defined for
//       either), growth uses this to resize the pool in place instead of
//       allocating a fresh buffer and moving every entry, which is faster.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/collections/cat.h"

// ── Optional macros with defaults ────────────────────────────────────────────

#ifndef _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN
#define _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN(key_a_ptr, key_b_ptr) (*(key_a_ptr) == *(key_b_ptr))
#endif

#ifndef _ZP_HASHMAP_TEMPLATE_INDEX_TYPE
#define _ZP_HASHMAP_TEMPLATE_INDEX_TYPE uint32_t
#endif

#ifndef _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY
#define _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY 16
#endif

#ifndef _ZP_HASHMAP_TEMPLATE_NAME
#ifdef _ZP_HASHMAP_TEMPLATE_IS_SET
#define _ZP_HASHMAP_TEMPLATE_NAME _ZP_CAT(_ZP_HASHMAP_TEMPLATE_KEY_TYPE, hset)
#else
#define _ZP_HASHMAP_TEMPLATE_NAME _ZP_CAT(_ZP_HASHMAP_TEMPLATE_KEY_TYPE, _ZP_CAT(_ZP_HASHMAP_TEMPLATE_VAL_TYPE, hmap))
#endif
#endif

#ifndef _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN
#define _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN(x) (void)(x)
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN
#define _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN(x) (void)(x)
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN
#define _ZP_HASHMAP_TEMPLATE_KEY_TRIVIALLY_MOVABLE
#define _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN(dst, src) *(dst) = *(src);
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_IS_SET
#ifndef _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN
#define _ZP_HASHMAP_TEMPLATE_VAL_TRIVIALLY_MOVABLE
#define _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(dst, src) *(dst) = *(src);
#endif
#endif  // !_ZP_HASHMAP_TEMPLATE_IS_SET

#ifndef _ZP_HASHMAP_TEMPLATE_ALLOC_FN
#define _ZP_HASHMAP_TEMPLATE_ALLOC_FN(bytes) malloc(bytes)
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_FREE_FN
#define _ZP_HASHMAP_TEMPLATE_FREE_FN(ptr) free(ptr)
#endif
// By default growth allocates a fresh pool and moves entries individually.
// A custom reallocation function can be provided to resize in place when both
// key and value are trivially movable.
// #define _ZP_HASHMAP_TEMPLATE_REALLOC_FN(ptr, bytes) realloc(ptr, bytes)

// A slot is trivially relocatable (bitwise copyable) only when both its key and
// value are trivially movable. In hashset mode there is no value, so the slot is
// trivially relocatable whenever the key is.
#ifdef _ZP_HASHMAP_TEMPLATE_IS_SET
#if defined(_ZP_HASHMAP_TEMPLATE_KEY_TRIVIALLY_MOVABLE)
#define _ZP_HASHMAP_TEMPLATE_SLOT_TRIVIALLY_MOVABLE
#endif
#else
#if defined(_ZP_HASHMAP_TEMPLATE_KEY_TRIVIALLY_MOVABLE) && defined(_ZP_HASHMAP_TEMPLATE_VAL_TRIVIALLY_MOVABLE)
#define _ZP_HASHMAP_TEMPLATE_SLOT_TRIVIALLY_MOVABLE
#endif
#endif

// ── Index / iterator type ─────────────────────────────────────────────────────
//
// The all-ones value of the (unsigned) index type is reserved as a sentinel
// (INDEX_NONE) used to mark empty buckets and the end of chains / the free list.
// Valid indices therefore range over [0, INDEX_NONE), which makes INDEX_NONE
// also the maximum number of entries the map can ever hold.

#define _ZP_HASHMAP_TEMPLATE_ITER_TYPE _ZP_HASHMAP_TEMPLATE_INDEX_TYPE
#define _ZP_HASHMAP_TEMPLATE_INDEX_NONE ((_ZP_HASHMAP_TEMPLATE_INDEX_TYPE)(~(_ZP_HASHMAP_TEMPLATE_INDEX_TYPE)0))
#define _ZP_HASHMAP_TEMPLATE_MAX_CAPACITY ((size_t)_ZP_HASHMAP_TEMPLATE_INDEX_NONE)

// ── Internal name helpers ─────────────────────────────────────────────────────

#define _ZP_HASHMAP_TEMPLATE_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, t)
#define _ZP_HASHMAP_TEMPLATE_NODE_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, elem_t)
#define _ZP_HASHMAP_TEMPLATE_SLOT_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, slot_t)
#define _ZP_HASHMAP_TEMPLATE_ITER_TYPEDEF _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, iter_t)

// ── Node ──────────────────────────────────────────────────────────────────────
//
// In hashmap mode the public node type holds the key/value payload exposed to
// callers.  In hashset mode the node type is simply the key type itself.
//
// _ZP_HASHMAP_TEMPLATE_NODE_KEY(node_ptr) yields a pointer to the key stored in
// a node regardless of the mode, so the shared code below can be written
// uniformly.

#ifdef _ZP_HASHMAP_TEMPLATE_IS_SET
typedef _ZP_HASHMAP_TEMPLATE_KEY_TYPE _ZP_HASHMAP_TEMPLATE_NODE_TYPE;
#define _ZP_HASHMAP_TEMPLATE_NODE_KEY(node_ptr) (node_ptr)
#else
typedef struct _ZP_HASHMAP_TEMPLATE_NODE_TYPE {
    _ZP_HASHMAP_TEMPLATE_KEY_TYPE key;
    _ZP_HASHMAP_TEMPLATE_VAL_TYPE val;
} _ZP_HASHMAP_TEMPLATE_NODE_TYPE;
#define _ZP_HASHMAP_TEMPLATE_NODE_KEY(node_ptr) (&(node_ptr)->key)
#endif

// Public typedefs for the key and value types. Applying `const` to these
// typedefs (e.g. `const <name>_key_t *`) qualifies the whole aliased type, which
// is correct even when the key/value type is itself a pointer. Spelling the
// `const` out as `const _ZP_HASHMAP_TEMPLATE_KEY_TYPE *` would instead expand to
// e.g. `const char **` for a `char *` key, qualifying the wrong pointer level and
// discarding qualifiers at call sites.
typedef _ZP_HASHMAP_TEMPLATE_KEY_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, key_t);
#ifndef _ZP_HASHMAP_TEMPLATE_IS_SET
typedef _ZP_HASHMAP_TEMPLATE_VAL_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, val_t);
#endif

// Public typedef for the index/iterator type so callers can store indices
// without spelling out the internal macro.
typedef _ZP_HASHMAP_TEMPLATE_ITER_TYPE _ZP_HASHMAP_TEMPLATE_ITER_TYPEDEF;

// ── Slot ───────────────────────────────────────────────────────────────────────
//
// One pool slot per node. Because the bucket count is always equal to the
// capacity, slot i serves a dual purpose:
//   _node      : key/value payload of node i
//   _next      : index of the next node in the bucket chain, or the next free slot if _bucket == INDEX_NONE
//   _bucket    : index of the first node in bucket i, INDEX_NONE = empty
//   _next_live : an index of the live node following this one in the iteration order, or INDEX_NONE if this is the
//   last live node.
//   _prev_live : an index of the live node preceding this one in the iteration order, or INDEX_NONE if
//   this is the first live node.
//   Live entries are threaded through these links starting at map->_live_head.

typedef struct _ZP_HASHMAP_TEMPLATE_SLOT_TYPE {
    _ZP_HASHMAP_TEMPLATE_NODE_TYPE _node;
    _ZP_HASHMAP_TEMPLATE_INDEX_TYPE _next;
    _ZP_HASHMAP_TEMPLATE_INDEX_TYPE _bucket;
    _ZP_HASHMAP_TEMPLATE_INDEX_TYPE _next_live;
    _ZP_HASHMAP_TEMPLATE_INDEX_TYPE _prev_live;
} _ZP_HASHMAP_TEMPLATE_SLOT_TYPE;

#define _ZP_HASHMAP_TEMPLATE_MAX_ALLOC_SIZE (SIZE_MAX / sizeof(_ZP_HASHMAP_TEMPLATE_SLOT_TYPE))

#define _ZP_HASHMAP_NODE_IS_LIVE(map_ptr, node_iter)                               \
    ((map_ptr)->_slots[node_iter]._prev_live != _ZP_HASHMAP_TEMPLATE_INDEX_NONE || \
     (map_ptr)->_live_head == (size_t)(node_iter))

// ── Map type ──────────────────────────────────────────────────────────────────
//
// _slots       : heap-allocated node pool (_capacity entries). Slot i also
//                holds the head of bucket i (merged storage). The number of
//                buckets is always equal to _capacity.
// _capacity    : number of slots in the pool (and number of buckets).
// _free_head   : index of the first free slot (free list via _slots[i]._next).
// _size        : number of live entries.

typedef struct _ZP_HASHMAP_TEMPLATE_TYPE {
    _ZP_HASHMAP_TEMPLATE_SLOT_TYPE *_slots;
    size_t _capacity;
    size_t _size;
    _ZP_HASHMAP_TEMPLATE_INDEX_TYPE _free_head;
    _ZP_HASHMAP_TEMPLATE_INDEX_TYPE _live_head;
} _ZP_HASHMAP_TEMPLATE_TYPE;

// ── init ──────────────────────────────────────────────────────────────────────
// Initializes a new, empty map. No allocation is performed until the first insert.

static inline void _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, init)(_ZP_HASHMAP_TEMPLATE_TYPE *map) {
    map->_slots = NULL;
    map->_capacity = 0;
    map->_free_head = _ZP_HASHMAP_TEMPLATE_INDEX_NONE;
    map->_live_head = _ZP_HASHMAP_TEMPLATE_INDEX_NONE;
    map->_size = 0;
}

// ── new ───────────────────────────────────────────────────────────────────────
// Creates a new, empty map. No allocation is performed until the first insert.

static inline _ZP_HASHMAP_TEMPLATE_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, new)(void) {
    _ZP_HASHMAP_TEMPLATE_TYPE map;
    _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, init)(&map);
    return map;
}

// ── Internal: grow the pool to new_capacity ───────────────────────────────────
//
// Grows the node pool so it can hold new_capacity entries, moving every live
// entry to the SAME index (so iterators stay valid), rebuilding the free list
// over the remaining slots and re-hashing all entries into the new (merged)
// bucket heads.
//
// Three relocation strategies, fastest first:
//   1. REALLOC_FN defined AND slot trivially movable: resize the pool in place
//      via realloc (no separate move of payloads).
//   2. Slot trivially movable: allocate a fresh pool and bitwise-copy the live
//      payloads with memcpy.
//   3. Otherwise: allocate a fresh pool and move each live entry individually
//      through the configured MOVE_FN macros.
//
// Returns true on success; on failure the map is left unchanged.

static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, grow)(_ZP_HASHMAP_TEMPLATE_TYPE *map, size_t new_capacity) {
    if (new_capacity <= map->_capacity) {
        return true;
    }
    if (new_capacity > _ZP_HASHMAP_TEMPLATE_MAX_CAPACITY || new_capacity > _ZP_HASHMAP_TEMPLATE_MAX_ALLOC_SIZE) {
        return false;
    }

    _ZP_HASHMAP_TEMPLATE_SLOT_TYPE *new_slots;
#if defined(_ZP_HASHMAP_TEMPLATE_REALLOC_FN) && defined(_ZP_HASHMAP_TEMPLATE_SLOT_TRIVIALLY_MOVABLE)
    // Strategy 1: resize the existing pool in place. Live payloads keep their
    // index and content; only the bucket heads / free list are rebuilt below.
    new_slots = (_ZP_HASHMAP_TEMPLATE_SLOT_TYPE *)_ZP_HASHMAP_TEMPLATE_REALLOC_FN(
        map->_slots, new_capacity * sizeof(_ZP_HASHMAP_TEMPLATE_SLOT_TYPE));
    if (new_slots == NULL) {
        return false;
    }
    map->_slots = NULL;  // ownership transferred to new_slots; avoid double free below
#else
    new_slots = (_ZP_HASHMAP_TEMPLATE_SLOT_TYPE *)_ZP_HASHMAP_TEMPLATE_ALLOC_FN(new_capacity *
                                                                                sizeof(_ZP_HASHMAP_TEMPLATE_SLOT_TYPE));
    if (new_slots == NULL) {
        return false;
    }
#if defined(_ZP_HASHMAP_TEMPLATE_SLOT_TRIVIALLY_MOVABLE)
    // Strategy 2: bitwise-copy the existing slots._node and the old
    // free-list _next and _next_live, _prev_live links are all carried over verbatim, so the existing free
    // lists rooted at map->_free_head and map->_live_head stay valid (see free-list note below).
    if (map->_capacity > 0) {
        // SAFETY: new_slots is guaranteed to have higher capacity then map->_capacity by construction.
        // Flawfinder: ignore [CWE-120]
        memcpy(new_slots, map->_slots, map->_capacity * sizeof(_ZP_HASHMAP_TEMPLATE_SLOT_TYPE));
    }
#else
    // Strategy 3: fresh buffer. Move each live entry to the same index through
    // MOVE_FN, and keep live links
    for (size_t i = 0; i < map->_capacity; i++) {
        new_slots[i]._next_live = map->_slots[i]._next_live;
        new_slots[i]._prev_live = map->_slots[i]._prev_live;
        if (_ZP_HASHMAP_NODE_IS_LIVE(map, i)) {
            _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN(_ZP_HASHMAP_TEMPLATE_NODE_KEY(&new_slots[i]._node),
                                             _ZP_HASHMAP_TEMPLATE_NODE_KEY(&map->_slots[i]._node));
#ifndef _ZP_HASHMAP_TEMPLATE_IS_SET
            _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(&new_slots[i]._node.val, &map->_slots[i]._node.val);
#endif
        } else {
            // relink only empty slots, since occupied ones will likely have different links due to rehashing
            new_slots[i]._next = map->_slots[i]._next;
        }
    }
#endif
#endif

    // Newly added slots start empty.
    for (size_t i = map->_capacity; i < new_capacity; i++) {
        new_slots[i]._next_live = _ZP_HASHMAP_TEMPLATE_INDEX_NONE;
        new_slots[i]._prev_live = _ZP_HASHMAP_TEMPLATE_INDEX_NONE;
    }
    // Every bucket head starts empty (rebuilt by the re-hash below).
    for (size_t b = 0; b < new_capacity; b++) {
        new_slots[b]._bucket = _ZP_HASHMAP_TEMPLATE_INDEX_NONE;
    }

    // Rebuild the free list.
    // Link newly added slots
    _ZP_HASHMAP_TEMPLATE_INDEX_TYPE free_head = map->_free_head;
    for (size_t i = new_capacity; i-- > map->_capacity;) {
        new_slots[i]._next = free_head;
        free_head = (_ZP_HASHMAP_TEMPLATE_INDEX_TYPE)i;
    }

    _ZP_HASHMAP_TEMPLATE_FREE_FN(map->_slots);
    map->_slots = new_slots;
    map->_free_head = free_head;
    // Do not update capacity yet, since the re-hash below needs to know the old capacity to iterate over all live
    // entries. Re-hash every live entry into the new bucket heads (indices are preserved).
    for (size_t i = 0; i < map->_capacity; i++) {
        if (_ZP_HASHMAP_NODE_IS_LIVE(map, i)) {
            size_t b =
                _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN(_ZP_HASHMAP_TEMPLATE_NODE_KEY(&new_slots[i]._node)) % new_capacity;
            new_slots[i]._next = new_slots[b]._bucket;
            new_slots[b]._bucket = (_ZP_HASHMAP_TEMPLATE_INDEX_TYPE)i;
        }
    }
    map->_capacity = new_capacity;

    return true;
}

// ── reserve ───────────────────────────────────────────────────────────────────
// Ensures the map can hold at least min_capacity entries without re-growing.
// Returns true on success, false if the allocation failed.

static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, reserve)(_ZP_HASHMAP_TEMPLATE_TYPE *map, size_t min_capacity) {
    return _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, grow)(map, min_capacity);
}

// ── Internal: allocate / free pool node ──────────────────────────────────────

static inline _ZP_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME,
                                                     pool_alloc)(_ZP_HASHMAP_TEMPLATE_TYPE *map) {
    _ZP_HASHMAP_TEMPLATE_ITER_TYPE idx = map->_free_head;
    if (idx == _ZP_HASHMAP_TEMPLATE_INDEX_NONE) {
        return _ZP_HASHMAP_TEMPLATE_INDEX_NONE;  // pool full
    }
    map->_free_head = map->_slots[idx]._next;
    map->_slots[idx]._next_live = map->_live_head;
    if (map->_live_head != _ZP_HASHMAP_TEMPLATE_INDEX_NONE) {
        map->_slots[map->_live_head]._prev_live = idx;
    }
    map->_live_head = idx;
    return idx;
}

static inline void _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, pool_free)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                                 _ZP_HASHMAP_TEMPLATE_ITER_TYPE idx) {
    map->_slots[idx]._next = map->_free_head;
    map->_free_head = idx;
    if (map->_slots[idx]._prev_live != _ZP_HASHMAP_TEMPLATE_INDEX_NONE) {
        map->_slots[map->_slots[idx]._prev_live]._next_live = map->_slots[idx]._next_live;
    } else {
        map->_live_head = map->_slots[idx]._next_live;
    }
    if (map->_slots[idx]._next_live != _ZP_HASHMAP_TEMPLATE_INDEX_NONE) {
        map->_slots[map->_slots[idx]._next_live]._prev_live = map->_slots[idx]._prev_live;
    }
    map->_slots[idx]._next_live = _ZP_HASHMAP_TEMPLATE_INDEX_NONE;
    map->_slots[idx]._prev_live = _ZP_HASHMAP_TEMPLATE_INDEX_NONE;
}

// ── get_iter ──────────────────────────────────────────────────────────────────
// Returns an iterator to the node for key, or an invalid iterator if not found.
// Note: iterators are stable across insertions, growth and removals of other
// keys, but become invalid if the same key is removed.

static inline _ZP_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME,
                                                     get_iter)(const _ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                               const _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, key_t) * key) {
    if (map->_capacity == 0) {
        return _ZP_HASHMAP_TEMPLATE_INDEX_NONE;
    }
    size_t b = _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN(key) % map->_capacity;
    _ZP_HASHMAP_TEMPLATE_ITER_TYPE idx = map->_slots[b]._bucket;
    while (idx != _ZP_HASHMAP_TEMPLATE_INDEX_NONE) {
        const _ZP_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_slots[idx]._node;
        if (_ZP_HASHMAP_TEMPLATE_KEY_EQ_FN(_ZP_HASHMAP_TEMPLATE_NODE_KEY(n), key)) {
            return idx;
        }
        idx = map->_slots[idx]._next;
    }
    return _ZP_HASHMAP_TEMPLATE_INDEX_NONE;
}

#ifndef _ZP_HASHMAP_TEMPLATE_IS_SET
// ── get ───────────────────────────────────────────────────────────────────────
// Returns a pointer to the value for key, or NULL if not found.

static inline _ZP_HASHMAP_TEMPLATE_VAL_TYPE *_ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME,
                                                     get)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                          const _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, key_t) * key) {
    _ZP_HASHMAP_TEMPLATE_ITER_TYPE idx = _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, get_iter)(map, key);
    if (idx != _ZP_HASHMAP_TEMPLATE_INDEX_NONE) {
        return &map->_slots[idx]._node.val;
    }
    return NULL;
}

// ── const_get ────────────────────────────────────────────────────────────────
// Returns a const pointer to the value for key, or NULL if not found.

static inline const _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, val_t) *
    _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, const_get)(const _ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                  const _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, key_t) * key) {
    _ZP_HASHMAP_TEMPLATE_ITER_TYPE idx = _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, get_iter)(map, key);
    if (idx != _ZP_HASHMAP_TEMPLATE_INDEX_NONE) {
        return &map->_slots[idx]._node.val;
    }
    return NULL;
}
#endif  // !_ZP_HASHMAP_TEMPLATE_IS_SET

// ── contains ─────────────────────────────────────────────────────────────────

static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, contains)(const _ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                                const _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, key_t) * key) {
    return _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, get_iter)(map, key) != _ZP_HASHMAP_TEMPLATE_INDEX_NONE;
}

// ── size / capacity / is_empty ────────────────────────────────────────────────

static inline size_t _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, size)(const _ZP_HASHMAP_TEMPLATE_TYPE *map) {
    return map->_size;
}

static inline size_t _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, capacity)(const _ZP_HASHMAP_TEMPLATE_TYPE *map) {
    return map->_capacity;
}

static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, is_empty)(const _ZP_HASHMAP_TEMPLATE_TYPE *map) {
    return map->_size == 0;
}

// ── at ──────────────────────────────────────────────────────────────────
// Converts a valid index to a pointer to its node.
// Behaviour is undefined if idx is not a valid iterator.
static inline _ZP_HASHMAP_TEMPLATE_NODE_TYPE *_ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME,
                                                      at)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                          _ZP_HASHMAP_TEMPLATE_ITER_TYPE idx) {
    return &map->_slots[idx]._node;
}

// ── const_at ──────────────────────────────────────────────────────────────────
// Converts a valid index to a const pointer to its node.
// Behaviour is undefined if idx is not a valid iterator.
static inline const _ZP_HASHMAP_TEMPLATE_NODE_TYPE *_ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME,
                                                            const_at)(const _ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                                      _ZP_HASHMAP_TEMPLATE_ITER_TYPE idx) {
    return &map->_slots[idx]._node;
}

#ifndef _ZP_HASHMAP_TEMPLATE_IS_SET
// ── insert ────────────────────────────────────────────────────────────────────
// Takes ownership of *key (and, in hashmap mode, *val) via move.
// If key already exists: the old value is destroyed and the new
// value is moved in; the incoming key is destroyed (the existing key is kept).
// Grows the pool (doubling its capacity) when full.
// Returns the iterator to the inserted/updated node.
// Returns an invalid iterator only when an allocation fails (and the key is not
// already present) or the maximum addressable capacity has been reached.
// *val can be NULL, in which case the entry will be created with
// an uninitialized value that can be initialized manually via _at at the
// returned iterator.

static inline _ZP_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME,
                                                     insert)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                             _ZP_HASHMAP_TEMPLATE_KEY_TYPE *key,
                                                             _ZP_HASHMAP_TEMPLATE_VAL_TYPE *val)
#else
// ── insert ────────────────────────────────────────────────────────────────────
// Takes ownership of *key (and, in hashmap mode, *val) via move.
// If key already exists: the incoming key is destroyed (the existing key is kept).
// Grows the pool (doubling its capacity) when full.
// Returns the iterator to the inserted/updated node.
// Returns an invalid iterator only when an allocation fails (and the key is not
// already present) or the maximum addressable capacity has been reached.
// In hashmap mode *val can be NULL, in which case the entry will be created with
// an uninitialized value that can be initialized manually via _at at the
// returned iterator.

static inline _ZP_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME,
                                                     insert)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                             _ZP_HASHMAP_TEMPLATE_KEY_TYPE *key)
#endif
{
    // Ensure the map has a backing store.
    if (map->_capacity == 0) {
        if (!_ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, grow)(map, _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY)) {
            return _ZP_HASHMAP_TEMPLATE_INDEX_NONE;
        }
    }

    size_t b = _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN(key) % map->_capacity;
    // Walk the chain looking for an existing entry with the same key.
    _ZP_HASHMAP_TEMPLATE_ITER_TYPE idx = map->_slots[b]._bucket;
    while (idx != _ZP_HASHMAP_TEMPLATE_INDEX_NONE) {
        _ZP_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_slots[idx]._node;
        if (_ZP_HASHMAP_TEMPLATE_KEY_EQ_FN(_ZP_HASHMAP_TEMPLATE_NODE_KEY(n), key)) {
            // Update: destroy incoming key, replace value in-place.
            _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN(key);
#ifndef _ZP_HASHMAP_TEMPLATE_IS_SET
            _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN(&n->val);
            if (val != NULL) {
                _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(&n->val, val);
            }
#endif
            return idx;
        }
        idx = map->_slots[idx]._next;
    }

    // New entry — grow the pool if it is full.
    if (map->_size == map->_capacity) {
        size_t max_capacity = _ZP_HASHMAP_TEMPLATE_MAX_ALLOC_SIZE;
        max_capacity =
            _ZP_HASHMAP_TEMPLATE_MAX_CAPACITY < max_capacity ? _ZP_HASHMAP_TEMPLATE_MAX_CAPACITY : max_capacity;
        if (map->_capacity == max_capacity) {
            return _ZP_HASHMAP_TEMPLATE_INDEX_NONE;  // capacity exhausted
        }
        size_t new_capacity = map->_capacity > max_capacity / 2 ? max_capacity : map->_capacity * 2;

        if (!_ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, grow)(map, new_capacity)) {
            return _ZP_HASHMAP_TEMPLATE_INDEX_NONE;  // allocation failed or capacity exhausted
        }
        // Bucket index may have changed after re-hashing.
        b = _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN(key) % map->_capacity;
    }

    _ZP_HASHMAP_TEMPLATE_ITER_TYPE new_idx = _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, pool_alloc)(map);
    _ZP_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_slots[new_idx]._node;
    _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN(_ZP_HASHMAP_TEMPLATE_NODE_KEY(n), key);
#ifndef _ZP_HASHMAP_TEMPLATE_IS_SET
    if (val != NULL) {
        _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(&n->val, val);
    }
#endif
    // Prepend to bucket chain (O(1)).
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
static inline _ZP_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME,
                                                     iter_next)(const _ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                                _ZP_HASHMAP_TEMPLATE_ITER_TYPE pos) {
    return pos == _ZP_HASHMAP_TEMPLATE_INDEX_NONE ? pos : map->_slots[pos]._next_live;
}

// Returns the iterator of the first live slot, or an end iterator if the map is empty.
static inline _ZP_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME,
                                                     begin)(const _ZP_HASHMAP_TEMPLATE_TYPE *map) {
    return map->_live_head;
}

// Returns an invalid post-end iterator.
static inline _ZP_HASHMAP_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME,
                                                     end)(const _ZP_HASHMAP_TEMPLATE_TYPE *map) {
    (void)map;
    return _ZP_HASHMAP_TEMPLATE_INDEX_NONE;
}

// ── remove_at ────────────────────────────────────────────────────────────────
// Remove the node at the given iterator (obtained from insert or a prior
// lookup).  Behaviour is undefined if iterator is invalid or has already been
// freed.  If out_node != NULL the node is moved out; otherwise it is destroyed.
// If next_idx != NULL it is set to the iterator of the next node.

static inline void _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, remove_at)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                                 _ZP_HASHMAP_TEMPLATE_ITER_TYPE idx,
                                                                 _ZP_HASHMAP_TEMPLATE_NODE_TYPE *out_node,
                                                                 _ZP_HASHMAP_TEMPLATE_ITER_TYPE *next_idx) {
    _ZP_HASHMAP_TEMPLATE_NODE_TYPE *n = &map->_slots[idx]._node;
    // Re-derive the bucket from the node's own key so the caller does not need
    // to supply it separately.
    size_t b = _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN(_ZP_HASHMAP_TEMPLATE_NODE_KEY(n)) % map->_capacity;
    // Walk the chain to find the predecessor and unlink idx.
    _ZP_HASHMAP_TEMPLATE_ITER_TYPE prev = _ZP_HASHMAP_TEMPLATE_INDEX_NONE;
    _ZP_HASHMAP_TEMPLATE_ITER_TYPE cur = map->_slots[b]._bucket;
    while (cur != idx) {
        prev = cur;
        cur = map->_slots[cur]._next;
    }
    if (prev == _ZP_HASHMAP_TEMPLATE_INDEX_NONE) {
        map->_slots[b]._bucket = map->_slots[idx]._next;
    } else {
        map->_slots[prev]._next = map->_slots[idx]._next;
    }
    if (out_node != NULL) {
        _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN(_ZP_HASHMAP_TEMPLATE_NODE_KEY(out_node), _ZP_HASHMAP_TEMPLATE_NODE_KEY(n));
#ifndef _ZP_HASHMAP_TEMPLATE_IS_SET
        _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(&out_node->val, &n->val);
#endif
    } else {
        _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN(_ZP_HASHMAP_TEMPLATE_NODE_KEY(n));
#ifndef _ZP_HASHMAP_TEMPLATE_IS_SET
        _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN(&n->val);
#endif
    }
    if (next_idx != NULL) {
        *next_idx = map->_slots[idx]._next_live;
    }
    _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, pool_free)(map, idx);
    map->_size--;
}

#ifndef _ZP_HASHMAP_TEMPLATE_IS_SET
// ── remove ────────────────────────────────────────────────────────────────────
// Removes and destroys the entry for key. Returns true if the key was found.
// If out_val != NULL the value is moved out; otherwise it is destroyed.
static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, remove)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                              const _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, key_t) * key,
                                                              _ZP_HASHMAP_TEMPLATE_VAL_TYPE *out_val)
#else
// ── remove ────────────────────────────────────────────────────────────────────
// Removes and destroys the entry for key. Returns true if the key was found.
static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, remove)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                              const _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, key_t) * key)
#endif
{
    _ZP_HASHMAP_TEMPLATE_ITER_TYPE idx = _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, get_iter)(map, key);
    if (idx == _ZP_HASHMAP_TEMPLATE_INDEX_NONE) {
        return false;  // not found
    }
#ifndef _ZP_HASHMAP_TEMPLATE_IS_SET
    if (out_val != NULL) {
        _ZP_HASHMAP_TEMPLATE_NODE_TYPE temp;
        _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, remove_at)(map, idx, &temp, NULL);
        _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN(&temp.key);  // key is not returned to caller, so destroy it
        _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(out_val, &temp.val);
    } else {
        _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, remove_at)(map, idx, NULL, NULL);
    }
#else
    _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, remove_at)(map, idx, NULL, NULL);
#endif
    return true;
}

// ── clear ─────────────────────────────────────────────────────────────────────
// Destroys all entries but keeps the allocated backing store for reuse.

static inline void _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, clear)(_ZP_HASHMAP_TEMPLATE_TYPE *map) {
    _ZP_HASHMAP_TEMPLATE_ITER_TYPE begin = _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, begin)(map);
    _ZP_HASHMAP_TEMPLATE_ITER_TYPE end = _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, end)(map);
    while (begin != end) {
        _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, remove_at)(map, begin, NULL, &begin);
    }
}

// ── destroy ─────────────────────────────────────────────────────────────────────
// Destroys all entries, frees the backing store and resets the map to the empty
// state (it can be reused afterwards). Does not free the map struct itself.

static inline void _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, destroy)(_ZP_HASHMAP_TEMPLATE_TYPE *map) {
    _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, clear)(map);
    _ZP_HASHMAP_TEMPLATE_FREE_FN(map->_slots);
    map->_slots = NULL;
    map->_capacity = 0;
    map->_free_head = _ZP_HASHMAP_TEMPLATE_INDEX_NONE;
}

// ── Undef all macros ──────────────────────────────────────────────────────────

#undef _ZP_HASHMAP_TEMPLATE_KEY_TYPE
#undef _ZP_HASHMAP_TEMPLATE_VAL_TYPE
#undef _ZP_HASHMAP_TEMPLATE_NAME
#undef _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY
#undef _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN
#undef _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN
#undef _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN
#undef _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN
#undef _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN
#undef _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN
#undef _ZP_HASHMAP_TEMPLATE_ALLOC_FN
#undef _ZP_HASHMAP_TEMPLATE_FREE_FN
#ifdef _ZP_HASHMAP_TEMPLATE_REALLOC_FN
#undef _ZP_HASHMAP_TEMPLATE_REALLOC_FN
#endif
#ifdef _ZP_HASHMAP_TEMPLATE_KEY_TRIVIALLY_MOVABLE
#undef _ZP_HASHMAP_TEMPLATE_KEY_TRIVIALLY_MOVABLE
#endif
#ifdef _ZP_HASHMAP_TEMPLATE_VAL_TRIVIALLY_MOVABLE
#undef _ZP_HASHMAP_TEMPLATE_VAL_TRIVIALLY_MOVABLE
#endif
#ifdef _ZP_HASHMAP_TEMPLATE_SLOT_TRIVIALLY_MOVABLE
#undef _ZP_HASHMAP_TEMPLATE_SLOT_TRIVIALLY_MOVABLE
#endif
#undef _ZP_HASHMAP_TEMPLATE_INDEX_TYPE
#undef _ZP_HASHMAP_TEMPLATE_TYPE
#undef _ZP_HASHMAP_TEMPLATE_NODE_TYPE
#undef _ZP_HASHMAP_TEMPLATE_NODE_KEY
#undef _ZP_HASHMAP_TEMPLATE_SLOT_TYPE
#undef _ZP_HASHMAP_TEMPLATE_ITER_TYPE
#undef _ZP_HASHMAP_TEMPLATE_INDEX_NONE
#undef _ZP_HASHMAP_TEMPLATE_MAX_CAPACITY
#undef _ZP_HASHMAP_TEMPLATE_MAX_ALLOC_SIZE
#undef _ZP_HASHMAP_TEMPLATE_NODE_IS_LIVE
#undef _ZP_HASHMAP_TEMPLATE_ITER_TYPEDEF
