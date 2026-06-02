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
// - _ZP_VECTOR_TEMPLATE_ELEM_TYPE: the type of the elements in the vector (required)
// - _ZP_VECTOR_TEMPLATE_NAME: the name of the vector type to generate, without the _t suffix
//   (optional, default is derived from the element type)
// - _ZP_VECTOR_TEMPLATE_INITIAL_CAPACITY: the initial heap allocation capacity
//   (optional, default is 16)
// - _ZP_VECTOR_TEMPLATE_ELEM_DESTROY_FN: the function-like macro used to destroy
//   an element (optional, default is a no-op)
// - _ZP_VECTOR_TEMPLATE_ELEM_MOVE_FN: the function-like macro used to move an
//   element (optional, default performs assignment without destroying the source element)
// - _ZP_VECTOR_TEMPLATE_ALLOC_FN: the function-like macro used to allocate memory
//   with signature void*(size_t bytes) (optional, default is malloc)
// - _ZP_VECTOR_TEMPLATE_FREE_FN: the function-like macro used to free memory
//   with signature void(void *ptr) (optional, default is free)

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/collections/cat.h"

#ifndef _ZP_VECTOR_TEMPLATE_ELEM_TYPE
#error "_ZP_VECTOR_TEMPLATE_ELEM_TYPE must be defined before including vector_template.h"
#define _ZP_VECTOR_TEMPLATE_ELEM_TYPE int
#endif
#ifndef _ZP_VECTOR_TEMPLATE_NAME
#define _ZP_VECTOR_TEMPLATE_NAME _ZP_CAT(_ZP_VECTOR_TEMPLATE_ELEM_TYPE, vec)
#endif

#ifndef _ZP_VECTOR_TEMPLATE_ELEM_DESTROY_FN
#define _ZP_VECTOR_TEMPLATE_ELEM_TRIVIALLY_DESTRUCTIBLE
#define _ZP_VECTOR_TEMPLATE_ELEM_DESTROY_FN(x) (void)(x)
#endif
#ifndef _ZP_VECTOR_TEMPLATE_ELEM_MOVE_FN
#define _ZP_VECTOR_TEMPLATE_ELEM_TRIVIALLY_MOVEABLE
#define _ZP_VECTOR_TEMPLATE_ELEM_MOVE_FN(dst, src) *(dst) = *(src);
#endif

#ifndef _ZP_VECTOR_TEMPLATE_ALLOC_FN
#define _ZP_VECTOR_TEMPLATE_ALLOC_FN(bytes) malloc(bytes)
#endif
// By default reallocation is implemented as malloc + free
// but a custom reallocation function can be provided by user to improve performance
// #define _ZP_VECTOR_TEMPLATE_REALLOC_FN(ptr, size) realloc(ptr, size)
#ifndef _ZP_VECTOR_TEMPLATE_FREE_FN
#define _ZP_VECTOR_TEMPLATE_FREE_FN(ptr) free(ptr)
#endif

#define _ZP_VECTOR_TEMPLATE_TYPE _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, t)
typedef struct _ZP_VECTOR_TEMPLATE_TYPE {
    _ZP_VECTOR_TEMPLATE_ELEM_TYPE *_buffer;
    size_t _size;
    size_t _capacity;
} _ZP_VECTOR_TEMPLATE_TYPE;

typedef _ZP_VECTOR_TEMPLATE_ELEM_TYPE _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, elem_t);

// Initializes a new, empty vector.
static inline void _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, init)(_ZP_VECTOR_TEMPLATE_TYPE *vec) {
    vec->_size = 0;
    vec->_capacity = 0;
    vec->_buffer = NULL;
}

// Creates a new, empty vector.
static inline _ZP_VECTOR_TEMPLATE_TYPE _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, new)(void) {
    _ZP_VECTOR_TEMPLATE_TYPE vec;
    _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, init)(&vec);
    return vec;
}

// Initializes an empty vector with specified capacity.
static inline bool _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, new_with_capacity)(_ZP_VECTOR_TEMPLATE_TYPE *v, size_t capacity) {
    if (capacity == 0) {
        v->_buffer = NULL;
    } else {
        v->_buffer = (_ZP_VECTOR_TEMPLATE_ELEM_TYPE *)_ZP_VECTOR_TEMPLATE_ALLOC_FN(
            capacity * sizeof(_ZP_VECTOR_TEMPLATE_ELEM_TYPE));
        if (v->_buffer == NULL) {
            return false;
        }
    }
    v->_size = 0;
    v->_capacity = capacity;
    return true;
}

// Returns the number of elements currently stored in the vector.
static inline size_t _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, size)(const _ZP_VECTOR_TEMPLATE_TYPE *vec) { return vec->_size; }

// Returns the current heap allocation capacity of the vector.
static inline size_t _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, capacity)(const _ZP_VECTOR_TEMPLATE_TYPE *vec) {
    return vec->_capacity;
}

// Returns true if the vector contains no elements.
static inline bool _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, is_empty)(const _ZP_VECTOR_TEMPLATE_TYPE *vec) {
    return vec->_size == 0;
}

// Destroys all elements in the vector by calling the configured destroy function on each one,
// frees the internal buffer, and resets the vector to an empty state.
// Does not free the vector struct itself.
static inline void _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, destroy)(_ZP_VECTOR_TEMPLATE_TYPE *vec) {
#if !defined(_ZP_VECTOR_TEMPLATE_ELEM_TRIVIALLY_DESTRUCTIBLE)
    for (size_t i = 0; i < vec->_size; i++) {
        _ZP_VECTOR_TEMPLATE_ELEM_DESTROY_FN(&vec->_buffer[i]);
    }
#endif
    _ZP_VECTOR_TEMPLATE_FREE_FN(vec->_buffer);
    vec->_buffer = NULL;
    vec->_size = 0;
    vec->_capacity = 0;
}

// Returns a pointer to the element at the given index, or NULL if out of bounds.
static inline _ZP_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, get)(_ZP_VECTOR_TEMPLATE_TYPE *vec,
                                                                                    size_t index) {
    if (index >= vec->_size) {
        return NULL;
    }
    return &vec->_buffer[index];
}

// Returns a const pointer to the element at the given index, or NULL if out of bounds.
static inline const _ZP_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME,
                                                           const_get)(const _ZP_VECTOR_TEMPLATE_TYPE *vec,
                                                                      size_t index) {
    if (index >= vec->_size) {
        return NULL;
    }
    return &vec->_buffer[index];
}

// Returns a pointer to the element at the given index, without bounds checking.
static inline _ZP_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, at)(_ZP_VECTOR_TEMPLATE_TYPE *vec,
                                                                                   size_t index) {
    return &vec->_buffer[index];
}

// Returns a const pointer to the element at the given index, without bounds checking.
static inline const _ZP_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME,
                                                           const_at)(const _ZP_VECTOR_TEMPLATE_TYPE *vec,
                                                                     size_t index) {
    return &vec->_buffer[index];
}

// Grows the internal buffer to at least @p new_capacity elements.
// Allocates a fresh buffer, moves each element individually (safe for non-trivially moveable types),
// then frees the old raw buffer. Returns true on success, or false if the allocation fails
// (the vector is left unchanged in that case).
static inline bool _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, reserve)(_ZP_VECTOR_TEMPLATE_TYPE *vec, size_t new_capacity) {
    if (new_capacity <= vec->_capacity) {
        return true;
    }
    _ZP_VECTOR_TEMPLATE_ELEM_TYPE *new_buffer;
#if defined(_ZP_VECTOR_TEMPLATE_REALLOC_FN) && defined(_ZP_VECTOR_TEMPLATE_ELEM_TRIVIALLY_MOVEABLE)
    new_buffer = (_ZP_VECTOR_TEMPLATE_ELEM_TYPE *)_ZP_VECTOR_TEMPLATE_REALLOC_FN(
        vec->_buffer, new_capacity * sizeof(_ZP_VECTOR_TEMPLATE_ELEM_TYPE));
    if (new_buffer == NULL) {
        return false;
    } else {
        vec->_buffer = new_buffer;
        vec->_capacity = new_capacity;
        return true;
    }
#else
    new_buffer = (_ZP_VECTOR_TEMPLATE_ELEM_TYPE *)_ZP_VECTOR_TEMPLATE_ALLOC_FN(new_capacity *
                                                                               sizeof(_ZP_VECTOR_TEMPLATE_ELEM_TYPE));
    if (new_buffer == NULL) {
        return false;
    }
#if defined(_ZP_VECTOR_TEMPLATE_ELEM_TRIVIALLY_MOVEABLE)
    memcpy(new_buffer, vec->_buffer, vec->_size * sizeof(_ZP_VECTOR_TEMPLATE_ELEM_TYPE));
#else
    for (size_t i = 0; i < vec->_size; i++) {
        _ZP_VECTOR_TEMPLATE_ELEM_MOVE_FN(&new_buffer[i], &vec->_buffer[i]);
    }
#endif
    _ZP_VECTOR_TEMPLATE_FREE_FN(vec->_buffer);
    vec->_buffer = new_buffer;
    vec->_capacity = new_capacity;
    return true;
#endif
}

// Appends an element to the back of the vector by moving it from @p elem.
// Grows the internal buffer (doubling capacity) if needed.
// Returns true on success, or false if a reallocation was required but failed.
static inline bool _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, push_back)(_ZP_VECTOR_TEMPLATE_TYPE *vec,
                                                                _ZP_VECTOR_TEMPLATE_ELEM_TYPE *elem) {
    if (vec->_size == vec->_capacity) {
        size_t new_capacity = vec->_capacity == 0 ? 1 : vec->_capacity * 2;
        if (!_ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, reserve)(vec, new_capacity)) {
            return false;
        }
    }
    _ZP_VECTOR_TEMPLATE_ELEM_MOVE_FN(&vec->_buffer[vec->_size], elem);
    vec->_size++;
    return true;
}

// Removes the element at the back of the vector.
// If @p out is non-NULL the element is moved into it; otherwise it is destroyed in place.
// Returns true on success, or false if the vector is empty.
static inline bool _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, pop_back)(_ZP_VECTOR_TEMPLATE_TYPE *vec,
                                                               _ZP_VECTOR_TEMPLATE_ELEM_TYPE *out) {
    if (_ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, is_empty)(vec)) {
        return false;
    }
    vec->_size--;
    if (out != NULL) {
        _ZP_VECTOR_TEMPLATE_ELEM_MOVE_FN(out, &vec->_buffer[vec->_size]);
    } else {
        _ZP_VECTOR_TEMPLATE_ELEM_DESTROY_FN(&vec->_buffer[vec->_size]);
    }
    return true;
}

// Returns a pointer to the element at the back of the vector without removing it,
// or NULL if the vector is empty.
static inline _ZP_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, back)(_ZP_VECTOR_TEMPLATE_TYPE *vec) {
    if (_ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, is_empty)(vec)) {
        return NULL;
    }
    return &vec->_buffer[vec->_size - 1];
}

// Returns a pointer to the element at the front of the vector without removing it,
// or NULL if the vector is empty.
static inline _ZP_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, front)(_ZP_VECTOR_TEMPLATE_TYPE *vec) {
    if (_ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, is_empty)(vec)) {
        return NULL;
    }
    return &vec->_buffer[0];
}

// elem_t is an alias for the element type, used by algorithms_template.h macros.
typedef _ZP_VECTOR_TEMPLATE_ELEM_TYPE _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, elem_t);

// iter_t is the iterator type (a plain index). Used by algorithms_template.h macros.
typedef size_t _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, iter_t);

// Returns the iterator to the raw buffer (first element). May be used for iteration.
// The valid range is [data, data + size).
static inline _ZP_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, data)(_ZP_VECTOR_TEMPLATE_TYPE *vec) {
    return vec->_buffer;
}

// Returns the index of the first element (always 0).
// Use together with end() for index-based iteration: for (size_t i = begin(v); i != end(v); i++).
// Dereference with at() or const_at(). Indices remain stable across growth.
static inline _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, iter_t)
    _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, begin)(const _ZP_VECTOR_TEMPLATE_TYPE *vec) {
    (void)vec;
    return 0;
}

// Returns the one-past-last index (equal to size). Used as the end sentinel for index iteration.
static inline _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, iter_t)
    _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, end)(const _ZP_VECTOR_TEMPLATE_TYPE *vec) {
    return vec->_size;
}

// Advances the iterator by one step.
static inline _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, iter_t)
    _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, iter_next)(const _ZP_VECTOR_TEMPLATE_TYPE *vec,
                                                 _ZP_CAT(_ZP_VECTOR_TEMPLATE_NAME, iter_t) pos) {
    (void)vec;
    return pos + 1;
}

#undef _ZP_VECTOR_TEMPLATE_TYPE
#undef _ZP_VECTOR_TEMPLATE_ELEM_TYPE
#undef _ZP_VECTOR_TEMPLATE_NAME
#undef _ZP_VECTOR_TEMPLATE_ELEM_DESTROY_FN
#undef _ZP_VECTOR_TEMPLATE_ELEM_MOVE_FN
#undef _ZP_VECTOR_TEMPLATE_ELEM_TRIVIALLY_DESTRUCTIBLE
#undef _ZP_VECTOR_TEMPLATE_ELEM_TRIVIALLY_MOVEABLE
#undef _ZP_VECTOR_TEMPLATE_ALLOC_FN
#ifdef _ZP_VECTOR_TEMPLATE_REALLOC_FN
#undef _ZP_VECTOR_TEMPLATE_REALLOC_FN
#endif
#undef _ZP_VECTOR_TEMPLATE_FREE_FN
