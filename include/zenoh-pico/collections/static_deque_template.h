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
// - _ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE: the type of the elements in the deque (required)
// - _ZP_STATIC_DEQUE_TEMPLATE_NAME: the name of the deque type to generate, without the _t suffix
//   (optional, default is derived from the element type and size)
// - _ZP_STATIC_DEQUE_TEMPLATE_SIZE: the maximum size of the fixed-capacity circular buffer
//   stored in the generated struct (optional, default is 16)
// - _ZP_STATIC_DEQUE_TEMPLATE_ELEM_DESTROY_FN: the function-like macro used to destroy
//   an element (optional, default is a no-op)
// - _ZP_STATIC_DEQUE_TEMPLATE_ELEM_MOVE_FN: the function-like macro used to move an
//   element (optional, default performs assignment without destroying the source element)

#include <stdbool.h>
#include <stddef.h>

#include "zenoh-pico/collections/cat.h"

#ifndef _ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE
#error "_ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE must be defined before including static_deque_template.h"
#define _ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE int
#endif
#ifndef _ZP_STATIC_DEQUE_TEMPLATE_SIZE
#define _ZP_STATIC_DEQUE_TEMPLATE_SIZE 16
#endif
#ifndef _ZP_STATIC_DEQUE_TEMPLATE_NAME
#define _ZP_STATIC_DEQUE_TEMPLATE_NAME \
    _ZP_CAT(_ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE, deque), _ZP_STATIC_DEQUE_TEMPLATE_SIZE)
#endif

#ifndef _ZP_STATIC_DEQUE_TEMPLATE_ELEM_DESTROY_FN
#define _ZP_STATIC_DEQUE_TEMPLATE_ELEM_DESTROY_FN(x) (void)(x)
#endif
#ifndef _ZP_STATIC_DEQUE_TEMPLATE_ELEM_MOVE_FN
#define _ZP_STATIC_DEQUE_TEMPLATE_ELEM_MOVE_FN(dst, src) *(dst) = *(src);
#endif

#define _ZP_STATIC_DEQUE_TEMPLATE_TYPE _ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME, t)
typedef struct _ZP_STATIC_DEQUE_TEMPLATE_TYPE {
    _ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE _buffer[_ZP_STATIC_DEQUE_TEMPLATE_SIZE];
    size_t _start;
    size_t _size;
} _ZP_STATIC_DEQUE_TEMPLATE_TYPE;

// Creates a new, empty deque. All fields are zero-initialised.
static inline _ZP_STATIC_DEQUE_TEMPLATE_TYPE _ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME, new)(void) {
    _ZP_STATIC_DEQUE_TEMPLATE_TYPE deque = {0};
    return deque;
}

// Returns the number of elements currently stored in the deque.
static inline size_t _ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME, size)(const _ZP_STATIC_DEQUE_TEMPLATE_TYPE *deque) {
    return deque->_size;
}

// Destroys all elements in the deque by calling the configured destroy function on each one,
// then resets the deque to an empty state. Does not free the deque struct itself.
static inline void _ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME, destroy)(_ZP_STATIC_DEQUE_TEMPLATE_TYPE *deque) {
    for (size_t i = 0; i < deque->_size; i++) {
        size_t idx = deque->_start + i;
        if (idx >= _ZP_STATIC_DEQUE_TEMPLATE_SIZE) {
            idx -= _ZP_STATIC_DEQUE_TEMPLATE_SIZE;
        }
        _ZP_STATIC_DEQUE_TEMPLATE_ELEM_DESTROY_FN(&deque->_buffer[idx]);
    }
    deque->_start = 0;
    deque->_size = 0;
}

// Returns true if the deque contains no elements.
static inline bool _ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME, is_empty)(const _ZP_STATIC_DEQUE_TEMPLATE_TYPE *deque) {
    return deque->_size == 0;
}

// Appends an element to the back of the deque by moving it from @p elem.
// Returns true on success, or false if the deque is at full capacity.
static inline bool _ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME, push_back)(_ZP_STATIC_DEQUE_TEMPLATE_TYPE *deque,
                                                                      _ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE *elem) {
    if (deque->_size == _ZP_STATIC_DEQUE_TEMPLATE_SIZE) {
        return false;
    }
    size_t idx = deque->_start + deque->_size;
    if (idx >= _ZP_STATIC_DEQUE_TEMPLATE_SIZE) {
        idx -= _ZP_STATIC_DEQUE_TEMPLATE_SIZE;
    }
    _ZP_STATIC_DEQUE_TEMPLATE_ELEM_MOVE_FN(&deque->_buffer[idx], elem);
    deque->_size++;
    return true;
}

// Removes the element at the back of the deque.
// If @p out is non-NULL the element is moved into it; otherwise it is destroyed in place.
// Returns true on success, or false if the deque is empty.
static inline bool _ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME, pop_back)(_ZP_STATIC_DEQUE_TEMPLATE_TYPE *deque,
                                                                     _ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE *out) {
    if (_ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME, is_empty)(deque)) {
        return false;
    }
    size_t idx = deque->_start + deque->_size - 1;
    if (idx >= _ZP_STATIC_DEQUE_TEMPLATE_SIZE) {
        idx -= _ZP_STATIC_DEQUE_TEMPLATE_SIZE;
    }
    if (out != NULL) {
        _ZP_STATIC_DEQUE_TEMPLATE_ELEM_MOVE_FN(out, &deque->_buffer[idx]);
    } else {
        _ZP_STATIC_DEQUE_TEMPLATE_ELEM_DESTROY_FN(&deque->_buffer[idx]);
    }
    if (deque->_size == 1) {
        deque->_start = 0;  // reset to initial state when empty
    }
    deque->_size--;
    return true;
}

// Returns a pointer to the element at the back of the deque without removing it,
// or NULL if the deque is empty.
static inline _ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME,
                                                           back)(_ZP_STATIC_DEQUE_TEMPLATE_TYPE *deque) {
    if (_ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME, is_empty)(deque)) {
        return NULL;
    }
    size_t idx = deque->_start + deque->_size - 1;
    if (idx >= _ZP_STATIC_DEQUE_TEMPLATE_SIZE) {
        idx -= _ZP_STATIC_DEQUE_TEMPLATE_SIZE;
    }
    return &deque->_buffer[idx];
}

// Prepends an element to the front of the deque by moving it from @p elem.
// Returns true on success, or false if the deque is at full capacity.
static inline bool _ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME, push_front)(_ZP_STATIC_DEQUE_TEMPLATE_TYPE *deque,
                                                                       _ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE *elem) {
    if (_ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME, size)(deque) == _ZP_STATIC_DEQUE_TEMPLATE_SIZE) {
        return false;
    }
    if (deque->_start == 0) {
        deque->_start = _ZP_STATIC_DEQUE_TEMPLATE_SIZE;
    }
    deque->_start--;
    deque->_size++;
    _ZP_STATIC_DEQUE_TEMPLATE_ELEM_MOVE_FN(&deque->_buffer[deque->_start], elem);
    return true;
}

// Removes the element at the front of the deque.
// If @p out is non-NULL the element is moved into it; otherwise it is destroyed in place.
// Returns true on success, or false if the deque is empty.
static inline bool _ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME, pop_front)(_ZP_STATIC_DEQUE_TEMPLATE_TYPE *deque,
                                                                      _ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE *out) {
    if (_ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME, is_empty)(deque)) {
        return false;
    }
    if (out != NULL) {
        _ZP_STATIC_DEQUE_TEMPLATE_ELEM_MOVE_FN(out, &deque->_buffer[deque->_start]);
    } else {
        _ZP_STATIC_DEQUE_TEMPLATE_ELEM_DESTROY_FN(&deque->_buffer[deque->_start]);
    }
    deque->_start++;
    if (deque->_start == _ZP_STATIC_DEQUE_TEMPLATE_SIZE) {
        deque->_start = 0;
    }
    deque->_size--;
    return true;
}

// Returns a pointer to the element at the front of the deque without removing it,
// or NULL if the deque is empty.
static inline _ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME,
                                                           front)(_ZP_STATIC_DEQUE_TEMPLATE_TYPE *deque) {
    if (_ZP_CAT(_ZP_STATIC_DEQUE_TEMPLATE_NAME, is_empty)(deque)) {
        return NULL;
    }
    return &deque->_buffer[deque->_start];
}

#undef _ZP_STATIC_DEQUE_TEMPLATE_TYPE
#undef _ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE
#undef _ZP_STATIC_DEQUE_TEMPLATE_NAME
#undef _ZP_STATIC_DEQUE_TEMPLATE_NODE_TYPE
#undef _ZP_STATIC_DEQUE_TEMPLATE_ELEM_DESTROY_FN
#undef _ZP_STATIC_DEQUE_TEMPLATE_ELEM_MOVE_FN
#undef _ZP_STATIC_DEQUE_TEMPLATE_SIZE
