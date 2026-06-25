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

// User needs to define the following macros before including this file:
// - _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE: the type of the elements in the vector (required)
// - _ZP_STATIC_VECTOR_TEMPLATE_NAME: the name of the vector type to generate, without the _t suffix
//   (optional, default is derived from the element type and size)
// - _ZP_STATIC_VECTOR_TEMPLATE_SIZE: the maximum capacity of the fixed-capacity array
//   stored in the generated struct (optional, default is 16)
// - _ZP_STATIC_VECTOR_TEMPLATE_ELEM_DESTROY_FN: the function-like macro used to destroy
//   an element (optional, default is a no-op)
// - _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN: the function-like macro used to move an
//   element (optional, default performs assignment without destroying the source element)

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/collections/cat.h"

#ifndef _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE
#error "_ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE must be defined before including static_vector_template.h"
#define _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE int
#endif
#ifndef _ZP_STATIC_VECTOR_TEMPLATE_SIZE
#define _ZP_STATIC_VECTOR_TEMPLATE_SIZE 16
#endif
#ifndef _ZP_STATIC_VECTOR_TEMPLATE_NAME
#define _ZP_STATIC_VECTOR_TEMPLATE_NAME \
    _ZP_CAT(_ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE, vec), _ZP_STATIC_VECTOR_TEMPLATE_SIZE)
#endif

#ifndef _ZP_STATIC_VECTOR_TEMPLATE_ELEM_DESTROY_FN
#define _ZP_STATIC_VECTOR_TEMPLATE_ELEM_DESTROY_FN(x) (void)(x)
#endif
#ifndef _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN
#define _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TRIVIALLY_MOVEABLE
#define _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(dst, src) *(dst) = *(src);
#endif

#define _ZP_STATIC_VECTOR_TEMPLATE_TYPE _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, t)
typedef struct _ZP_STATIC_VECTOR_TEMPLATE_TYPE {
    _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE _buffer[_ZP_STATIC_VECTOR_TEMPLATE_SIZE];
    size_t _size;
} _ZP_STATIC_VECTOR_TEMPLATE_TYPE;

typedef _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, elem_t);

// iter_t is the iterator type (a plain index). Used by algorithms_template.h macros.
typedef size_t _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, iter_t);

// Initializes a new, empty vector. All fields are zero-initialised.
static inline void _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, init)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec) {
    memset(vec, 0, sizeof(_ZP_STATIC_VECTOR_TEMPLATE_TYPE));
}

// Creates a new, empty vector. All fields are zero-initialised.
static inline _ZP_STATIC_VECTOR_TEMPLATE_TYPE _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, new)(void) {
    _ZP_STATIC_VECTOR_TEMPLATE_TYPE vec = {0};
    return vec;
}

// Returns the number of elements currently stored in the vector.
static inline size_t _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, size)(const _ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec) {
    return vec->_size;
}

// Returns the maximum number of elements the vector can hold.
static inline size_t _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, capacity)(const _ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec) {
    (void)vec;
    return _ZP_STATIC_VECTOR_TEMPLATE_SIZE;
}

// Returns true if the vector contains no elements.
static inline bool _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, is_empty)(const _ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec) {
    return vec->_size == 0;
}

// Destroys all elements in the vector by calling the configured destroy function on each one,
// then resets the vector to an empty state. Does not free the vector struct itself.
static inline void _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, destroy)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec) {
    for (size_t i = 0; i < vec->_size; i++) {
        _ZP_STATIC_VECTOR_TEMPLATE_ELEM_DESTROY_FN(&vec->_buffer[i]);
    }
    vec->_size = 0;
}

// Returns a pointer to the element at the given index, or NULL if out of bounds.
static inline _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME,
                                                            get)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec, size_t index) {
    if (index >= vec->_size) {
        return NULL;
    }
    return &vec->_buffer[index];
}

// Returns a const pointer to the element at the given index, or NULL if out of bounds.
// Note: the `const` is applied to the `elem_t` typedef rather than spelled out as
// `const _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *`. When the element type is itself a pointer
// (e.g. `char *`), the latter would expand to `const char **`, which qualifies the
// wrong pointer level and discards qualifiers when returning `&vec->_buffer[index]`.
// Using the typedef yields `<elem> const *` (e.g. `char * const *`), which is correct.
static inline const _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, elem_t) *
    _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, const_get)(const _ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec, size_t index) {
    if (index >= vec->_size) {
        return NULL;
    }
    return &vec->_buffer[index];
}

// Returns a pointer to the element at the given index, without bounds checking.
static inline _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME,
                                                            at)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec, size_t index) {
    return &vec->_buffer[index];
}

// Returns a const pointer to the element at the given index, without bounds checking.
static inline const _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, elem_t) *
    _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, const_at)(const _ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec, size_t index) {
    return &vec->_buffer[index];
}

// Appends an element to the back of the vector by moving it from @p elem.
// Returns true on success, or false if the vector is at full capacity.
static inline bool _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, push_back)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec,
                                                                       _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *elem) {
    if (vec->_size == _ZP_STATIC_VECTOR_TEMPLATE_SIZE) {
        return false;
    }
    _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(&vec->_buffer[vec->_size], elem);
    vec->_size++;
    return true;
}

// Appends @p len elements to the back of the vector by moving them from the array @p elems.
// On success every element in [elems, elems + len) is moved into the vector.
// Returns true on success, or false if the vector does not have enough remaining capacity,
// in which case the vector is left unchanged and no elements are moved.
static inline bool _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, append)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec,
                                                                    _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *elems,
                                                                    size_t len) {
    if (len == 0) {
        return true;
    }
    if (len > _ZP_STATIC_VECTOR_TEMPLATE_SIZE - vec->_size) {
        return false;
    }
#if defined(_ZP_STATIC_VECTOR_TEMPLATE_ELEM_TRIVIALLY_MOVEABLE)
    // SAFETY: The check above ensures we have enough capacity in the vector to hold all len elements.
    // Flawfinder: ignore [CWE-120]
    memcpy(&vec->_buffer[vec->_size], elems, len * sizeof(_ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE));
#else
    for (size_t i = 0; i < len; i++) {
        _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(&vec->_buffer[vec->_size + i], &elems[i]);
    }
#endif
    vec->_size += len;
    return true;
}

// Removes the element at the back of the vector.
// If @p out is non-NULL the element is moved into it; otherwise it is destroyed in place.
// Returns true on success, or false if the vector is empty.
static inline bool _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, pop_back)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec,
                                                                      _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *out) {
    if (_ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, is_empty)(vec)) {
        return false;
    }
    vec->_size--;
    if (out != NULL) {
        _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(out, &vec->_buffer[vec->_size]);
    } else {
        _ZP_STATIC_VECTOR_TEMPLATE_ELEM_DESTROY_FN(&vec->_buffer[vec->_size]);
    }
    return true;
}

// Returns a pointer to the element at the back of the vector without removing it,
// or NULL if the vector is empty.
static inline _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME,
                                                            back)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec) {
    if (_ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, is_empty)(vec)) {
        return NULL;
    }
    return &vec->_buffer[vec->_size - 1];
}

// Returns a pointer to the element at the front of the vector without removing it,
// or NULL if the vector is empty.
static inline _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME,
                                                            front)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec) {
    if (_ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, is_empty)(vec)) {
        return NULL;
    }
    return &vec->_buffer[0];
}

// Inserts an element at the given index by moving it from @p elem, shifting subsequent elements right.
// Returns true on success, or false if the vector is at full capacity or the index is out of bounds.
static inline bool _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, insert)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec, size_t index,
                                                                    _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *elem) {
    if (vec->_size == _ZP_STATIC_VECTOR_TEMPLATE_SIZE || index > vec->_size) {
        return false;
    }
#if defined(_ZP_STATIC_VECTOR_TEMPLATE_ELEM_TRIVIALLY_MOVEABLE)
    memmove(&vec->_buffer[index + 1], &vec->_buffer[index],
            (vec->_size - index) * sizeof(_ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE));
#else
    for (size_t i = vec->_size; i > index; i--) {
        _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(&vec->_buffer[i], &vec->_buffer[i - 1]);
    }
#endif
    _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(&vec->_buffer[index], elem);
    vec->_size++;
    return true;
}

// Removes the element at the given index, shifting subsequent elements left.
// If @p out is non-NULL the removed element is moved into it; otherwise it is destroyed in place.
// Returns true on success, or false if the index is out of bounds.
static inline bool _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, remove)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec, size_t index,
                                                                    _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *out) {
    if (index >= vec->_size) {
        return false;
    }
    if (out != NULL) {
        _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(out, &vec->_buffer[index]);
    } else {
        _ZP_STATIC_VECTOR_TEMPLATE_ELEM_DESTROY_FN(&vec->_buffer[index]);
    }
#if defined(_ZP_STATIC_VECTOR_TEMPLATE_ELEM_TRIVIALLY_MOVEABLE)
    memmove(&vec->_buffer[index], &vec->_buffer[index + 1],
            (vec->_size - index - 1) * sizeof(_ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE));
#else
    for (size_t i = index + 1; i < vec->_size; i++) {
        _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(&vec->_buffer[i - 1], &vec->_buffer[i]);
    }
#endif
    vec->_size--;
    return true;
}

// Removes the element at the given iterator (index), shifting subsequent elements left.
// Thin wrapper around remove() that mirrors the hashmap remove_at signature so the vector can be
// used with the _ZP_REMOVE macro from algorithms_template.h.
// If @p out is non-NULL the removed element is moved into it; otherwise it is destroyed in place.
// If @p next_idx is non-NULL it is set to the iterator of the next element to visit: because removal
// shifts the tail left, this is the same index when another element followed, or the end() iterator
// when the removed element was the last one.
// Behaviour is undefined if idx is out of bounds (idx >= size).
static inline void _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME,
                           remove_at)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec,
                                      _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, iter_t) idx,
                                      _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *out,
                                      _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, iter_t) * next_idx) {
    _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, remove)(vec, idx, out);
    if (next_idx != NULL) {
        // After the left shift, idx addresses the element that followed the removed one, or equals
        // end() (== _size) when the removed element was the last.
        *next_idx = idx;
    }
}

// Removes the element at the given index in O(1) by moving the last element into its place.
// This does NOT preserve the relative order of the remaining elements.
// If @p out is non-NULL the removed element is moved into it; otherwise it is destroyed in place.
// If the removed element was not the last one, the last element is moved into the vacated slot.
// Returns true on success, or false if the index is out of bounds (index >= size).
static inline bool _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, swap_remove)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec,
                                                                         size_t index,
                                                                         _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *out) {
    if (index >= vec->_size) {
        return false;
    }
    if (out != NULL) {
        _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(out, &vec->_buffer[index]);
    } else {
        _ZP_STATIC_VECTOR_TEMPLATE_ELEM_DESTROY_FN(&vec->_buffer[index]);
    }
    vec->_size--;
    // If the removed element was not the last, move the (former) last element into the hole.
    if (index != vec->_size) {
        _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(&vec->_buffer[index], &vec->_buffer[vec->_size]);
    }
    return true;
}

// Returns a pointer to the raw buffer.
static inline _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME,
                                                            data)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec) {
    return vec->_buffer;
}

// Returns a const pointer to the raw buffer.
// See the note on const_get regarding the use of the `elem_t` typedef for the `const` qualifier.
static inline const _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, elem_t) *
    _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, const_data)(const _ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec) {
    return vec->_buffer;
}

// Returns the index of the first element (always 0).
// Use together with end() for index-based iteration: for (size_t i = begin(v); i != end(v); i++).
// Dereference with at() or const_at(). Indices remain stable across growth.
static inline _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, iter_t)
    _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, begin)(const _ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec) {
    (void)vec;
    return 0;
}

// Returns the one-past-last index (equal to size). Used as the end sentinel for index iteration.
static inline _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, iter_t)
    _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, end)(const _ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec) {
    return vec->_size;
}

// Advances the iterator by one step.
static inline _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, iter_t)
    _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, iter_next)(const _ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec,
                                                        _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, iter_t) pos) {
    (void)vec;
    return pos + 1;
}

#undef _ZP_STATIC_VECTOR_TEMPLATE_TYPE
#undef _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE
#undef _ZP_STATIC_VECTOR_TEMPLATE_NAME
#undef _ZP_STATIC_VECTOR_TEMPLATE_ELEM_DESTROY_FN
#undef _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN
#ifdef _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TRIVIALLY_MOVEABLE
#undef _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TRIVIALLY_MOVEABLE
#endif
#undef _ZP_STATIC_VECTOR_TEMPLATE_SIZE
