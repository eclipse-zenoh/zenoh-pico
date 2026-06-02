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
#define _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(dst, src) *(dst) = *(src);
#endif

#define _ZP_STATIC_VECTOR_TEMPLATE_TYPE _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, t)
typedef struct _ZP_STATIC_VECTOR_TEMPLATE_TYPE {
    _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE _buffer[_ZP_STATIC_VECTOR_TEMPLATE_SIZE];
    size_t _size;
} _ZP_STATIC_VECTOR_TEMPLATE_TYPE;

typedef _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, elem_t);

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
static inline const _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME,
                                                                  const_get)(const _ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec,
                                                                             size_t index) {
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
static inline const _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME,
                                                                  const_at)(const _ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec,
                                                                            size_t index) {
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
    for (size_t i = vec->_size; i > index; i--) {
        _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(&vec->_buffer[i], &vec->_buffer[i - 1]);
    }
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
    for (size_t i = index + 1; i < vec->_size; i++) {
        _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(&vec->_buffer[i - 1], &vec->_buffer[i]);
    }
    vec->_size--;
    return true;
}

// elem_t is an alias for the element type, used by algorithms_template.h macros.
typedef _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, elem_t);

// iter_t is the iterator type (a plain index). Used by algorithms_template.h macros.
typedef size_t _ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME, iter_t);

// Returns a pointer to the raw buffer (first element). May be used for iteration.
// The valid range is [data, data + size).
static inline _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_STATIC_VECTOR_TEMPLATE_NAME,
                                                            data)(_ZP_STATIC_VECTOR_TEMPLATE_TYPE *vec) {
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
#undef _ZP_STATIC_VECTOR_TEMPLATE_SIZE
