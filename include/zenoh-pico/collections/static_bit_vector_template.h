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

// A fixed-capacity vector of single bits (0/1 values). The bits are packed into an inline
// array of words embedded in the generated struct, so the container performs no heap
// allocation. Its interface mirrors static_vector_template.h, with the differences forced by
// the fact that an individual bit is not addressable: accessors return/accept a bool *by value*
// instead of returning a pointer to the element.
//
// User needs to define the following macros before including this file:
// - _ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME: the name of the bit vector type to generate, without
//   the _t suffix (optional, default is derived from the size, e.g. bitvec_16)
// - _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE: the maximum capacity, in bits, of the fixed-capacity
//   array stored in the generated struct (optional, default is 16)
// - _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE: the unsigned integer type used as the backing
//   storage word (optional, default is size_t). Smaller types reduce the rounding-up overhead
//   of the inline array, while larger types speed up bulk word operations.

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zenoh-pico/collections/cat.h"

#ifndef _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE 16
#endif
#ifndef _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE size_t
#endif
#ifndef _ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME _ZP_CAT(bitvec, _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE)
#endif

// Number of bits in a single storage block of the configured type.
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS (sizeof(_ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE) * CHAR_BIT)

// Number of blocks needed to store SIZE bits.
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_NBLOCKS                                                  \
    (((_ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE) + _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS - 1u) / \
     _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS)

#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, t)
typedef struct _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE {
    _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE _blocks[_ZP_STATIC_BIT_VECTOR_TEMPLATE_NBLOCKS];
    size_t _size;
} _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE;

// block_t is the backing storage word type.
typedef _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, block_t);

// elem_t is the logical element type (a single bit, represented as a bool).
typedef bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, elem_t);

// iter_t is the iterator type (a plain index). Used for index-based iteration.
typedef size_t _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, iter_t);

// Initializes a new, empty bit vector. All fields are zero-initialised.
static inline void _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, init)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec) {
    memset(vec, 0, sizeof(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE));
}

// Creates a new, empty bit vector. All fields are zero-initialised.
static inline _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, new)(void) {
    _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE vec = {0};
    return vec;
}

// Returns the number of bits currently stored in the vector.
static inline size_t _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME,
                             size)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec) {
    return vec->_size;
}

// Returns the maximum number of bits the vector can hold.
static inline size_t _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME,
                             capacity)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec) {
    (void)vec;
    return _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE;
}

// Returns true if the vector contains no bits.
static inline bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME,
                           is_empty)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec) {
    return vec->_size == 0;
}

// Resets the vector to an empty state, clearing the underlying storage.
// Provided for parity with the other containers; bits own no resources so nothing is freed.
static inline void _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, destroy)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec) {
    memset(vec->_blocks, 0, sizeof(vec->_blocks));
    vec->_size = 0;
}

// Returns the bit at the given index, without bounds checking.
// Behaviour is undefined if index >= size.
static inline bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, at)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                    size_t index) {
    size_t block_idx = index / _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
    size_t bit_idx = index % _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
    return vec->_blocks[block_idx] &
           (_ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE)((_ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE)1 << bit_idx);
}

// const_at is identical to at: a bit is returned by value, so there is no separate const view.
static inline bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME,
                           const_at)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec, size_t index) {
    size_t block_idx = index / _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
    size_t bit_idx = index % _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
    return vec->_blocks[block_idx] &
           (_ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE)((_ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE)1 << bit_idx);
}

// Reads the bit at the given index with bounds checking.
// On success returns true and, if @p out is non-NULL, stores the bit value into *out.
// Returns false (leaving *out untouched) if the index is out of bounds.
static inline bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, get)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                     size_t index, bool *out) {
    if (index >= vec->_size) {
        return false;
    }
    if (out != NULL) {
        *out = _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, const_at)(vec, index);
    }
    return true;
}

// Sets the bit at the given index to @p value, without bounds checking.
// Behaviour is undefined if index >= size.
static inline void _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, set_at)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                        size_t index, bool value) {
    size_t block_idx = index / _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
    size_t bit_idx = index % _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
    _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE mask =
        (_ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE)((_ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE)1u << bit_idx);
    if (value) {
        vec->_blocks[block_idx] |= mask;
    } else {
        vec->_blocks[block_idx] &= ~mask;
    }
}

// Sets the bit at the given index to @p value with bounds checking.
// Returns true on success, or false if the index is out of bounds.
static inline bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, set)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                     size_t index, bool value) {
    if (index >= vec->_size) {
        return false;
    }
    _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, set_at)(vec, index, value);
    return true;
}

// Toggles the bit at the given index, without bounds checking.
// Behaviour is undefined if index >= size.
static inline void _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, flip_at)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                         size_t index) {
    size_t block_idx = index / _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
    size_t bit_idx = index % _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
    _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE mask =
        (_ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE)((_ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE)1u << bit_idx);
    vec->_blocks[block_idx] ^= mask;
}

// Toggles the bit at the given index with bounds checking.
// Returns true on success, or false if the index is out of bounds.
static inline bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, flip)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                      size_t index) {
    if (index >= vec->_size) {
        return false;
    }
    _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, flip_at)(vec, index);
    return true;
}

// Appends a bit to the back of the vector.
// Returns true on success, or false if the vector is at full capacity.
static inline bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, push_back)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                           bool value) {
    if (vec->_size == _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE) {
        return false;
    }
    _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, set_at)(vec, vec->_size, value);
    vec->_size++;
    return true;
}

// Appends @p len bits to the back of the vector, reading them by value from the bool array @p elems.
// Returns true on success, or false if the vector does not have enough remaining capacity,
// in which case the vector is left unchanged.
static inline bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, append)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                        const bool *elems, size_t len) {
    if (len == 0) {
        return true;
    }
    if (len > _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE - vec->_size) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, set_at)(vec, vec->_size + i, elems[i]);
    }
    vec->_size += len;
    return true;
}

// Removes the bit at the back of the vector.
// If @p out is non-NULL the removed bit is stored into *out.
// Returns true on success, or false if the vector is empty.
static inline bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, pop_back)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                          bool *out) {
    if (_ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, is_empty)(vec)) {
        return false;
    }
    vec->_size--;
    if (out != NULL) {
        *out = _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, at)(vec, vec->_size);
    }
    return true;
}

// Reads the bit at the front of the vector without removing it.
// On success returns true and, if @p out is non-NULL, stores the bit value into *out.
// Returns false if the vector is empty.
static inline bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, front)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                       bool *out) {
    if (_ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, is_empty)(vec)) {
        return false;
    }
    if (out != NULL) {
        *out = _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, const_at)(vec, 0);
    }
    return true;
}

// Reads the bit at the back of the vector without removing it.
// On success returns true and, if @p out is non-NULL, stores the bit value into *out.
// Returns false if the vector is empty.
static inline bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, back)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                      bool *out) {
    if (_ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, is_empty)(vec)) {
        return false;
    }
    if (out != NULL) {
        *out = _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, const_at)(vec, vec->_size - 1);
    }
    return true;
}

// Inserts a bit at the given index, shifting subsequent bits one position towards the back.
// Returns true on success, or false if the vector is at full capacity or the index is out of bounds.
static inline bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, insert)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                        size_t index, bool value) {
    if (vec->_size == _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE || index > vec->_size) {
        return false;
    }
    for (size_t i = vec->_size; i > index; i--) {
        bool b = _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, at)(vec, i - 1);
        _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, set_at)(vec, i, b);
    }
    _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, set_at)(vec, index, value);
    vec->_size++;
    return true;
}

// Removes the bit at the given index, shifting subsequent bits one position towards the front.
// If @p out is non-NULL the removed bit is stored into *out.
// Returns true on success, or false if the index is out of bounds.
static inline bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, remove)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                        size_t index, bool *out) {
    if (index >= vec->_size) {
        return false;
    }
    if (out != NULL) {
        *out = _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, const_at)(vec, index);
    }
    for (size_t i = index; i + 1 < vec->_size; i++) {
        bool b = _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, const_at)(vec, i + 1);
        _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, set_at)(vec, i, b);
    }
    vec->_size--;
    _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, set_at)(vec, vec->_size, false);
    return true;
}

// Removes the bit at the given iterator (index), shifting subsequent bits towards the front.
// Thin wrapper around remove() that mirrors the signature used by the other containers.
// If @p out is non-NULL the removed bit is stored into *out.
// If @p next_idx is non-NULL it is set to the iterator of the next bit to visit: because removal
// shifts the tail left, this is the same index when another bit followed, or the end() iterator
// when the removed bit was the last one.
// Behaviour is undefined if idx is out of bounds (idx >= size).
static inline void _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME,
                           remove_at)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                      _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, iter_t) idx, bool *out,
                                      _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, iter_t) * next_idx) {
    _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, remove)(vec, idx, out);
    if (next_idx != NULL) {
        *next_idx = idx;
    }
}

// Removes the bit at the given index in O(1) by moving the last bit into its place.
// This does NOT preserve the relative order of the remaining bits.
// If @p out is non-NULL the removed bit is stored into *out.
// Returns true on success, or false if the index is out of bounds (index >= size).
static inline bool _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, swap_remove)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                             size_t index, bool *out) {
    if (index >= vec->_size) {
        return false;
    }
    if (out != NULL) {
        *out = _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, const_at)(vec, index);
    }
    vec->_size--;
    if (index != vec->_size) {
        bool b = _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, const_at)(vec, vec->_size);
        _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, set_at)(vec, index, b);
    }
    _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, set_at)(vec, vec->_size, false);
    return true;
}

// Sets every bit currently in the vector to @p value.
// Operates a whole block at a time. The trailing partial block (if any) is written so that the
// dead high bits beyond the logical size stay zero, preserving the canonical-zero invariant.
static inline void _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, set_all)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                         bool value) {
    if (!value) {
        memset(vec->_blocks, 0, sizeof(vec->_blocks));
    } else {
        _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE all_ones =
            (_ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE)(~(_ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE)0);
        size_t num_blocks = vec->_size / _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
        for (size_t i = 0; i < num_blocks; i++) {
            vec->_blocks[i] = all_ones;
        }
        size_t rem = vec->_size - num_blocks * _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
        if (rem > 0) {
            vec->_blocks[num_blocks] = all_ones >> (_ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS - rem);
        }
    }
}

// Returns the number of bits set to 1 among the bits currently in the vector.
// Sums the population count of each full block plus the masked trailing partial block, so it is
// correct even if dead bits were dirtied through the raw block accessors.
static inline size_t _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME,
                             count)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec) {
    size_t num_blocks =
        (vec->_size + _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS - 1) / _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
    size_t cnt = 0;
    for (size_t i = 0; i < num_blocks; i++) {
        _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE block = vec->_blocks[i];
        while (block != 0) {
            cnt += (size_t)(block & 1u);
            block >>= 1;
        }
    }

    return cnt;
}

// Returns a pointer to the raw packed-bit storage (one bit per logical element, LSB-first
// within each block). Bit i lives in block i / (bits per block) at bit position
// i % (bits per block).
static inline _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE *_ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME,
                                                                 blocks)(_ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec) {
    return vec->_blocks;
}

// Returns a const pointer to the raw packed-bit storage.
static inline const _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE *_ZP_CAT(
    _ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, const_blocks)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec) {
    return vec->_blocks;
}

// Returns the number of blocks in the raw packed-bit storage (ceil(capacity / bits per block)).
static inline size_t _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME,
                             block_count)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec) {
    (void)vec;
    return _ZP_STATIC_BIT_VECTOR_TEMPLATE_NBLOCKS;
}

// Returns the number of bits stored in a single backing block.
static inline size_t _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME,
                             block_bits)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec) {
    (void)vec;
    return _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
}

// Returns the index of the first bit (always 0).
// Use together with end() for index-based iteration: for (size_t i = begin(v); i != end(v); i++).
// Dereference with at() or const_at().
static inline _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, iter_t)
    _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, begin)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec) {
    (void)vec;
    return 0;
}

// Returns the one-past-last index (equal to size). Used as the end sentinel for index iteration.
static inline _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, iter_t)
    _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, end)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec) {
    return vec->_size;
}

// Advances the iterator by one step.
static inline _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, iter_t)
    _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, iter_next)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                            _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, iter_t) pos) {
    (void)vec;
    return pos + 1;
}

// Returns the index of the first bit set to 1, or end() (== size) if no bit is set.
// Together with end()/iter_next_true() this iterates only over the set bits:
//   for (iter_t i = begin_true(v); i != end(v); i = iter_next_true(v, i)) { use i }
// Whole zero blocks are skipped, so the scan is cheap on sparse vectors.
static inline _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, iter_t)
    _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, begin_true)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec) {
    size_t num_blocks =
        (vec->_size + _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS - 1) / _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
    size_t i = 0;
    for (size_t block_idx = 0; block_idx < num_blocks; i = (++block_idx) * _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS) {
        _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE block = vec->_blocks[block_idx];
        while (block != 0) {
            if (block & 1u) {
                return i;
            }
            block = block >> 1;
            i++;
        }
    }
    return vec->_size;
}

// Returns the index of the first bit set to 1 strictly after @p pos, or end() (== size) if
// there is none. @p pos is normally an index previously returned by begin_true()/iter_next_true().
static inline _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, iter_t)
    _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, iter_next_true)(const _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE *vec,
                                                                 _ZP_CAT(_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME, iter_t)
                                                                     pos) {
    size_t num_blocks =
        (vec->_size + _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS - 1) / _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
    size_t i = pos + 1;
    size_t shift = i % _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
    size_t block_idx = i / _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS;
    for (; block_idx < num_blocks; i = (++block_idx) * _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS) {
        _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE block = vec->_blocks[block_idx] >> shift;
        while (block != 0) {
            if (block & 1u) {
                return i;
            }
            block = block >> 1;
            i++;
        }
        shift = 0;
    }
    return vec->_size;
}

#undef _ZP_STATIC_BIT_VECTOR_TEMPLATE_TYPE
#undef _ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME
#undef _ZP_STATIC_BIT_VECTOR_TEMPLATE_NBLOCKS
#undef _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_BITS
#undef _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE
#undef _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE
