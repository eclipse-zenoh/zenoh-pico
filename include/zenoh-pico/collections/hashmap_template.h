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

// Heap-allocated open-addressing hashmap with Robin Hood (backward-shift)
// hashing.
//
// Design summary
// ──────────────
// • Open addressing: entries are stored directly in a flat array; no chaining.
// • Robin Hood insertion: when inserting, if the probe sequence length (PSL)
//   of the candidate slot's current occupant is less than the PSL of the
//   element being inserted, they are swapped ("robbing from the rich").  This
//   keeps the maximum PSL low and makes lookups very cache-friendly.
// • Deletion uses backward-shift deletion (no tombstones): after removing a
//   slot the algorithm shifts subsequent entries back until it finds an empty
//   slot or an entry sitting at its ideal bucket (PSL == 0).
// • The table is grown (capacity doubled) when the load factor exceeds
//   _ZP_HASHMAP_TEMPLATE_MAX_LOAD_NUM / _ZP_HASHMAP_TEMPLATE_MAX_LOAD_DEN
//   (default 75 / 100 = 0.75).  Shrinking is not performed automatically.
// • Memory is managed via the user-supplied (or default) alloc/free macros.
//
// User must define the following macros before including this file:
//
// Required:
//   _ZP_HASHMAP_TEMPLATE_KEY_TYPE
//       type of the key
//   _ZP_HASHMAP_TEMPLATE_VAL_TYPE
//       type of the value
//   _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN(key_ptr) -> size_t
//       hash function for the key
//
// Optional:
//   _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN(key_a_ptr, key_b_ptr) -> bool
//       equality function for keys (default: pointer dereference ==)
//   _ZP_HASHMAP_TEMPLATE_NAME
//       base name for all generated symbols
//       (default: key_type##_##val_type##_##rhhmap)
//   _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY
//       initial slot count (must be a power of two; default: 16)
//   _ZP_HASHMAP_TEMPLATE_MAX_LOAD_NUM
//       numerator of the maximum load factor (default: 75)
//   _ZP_HASHMAP_TEMPLATE_MAX_LOAD_DEN
//       denominator of the maximum load factor (default: 100)
//   _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN(key_ptr)
//       destroy a key (default: no-op)
//   _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN(val_ptr)
//       destroy a value (default: no-op)
//   _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN(dst_ptr, src_ptr)
//       move a key (default: *dst = *src)
//   _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(dst_ptr, src_ptr)
//       move a value (default: *dst = *src)
//   _ZP_HASHMAP_TEMPLATE_ALLOC_FN(size) -> void*
//       allocate 'size' bytes (default: malloc)
//   _ZP_HASHMAP_TEMPLATE_FREE_FN(ptr)
//       free a pointer (default: free)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/collections/cat.h"

#ifndef _ZP_HASHMAP_ITER_INVALID
#define _ZP_HASHMAP_ITER_INVALID 0
#endif
// ── Required macros ──────────────────────────────────────────────────────────

#ifndef _ZP_HASHMAP_TEMPLATE_KEY_TYPE
#error "_ZP_HASHMAP_TEMPLATE_KEY_TYPE must be defined before including robin_hood_hashmap_template.h"
#define _ZP_HASHMAP_TEMPLATE_KEY_TYPE int
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_VAL_TYPE
#error "_ZP_HASHMAP_TEMPLATE_VAL_TYPE must be defined before including robin_hood_hashmap_template.h"
#define _ZP_HASHMAP_TEMPLATE_VAL_TYPE int
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN
#error "_ZP_HASHMAP_TEMPLATE_KEY_HASH_FN must be defined before including robin_hood_hashmap_template.h"
#endif

// ── Optional macros with defaults ────────────────────────────────────────────

#ifndef _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN
#define _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN(a, b) (*(a) == *(b))
#endif

#ifndef _ZP_HASHMAP_TEMPLATE_NAME
#define _ZP_HASHMAP_TEMPLATE_NAME _ZP_CAT(_ZP_HASHMAP_TEMPLATE_KEY_TYPE, _ZP_CAT(_ZP_HASHMAP_TEMPLATE_VAL_TYPE, rhhmap))
#endif

#ifndef _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY
#define _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY 16u
#endif

#ifndef _ZP_HASHMAP_TEMPLATE_MAX_LOAD_NUM
#define _ZP_HASHMAP_TEMPLATE_MAX_LOAD_NUM 75u
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_MAX_LOAD_DEN
#define _ZP_HASHMAP_TEMPLATE_MAX_LOAD_DEN 100u
#endif

#ifndef _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN
#define _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN(x) (void)(x)
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN
#define _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN(x) (void)(x)
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN
#define _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN(dst, src) *(dst) = *(src);
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN
#define _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(dst, src) *(dst) = *(src);
#endif

#ifndef _ZP_HASHMAP_TEMPLATE_ALLOC_FN
#define _ZP_HASHMAP_TEMPLATE_ALLOC_FN(sz) malloc(sz)
#endif
#ifndef _ZP_HASHMAP_TEMPLATE_FREE_FN
#define _ZP_HASHMAP_TEMPLATE_FREE_FN(ptr) free(ptr)
#endif

// ── Internal name helpers ─────────────────────────────────────────────────────

#define _ZP_HASHMAP_TEMPLATE_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, t)
#define _ZP_HASHMAP_TEMPLATE_NODE_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, node_t)
#define _ZP_HASHMAP_TEMPLATE_SLOT_TYPE _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, slot_t)

// ── Node ─────────────────────────────────────────────────────────────────────
// Holds the user-visible key and value.  Valid only when the enclosing slot is
// occupied (i.e. _info != 0).

typedef struct _ZP_HASHMAP_TEMPLATE_NODE_TYPE {
    _ZP_HASHMAP_TEMPLATE_KEY_TYPE key;
    _ZP_HASHMAP_TEMPLATE_VAL_TYPE val;
} _ZP_HASHMAP_TEMPLATE_NODE_TYPE;

// ── Slot ─────────────────────────────────────────────────────────────────────
//
// _info encodes two fields in a single size_t:
//   bit  0      : occupied flag  (0 = empty, 1 = live)
//   bits [N-1:1]: probe sequence length (PSL), i.e. distance from ideal bucket
//
// Invariant: _info == 0  ⟺  slot is empty.
//
// Accessor helpers (used internally):
//   _ZP_RH_HMAP_SLOT_OCCUPIED(s)       – non-zero if live
//   _ZP_RH_HMAP_SLOT_PSL(s)            – PSL value (0 = ideal position)
//   _ZP_RH_HMAP_SLOT_SET(s, psl)       – mark live with given PSL
//   _ZP_RH_HMAP_SLOT_CLEAR(s)          – mark empty

#define _ZP_RH_HMAP_SLOT_OCCUPIED(s) ((s)->_info & (size_t)1u)
#define _ZP_RH_HMAP_SLOT_PSL(s) ((s)->_info >> 1)
#define _ZP_RH_HMAP_SLOT_SET(s, psl) ((s)->_info = (((size_t)(psl)) << 1) | (size_t)1u)
#define _ZP_RH_HMAP_SLOT_CLEAR(s) ((s)->_info = (size_t)0u)
typedef struct _ZP_HASHMAP_TEMPLATE_SLOT_TYPE {
    _ZP_HASHMAP_TEMPLATE_NODE_TYPE node;  // key+val; valid when _info != 0
    size_t _info;                         // packed occupied flag (bit 0) + PSL (bits [N-1:1])
} _ZP_HASHMAP_TEMPLATE_SLOT_TYPE;

#define _ZP_RH_HMAP_IDX_TO_ITER(idx) ((idx) + 1u)
#define _ZP_RH_HMAP_ITER_TO_IDX(iter) ((iter) - 1u)

// ── Map type ──────────────────────────────────────────────────────────────────

typedef struct _ZP_HASHMAP_TEMPLATE_TYPE {
    _ZP_HASHMAP_TEMPLATE_SLOT_TYPE *_slots;  // heap-allocated slot array
    size_t _capacity;                        // current number of slots (always a power of two)
    size_t _size;                            // number of live entries
} _ZP_HASHMAP_TEMPLATE_TYPE;

// ── Internal helpers ──────────────────────────────────────────────────────────

// Mask for fast modulo when capacity is a power of two.
#define _ZP_RH_HMAP_MASK(map) ((map)->_capacity - 1u)

// Ideal slot for a hash value.
#define _ZP_RH_HMAP_BUCKET(hash, mask) ((hash) & (mask))

// Should we grow?  size * DEN > capacity * NUM
#define _ZP_RH_HMAP_NEEDS_GROW(map) \
    ((map)->_size * _ZP_HASHMAP_TEMPLATE_MAX_LOAD_DEN > (map)->_capacity * _ZP_HASHMAP_TEMPLATE_MAX_LOAD_NUM)

// ── new_with_capacity ────────────────────────────────────────────────────────────
// Initializes a new map. The map will be preallocated to specified capacity capacity, returns false if allocation
// failed.
static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, new_with_capacity)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                                         size_t capacity) {
    size_t cap = 1;
    while (cap < capacity) {
        cap = cap << 1u;
    }
    map->_capacity = cap;
    map->_size = 0;
    map->_slots = (_ZP_HASHMAP_TEMPLATE_SLOT_TYPE *)_ZP_HASHMAP_TEMPLATE_ALLOC_FN(
        map->_capacity * sizeof(_ZP_HASHMAP_TEMPLATE_SLOT_TYPE));
    if (map->_slots != NULL) {
        // Zero-initialise: _info == 0 means empty for every slot.
        memset(map->_slots, 0, map->_capacity * sizeof(_ZP_HASHMAP_TEMPLATE_SLOT_TYPE));
    }
    return map->_slots != NULL;
}

// ── new ───────────────────────────────────────────────────────────────────────
// Initializes a new map. The map will be preallocated to its default capacity, returns false if allocation failed.

static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, new)(_ZP_HASHMAP_TEMPLATE_TYPE *map) {
    return _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, new_with_capacity)(map, _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY);
}

// ── is_valid ──────────────────────────────────────────────────────────────────
// Returns false when new() failed (allocation error).

static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, is_valid)(const _ZP_HASHMAP_TEMPLATE_TYPE *map) {
    return map->_slots != NULL;
}

// ── size / is_empty ───────────────────────────────────────────────────────────

static inline size_t _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, size)(const _ZP_HASHMAP_TEMPLATE_TYPE *map) {
    return map->_size;
}

static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, is_empty)(const _ZP_HASHMAP_TEMPLATE_TYPE *map) {
    return map->_size == 0;
}

// ── Internal: raw insert into a pre-allocated slot array ─────────────────────
// Takes ownership of *candidate (key+val already moved in, _info already set).
// The caller must not access *candidate after this call returns.
//
// Algorithm (two-pass, no temporary needed):
//   Pass 1 — find the insertion position 'ins': the first slot that is empty
//             or whose stored PSL is less than the candidate's PSL at that
//             position (Robin Hood condition).
//   Pass 2 — from 'ins', find the next empty slot 'end', then shift every
//             element in [ins, end) one step forward (back-to-front traversal
//             so each destination is already vacated). Each shifted element's
//             PSL grows by 1 (+2 to its _info word).
//   Finally — place the candidate at 'ins' with the computed PSL.

static inline size_t _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, _raw_insert)(_ZP_HASHMAP_TEMPLATE_SLOT_TYPE *slots, size_t cap,
                                                                     _ZP_HASHMAP_TEMPLATE_SLOT_TYPE *candidate) {
    size_t mask = cap - 1u;
    size_t hash = _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN(&candidate->node.key);
    size_t ins = _ZP_RH_HMAP_BUCKET(hash, mask);
    size_t ins_psl = 0;

    // Pass 1: locate the insertion slot.
    for (;;) {
        _ZP_HASHMAP_TEMPLATE_SLOT_TYPE *slot = &slots[ins];
        if (!_ZP_RH_HMAP_SLOT_OCCUPIED(slot)) {
            // Fast path: insertion slot is empty — no shifting required.
            _ZP_RH_HMAP_SLOT_SET(&slots[ins], ins_psl);
            _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN(&slots[ins].node.key, &candidate->node.key);
            _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(&slots[ins].node.val, &candidate->node.val);
            return ins;
        } else if (_ZP_RH_HMAP_SLOT_PSL(slot) < ins_psl) {
            break;
        }
        ins_psl++;
        ins = (ins + 1u) & mask;
    }

    // Pass 2: find the next empty slot after 'ins'.
    size_t end = (ins + 1u) & mask;
    while (_ZP_RH_HMAP_SLOT_OCCUPIED(&slots[end])) {
        end = (end + 1u) & mask;
    }

    // Shift elements in [ins, end) one step forward, back-to-front.
    // n = number of elements to shift (handles wrap-around via mask arithmetic).
    // Iteration k (n down to 1):
    //   src = (ins + k - 1) & mask
    //   dst = (ins + k)     & mask   ← always already vacated or initially empty (k=n)
    size_t n = (end - ins) & mask;
    for (size_t k = n; k > 0u; k--) {
        size_t src = (ins + k - 1u) & mask;
        size_t dst = (ins + k) & mask;
        slots[dst]._info = slots[src]._info + (size_t)2u;  // PSL += 1
        _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN(&slots[dst].node.key, &slots[src].node.key);
        _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(&slots[dst].node.val, &slots[src].node.val);
    }

    // Place candidate at the insertion slot.
    _ZP_RH_HMAP_SLOT_SET(&slots[ins], ins_psl);
    _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN(&slots[ins].node.key, &candidate->node.key);
    _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(&slots[ins].node.val, &candidate->node.val);
    return ins;
}

// ── Internal: grow ────────────────────────────────────────────────────────────

static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, _grow)(_ZP_HASHMAP_TEMPLATE_TYPE *map) {
    size_t new_cap = map->_capacity * 2u;
    _ZP_HASHMAP_TEMPLATE_SLOT_TYPE *new_slots = (_ZP_HASHMAP_TEMPLATE_SLOT_TYPE *)_ZP_HASHMAP_TEMPLATE_ALLOC_FN(
        new_cap * sizeof(_ZP_HASHMAP_TEMPLATE_SLOT_TYPE));
    if (new_slots == NULL) {
        return false;
    }
    memset(new_slots, 0, new_cap * sizeof(_ZP_HASHMAP_TEMPLATE_SLOT_TYPE));

    // Re-insert every live entry into the new array.
    for (size_t i = 0; i < map->_capacity; i++) {
        if (_ZP_RH_HMAP_SLOT_OCCUPIED(&map->_slots[i])) {
            _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, _raw_insert)(new_slots, new_cap, &map->_slots[i]);
        }
    }
    _ZP_HASHMAP_TEMPLATE_FREE_FN(map->_slots);
    map->_slots = new_slots;
    map->_capacity = new_cap;
    return true;
}

// ── get_iter ────────────────────────────────────────
// Returns iterator to the slot holding key, the iterator will be invalid if the key is not found.
// Note: iterator maybe invalidated by any insertions or removals.
static inline size_t _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, get_iter)(const _ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                                  const _ZP_HASHMAP_TEMPLATE_KEY_TYPE *key) {
    if (map->_capacity == 0 || map->_slots == NULL) {
        return _ZP_HASHMAP_ITER_INVALID;
    }
    size_t mask = _ZP_RH_HMAP_MASK(map);
    size_t hash = _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN(key);
    size_t pos = _ZP_RH_HMAP_BUCKET(hash, mask);
    size_t psl = 0;

    for (;;) {
        const _ZP_HASHMAP_TEMPLATE_SLOT_TYPE *slot = &map->_slots[pos];
        if (!_ZP_RH_HMAP_SLOT_OCCUPIED(slot) || _ZP_RH_HMAP_SLOT_PSL(slot) < psl) {
            // The entry would have been placed here or earlier if it existed.
            return _ZP_HASHMAP_ITER_INVALID;
        }
        // A key hashing to our bucket always sits at exactly PSL == psl at this probe step.
        // If the stored PSL is greater, the occupant belongs to a different ideal bucket —
        // our key cannot be here, but may still be further along the probe chain.
        if (_ZP_RH_HMAP_SLOT_PSL(slot) == psl && _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN(&slot->node.key, key)) {
            return _ZP_RH_HMAP_IDX_TO_ITER(pos);
        }
        psl++;
        pos = (pos + 1u) & mask;
    }
}

// ── get ───────────────────────────────────────────────────────────────────────
// Returns a pointer to the value for key, or NULL if not found.

static inline _ZP_HASHMAP_TEMPLATE_VAL_TYPE *_ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME,
                                                     get)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                          const _ZP_HASHMAP_TEMPLATE_KEY_TYPE *key) {
    size_t idx = _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, get_iter)(map, key);
    if (idx == _ZP_HASHMAP_ITER_INVALID) {
        return NULL;
    }
    return &map->_slots[_ZP_RH_HMAP_ITER_TO_IDX(idx)].node.val;
}

// ── contains ─────────────────────────────────────────────────────────────────

static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, contains)(const _ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                                const _ZP_HASHMAP_TEMPLATE_KEY_TYPE *key) {
    return _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, get_iter)(map, key) != _ZP_HASHMAP_ITER_INVALID;
}

// ── insert ────────────────────────────────────────────────────────────────────
// Takes ownership of *key and *val via move semantics.
// If key already exists: old value is destroyed, new value is moved in;
// the new key is also destroyed (existing key kept in place).
// Returns a valid iterator to inserted element on success, invalid one on allocation failure.

static inline size_t _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, insert)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                                _ZP_HASHMAP_TEMPLATE_KEY_TYPE *key,
                                                                _ZP_HASHMAP_TEMPLATE_VAL_TYPE *val) {
    // Check for existing entry first.
    size_t existing = _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, get_iter)(map, key);
    if (existing != _ZP_HASHMAP_ITER_INVALID) {
        existing = _ZP_RH_HMAP_ITER_TO_IDX(existing);
        _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN(key);
        _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN(&map->_slots[existing].node.val);
        _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(&map->_slots[existing].node.val, val);
        return _ZP_RH_HMAP_IDX_TO_ITER(existing);
    }

    // Grow before inserting if needed.
    if (_ZP_RH_HMAP_NEEDS_GROW(map)) {
        if (!_ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, _grow)(map)) {
            return _ZP_HASHMAP_ITER_INVALID;
        }
    }

    // Build a candidate slot (PSL will be set to 0 inside _raw_insert).
    _ZP_HASHMAP_TEMPLATE_SLOT_TYPE candidate;
    memset(&candidate, 0, sizeof(candidate));
    candidate._info = (size_t)1u;  // occupied, PSL=0
    _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN(&candidate.node.key, key);
    _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(&candidate.node.val, val);

    size_t idx = _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, _raw_insert)(map->_slots, map->_capacity, &candidate);
    map->_size++;
    return _ZP_RH_HMAP_IDX_TO_ITER(idx);
}

// ── Iteration ─────────────────────────────────────────────────────────────────
//
// Pattern:
//   for (size_t i = map_iter(&map); i != _ZP_HASHMAP_ITER_INVALID; i = map_iter_next(&map, i)) {
//       map_node_t *n = map_node_at(&map, i);
//       // use n->key, n->val
//   }

// Returns the index of the next live slot after 'pos', or _ZP_HASHMAP_ITER_INVALID.
static inline size_t _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, iter_next)(const _ZP_HASHMAP_TEMPLATE_TYPE *map, size_t pos) {
    for (size_t i = pos; i < map->_capacity; i++) {
        if (_ZP_RH_HMAP_SLOT_OCCUPIED(&map->_slots[i])) {
            return _ZP_RH_HMAP_IDX_TO_ITER(i);
        }
    }
    return _ZP_HASHMAP_ITER_INVALID;
}

// Returns the index of the first live slot, or _ZP_HASHMAP_ITER_INVALID if the map is empty.
static inline size_t _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, iter)(const _ZP_HASHMAP_TEMPLATE_TYPE *map) {
    return _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, iter_next)(map, 0);
}

// ── remove_at ────────────────────────────────────────────────────────────────
// Remove the node at the iterator index (obtained from insert or a prior
// lookup).  Behaviour is undefined if idx is _ZP_HASHMAP_ITER_INVALID or has already been
// freed.  If out_val != NULL the value is moved out; otherwise it is destroyed.
// If next_idx != NULL, the iterator of the next slot is written to *next_idx (or _ZP_HASHMAP_ITER_INVALID if there are
// no more).
static inline void _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, remove_at)(_ZP_HASHMAP_TEMPLATE_TYPE *map, size_t idx,
                                                                 _ZP_HASHMAP_TEMPLATE_NODE_TYPE *out_val,
                                                                 size_t *next_idx) {
    idx = _ZP_RH_HMAP_ITER_TO_IDX(idx);  // shift back from incremented iterator
    size_t mask = _ZP_RH_HMAP_MASK(map);
    _ZP_HASHMAP_TEMPLATE_SLOT_TYPE *slots = map->_slots;

    if (out_val != NULL) {
        _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN(&out_val->key, &slots[idx].node.key);
        _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(&out_val->val, &slots[idx].node.val);
    } else {
        _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN(&slots[idx].node.key);
        _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN(&slots[idx].node.val);
    }
    _ZP_RH_HMAP_SLOT_CLEAR(&slots[idx]);
    map->_size--;

    // Backward-shift deletion: pull forward neighbours that are displaced.
    size_t cur = idx;
    size_t next = (cur + 1u) & mask;
    for (;;) {
        _ZP_HASHMAP_TEMPLATE_SLOT_TYPE *nb = &slots[next];
        if (!_ZP_RH_HMAP_SLOT_OCCUPIED(nb) || _ZP_RH_HMAP_SLOT_PSL(nb) == 0) {
            break;
        }
        // Move key+val from nb into cur, then decrement PSL by 1.
        slots[cur]._info = nb->_info - (size_t)2u;  // copy _info with PSL-1
        _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN(&slots[cur].node.key, &nb->node.key);
        _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(&slots[cur].node.val, &nb->node.val);
        _ZP_RH_HMAP_SLOT_CLEAR(nb);
        cur = next;
        next = (next + 1u) & mask;
    }
    if (next_idx != NULL) {
        // idx already corresponds to the iterator preceeding the removed one, so
        // it will force iter_next to reverify the slot at idx and return its iterator if it's still occupied, or the
        // next one otherwise.
        *next_idx = _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, iter_next)(map, idx);
    }
}

// ── remove ────────────────────────────────────────────────────────────────────
// If out_val != NULL the value is moved out; otherwise it is destroyed.
// Returns true if the key was found and removed.
//
// Uses backward-shift deletion: after removing a slot the algorithm shifts
// subsequent entries back until an empty slot or a PSL-0 entry is encountered.

static inline bool _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, remove)(_ZP_HASHMAP_TEMPLATE_TYPE *map,
                                                              const _ZP_HASHMAP_TEMPLATE_KEY_TYPE *key,
                                                              _ZP_HASHMAP_TEMPLATE_VAL_TYPE *out_val) {
    size_t idx = _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, get_iter)(map, key);
    if (idx == _ZP_HASHMAP_ITER_INVALID) {
        return false;
    }
    if (out_val == NULL) {
        _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, remove_at)(map, idx, NULL, NULL);
    } else {
        _ZP_HASHMAP_TEMPLATE_NODE_TYPE tmp;
        _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, remove_at)(map, idx, &tmp, NULL);
        _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(out_val, &tmp.val);
        _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN(&tmp.key);
    }
    return true;
}

// ── destroy ───────────────────────────────────────────────────────────────────
// Destroys all entries and frees the underlying slot array.

static inline void _ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME, destroy)(_ZP_HASHMAP_TEMPLATE_TYPE *map) {
    if (map->_slots == NULL) {
        return;
    }
    for (size_t i = 0; i < map->_capacity; i++) {
        if (_ZP_RH_HMAP_SLOT_OCCUPIED(&map->_slots[i])) {
            _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN(&map->_slots[i].node.key);
            _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN(&map->_slots[i].node.val);
        }
    }
    _ZP_HASHMAP_TEMPLATE_FREE_FN(map->_slots);
    map->_slots = NULL;
    map->_capacity = 0;
    map->_size = 0;
}

// Returns a pointer to the node (key+val) at the given iterator position.
// Behaviour is undefined if pos is _ZP_HASHMAP_ITER_INVALID or the slot is not occupied.
static inline _ZP_HASHMAP_TEMPLATE_NODE_TYPE *_ZP_CAT(_ZP_HASHMAP_TEMPLATE_NAME,
                                                      node_at)(_ZP_HASHMAP_TEMPLATE_TYPE *map, size_t pos) {
    return &map->_slots[_ZP_RH_HMAP_ITER_TO_IDX(pos)].node;
}

// ── Undef all macros ──────────────────────────────────────────────────────────

#undef _ZP_HASHMAP_TEMPLATE_KEY_TYPE
#undef _ZP_HASHMAP_TEMPLATE_VAL_TYPE
#undef _ZP_HASHMAP_TEMPLATE_NAME
#undef _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY
#undef _ZP_HASHMAP_TEMPLATE_MAX_LOAD_NUM
#undef _ZP_HASHMAP_TEMPLATE_MAX_LOAD_DEN
#undef _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN
#undef _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN
#undef _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN
#undef _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN
#undef _ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN
#undef _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN
#undef _ZP_HASHMAP_TEMPLATE_ALLOC_FN
#undef _ZP_HASHMAP_TEMPLATE_FREE_FN
#undef _ZP_HASHMAP_TEMPLATE_TYPE
#undef _ZP_HASHMAP_TEMPLATE_NODE_TYPE
#undef _ZP_HASHMAP_TEMPLATE_SLOT_TYPE
#undef _ZP_RH_HMAP_MASK
#undef _ZP_RH_HMAP_BUCKET
#undef _ZP_RH_HMAP_NEEDS_GROW
#undef _ZP_RH_HMAP_SLOT_OCCUPIED
#undef _ZP_RH_HMAP_SLOT_PSL
#undef _ZP_RH_HMAP_SLOT_SET
#undef _ZP_RH_HMAP_SLOT_CLEAR
#undef _ZP_RH_HMAP_IDX_TO_ITER
#undef _ZP_RH_HMAP_ITER_TO_IDX