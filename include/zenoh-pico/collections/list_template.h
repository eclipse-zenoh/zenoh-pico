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
// - _ZP_LIST_TEMPLATE_ELEM_TYPE: the type of the elements in the list (required)
// - _ZP_LIST_TEMPLATE_NAME: the name of the list type to generate, without the _t suffix
//   (optional, default is derived from the element type)
// - _ZP_LIST_TEMPLATE_ELEM_DESTROY_FN: the function-like macro used to destroy
//   an element (optional, default is a no-op)
// - _ZP_LIST_TEMPLATE_ELEM_MOVE_FN: the function-like macro used to move an
//   element (optional, default performs assignment without destroying the source element)
// - _ZP_LIST_TEMPLATE_ALLOC_CTX_TYPE: the type of the allocator context stored inside
//   the list struct (optional). When defined:
//     - _ZP_LIST_TEMPLATE_ALLOC_FN must have signature: void*(ctx_type *ctx, size_t bytes)
//     - _ZP_LIST_TEMPLATE_FREE_FN must have signature: void(ctx_type *ctx, void *ptr)
//     - the generated _new() function takes a _ZP_LIST_TEMPLATE_ALLOC_CTX_TYPE argument
//   When not defined, the bare two-argument forms below are used instead.
// - _ZP_LIST_TEMPLATE_ALLOC_FN: the function-like macro used to allocate memory.
//   Without context: void*(size_t bytes)  (optional, default is malloc)
//   With context:    void*(ctx_type *ctx, size_t bytes)  (required when ALLOC_CTX_TYPE is set)
// - _ZP_LIST_TEMPLATE_FREE_FN: the function-like macro used to free memory.
//   Without context: void(void *ptr)  (optional, default is free)
//   With context:    void(ctx_type *ctx, void *ptr)  (required when ALLOC_CTX_TYPE is set)

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "zenoh-pico/collections/cat.h"

#ifndef _ZP_LIST_TEMPLATE_ELEM_TYPE
#error "_ZP_LIST_TEMPLATE_ELEM_TYPE must be defined before including list_template.h"
#define _ZP_LIST_TEMPLATE_ELEM_TYPE int
#endif
#ifndef _ZP_LIST_TEMPLATE_NAME
#define _ZP_LIST_TEMPLATE_NAME _ZP_CAT(_ZP_LIST_TEMPLATE_ELEM_TYPE, list)
#endif

#ifndef _ZP_LIST_TEMPLATE_ELEM_DESTROY_FN
#define _ZP_LIST_TEMPLATE_ELEM_DESTROY_FN(x) (void)(x)
#endif
#ifndef _ZP_LIST_TEMPLATE_ELEM_MOVE_FN
#define _ZP_LIST_TEMPLATE_ELEM_MOVE_FN(dst, src) *(dst) = *(src);
#endif

#ifdef _ZP_LIST_TEMPLATE_ALLOC_CTX_TYPE
// Context-aware allocator: user must supply both ALLOC_FN and FREE_FN.
#ifndef _ZP_LIST_TEMPLATE_ALLOC_FN
#error "_ZP_LIST_TEMPLATE_ALLOC_FN must be defined when _ZP_LIST_TEMPLATE_ALLOC_CTX_TYPE is set"
#endif
#ifndef _ZP_LIST_TEMPLATE_FREE_FN
#error "_ZP_LIST_TEMPLATE_FREE_FN must be defined when _ZP_LIST_TEMPLATE_ALLOC_CTX_TYPE is set"
#endif
// Internal wrappers: _alloc(list_ptr, bytes) / _free(list_ptr, ptr)
#define _ZP_LIST_TEMPLATE_INTERNAL_ALLOC(list_ptr, bytes) _ZP_LIST_TEMPLATE_ALLOC_FN(&(list_ptr)->_alloc_ctx, (bytes))
#define _ZP_LIST_TEMPLATE_INTERNAL_FREE(list_ptr, ptr) _ZP_LIST_TEMPLATE_FREE_FN(&(list_ptr)->_alloc_ctx, (ptr))
#define _ZP_LIST_TEMPLATE_HAS_ALLOC_CTX
#else
// Context-free allocator (default).
#ifndef _ZP_LIST_TEMPLATE_ALLOC_FN
#define _ZP_LIST_TEMPLATE_ALLOC_FN(bytes) malloc(bytes)
#endif
#ifndef _ZP_LIST_TEMPLATE_FREE_FN
#define _ZP_LIST_TEMPLATE_FREE_FN(ptr) free(ptr)
#endif
#define _ZP_LIST_TEMPLATE_INTERNAL_ALLOC(list_ptr, bytes) _ZP_LIST_TEMPLATE_ALLOC_FN((bytes))
#define _ZP_LIST_TEMPLATE_INTERNAL_FREE(list_ptr, ptr) _ZP_LIST_TEMPLATE_FREE_FN((ptr))
#endif

// --- Node type ---
#define _ZP_LIST_TEMPLATE_NODE_TYPE _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, node_t)
typedef struct _ZP_LIST_TEMPLATE_NODE_TYPE {
    _ZP_LIST_TEMPLATE_ELEM_TYPE value;
    struct _ZP_LIST_TEMPLATE_NODE_TYPE *_prev;
    struct _ZP_LIST_TEMPLATE_NODE_TYPE *_next;
} _ZP_LIST_TEMPLATE_NODE_TYPE;

// --- List type ---
#define _ZP_LIST_TEMPLATE_TYPE _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, t)
typedef struct _ZP_LIST_TEMPLATE_TYPE {
    _ZP_LIST_TEMPLATE_NODE_TYPE *_head;
    _ZP_LIST_TEMPLATE_NODE_TYPE *_tail;
    size_t _size;
#ifdef _ZP_LIST_TEMPLATE_HAS_ALLOC_CTX
    _ZP_LIST_TEMPLATE_ALLOC_CTX_TYPE _alloc_ctx;
#endif
} _ZP_LIST_TEMPLATE_TYPE;

#ifdef _ZP_LIST_TEMPLATE_HAS_ALLOC_CTX
// Creates a new, empty list with the given allocator context.
static inline _ZP_LIST_TEMPLATE_TYPE _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, new)(_ZP_LIST_TEMPLATE_ALLOC_CTX_TYPE alloc_ctx) {
    _ZP_LIST_TEMPLATE_TYPE list;
    list._head = NULL;
    list._tail = NULL;
    list._size = 0;
    list._alloc_ctx = alloc_ctx;
    return list;
}
#else
// Creates a new, empty list.
static inline _ZP_LIST_TEMPLATE_TYPE _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, new)(void) {
    _ZP_LIST_TEMPLATE_TYPE list;
    list._head = NULL;
    list._tail = NULL;
    list._size = 0;
    return list;
}
#endif

// Returns the number of elements currently stored in the list.
static inline size_t _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, size)(const _ZP_LIST_TEMPLATE_TYPE *list) { return list->_size; }

// Returns true if the list contains no elements.
static inline bool _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, is_empty)(const _ZP_LIST_TEMPLATE_TYPE *list) {
    return list->_size == 0;
}

// Destroys all elements in the list by calling the configured destroy function on each one,
// frees all nodes, and resets the list to an empty state.
// Does not free the list struct itself.
static inline void _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, destroy)(_ZP_LIST_TEMPLATE_TYPE *list) {
    _ZP_LIST_TEMPLATE_NODE_TYPE *node = list->_head;
    while (node != NULL) {
        _ZP_LIST_TEMPLATE_NODE_TYPE *next = node->_next;
        _ZP_LIST_TEMPLATE_ELEM_DESTROY_FN(&node->value);
        _ZP_LIST_TEMPLATE_INTERNAL_FREE(list, node);
        node = next;
    }
    list->_head = NULL;
    list->_tail = NULL;
    list->_size = 0;
}

// Allocates a new node and moves @p elem into it. Returns NULL on allocation failure.
static inline _ZP_LIST_TEMPLATE_NODE_TYPE *_ZP_CAT(_ZP_LIST_TEMPLATE_NAME,
                                                   _alloc_node)(_ZP_LIST_TEMPLATE_TYPE *list,
                                                                _ZP_LIST_TEMPLATE_ELEM_TYPE *elem) {
    _ZP_LIST_TEMPLATE_NODE_TYPE *node =
        (_ZP_LIST_TEMPLATE_NODE_TYPE *)_ZP_LIST_TEMPLATE_INTERNAL_ALLOC(list, sizeof(_ZP_LIST_TEMPLATE_NODE_TYPE));
    if (node == NULL) {
        return NULL;
    }
    _ZP_LIST_TEMPLATE_ELEM_MOVE_FN(&node->value, elem);
    node->_prev = NULL;
    node->_next = NULL;
    return node;
}

// Appends an element to the back of the list by moving it from @p elem.
// Returns true on success, or false if the node allocation failed.
static inline bool _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, push_back)(_ZP_LIST_TEMPLATE_TYPE *list,
                                                              _ZP_LIST_TEMPLATE_ELEM_TYPE *elem) {
    _ZP_LIST_TEMPLATE_NODE_TYPE *node = _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, _alloc_node)(list, elem);
    if (node == NULL) {
        return false;
    }
    node->_prev = list->_tail;
    if (list->_tail != NULL) {
        list->_tail->_next = node;
    } else {
        list->_head = node;
    }
    list->_tail = node;
    list->_size++;
    return true;
}

// Prepends an element to the front of the list by moving it from @p elem.
// Returns true on success, or false if the node allocation failed.
static inline bool _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, push_front)(_ZP_LIST_TEMPLATE_TYPE *list,
                                                               _ZP_LIST_TEMPLATE_ELEM_TYPE *elem) {
    _ZP_LIST_TEMPLATE_NODE_TYPE *node = _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, _alloc_node)(list, elem);
    if (node == NULL) {
        return false;
    }
    node->_next = list->_head;
    if (list->_head != NULL) {
        list->_head->_prev = node;
    } else {
        list->_tail = node;
    }
    list->_head = node;
    list->_size++;
    return true;
}

// Removes the element at the back of the list.
// If @p out is non-NULL the element is moved into it; otherwise it is destroyed in place.
// Returns true on success, or false if the list is empty.
static inline bool _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, pop_back)(_ZP_LIST_TEMPLATE_TYPE *list,
                                                             _ZP_LIST_TEMPLATE_ELEM_TYPE *out) {
    if (_ZP_CAT(_ZP_LIST_TEMPLATE_NAME, is_empty)(list)) {
        return false;
    }
    _ZP_LIST_TEMPLATE_NODE_TYPE *node = list->_tail;
    list->_tail = node->_prev;
    if (list->_tail != NULL) {
        list->_tail->_next = NULL;
    } else {
        list->_head = NULL;
    }
    list->_size--;
    if (out != NULL) {
        _ZP_LIST_TEMPLATE_ELEM_MOVE_FN(out, &node->value);
    } else {
        _ZP_LIST_TEMPLATE_ELEM_DESTROY_FN(&node->value);
    }
    _ZP_LIST_TEMPLATE_INTERNAL_FREE(list, node);
    return true;
}

// Removes the element at the front of the list.
// If @p out is non-NULL the element is moved into it; otherwise it is destroyed in place.
// Returns true on success, or false if the list is empty.
static inline bool _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, pop_front)(_ZP_LIST_TEMPLATE_TYPE *list,
                                                              _ZP_LIST_TEMPLATE_ELEM_TYPE *out) {
    if (_ZP_CAT(_ZP_LIST_TEMPLATE_NAME, is_empty)(list)) {
        return false;
    }
    _ZP_LIST_TEMPLATE_NODE_TYPE *node = list->_head;
    list->_head = node->_next;
    if (list->_head != NULL) {
        list->_head->_prev = NULL;
    } else {
        list->_tail = NULL;
    }
    list->_size--;
    if (out != NULL) {
        _ZP_LIST_TEMPLATE_ELEM_MOVE_FN(out, &node->value);
    } else {
        _ZP_LIST_TEMPLATE_ELEM_DESTROY_FN(&node->value);
    }
    _ZP_LIST_TEMPLATE_INTERNAL_FREE(list, node);
    return true;
}

// Returns a pointer to the element at the back of the list without removing it,
// or NULL if the list is empty.
static inline _ZP_LIST_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_LIST_TEMPLATE_NAME, back)(_ZP_LIST_TEMPLATE_TYPE *list) {
    if (list->_tail == NULL) {
        return NULL;
    }
    return &list->_tail->value;
}

// Returns a pointer to the element at the front of the list without removing it,
// or NULL if the list is empty.
static inline _ZP_LIST_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_LIST_TEMPLATE_NAME, front)(_ZP_LIST_TEMPLATE_TYPE *list) {
    if (list->_head == NULL) {
        return NULL;
    }
    return &list->_head->value;
}

// Returns a const pointer to the element at the back of the list without removing it,
// or NULL if the list is empty.
static inline const _ZP_LIST_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_LIST_TEMPLATE_NAME,
                                                         cback)(const _ZP_LIST_TEMPLATE_TYPE *list) {
    if (list->_tail == NULL) {
        return NULL;
    }
    return &list->_tail->value;
}

// Returns a const pointer to the element at the front of the list without removing it,
// or NULL if the list is empty.
static inline const _ZP_LIST_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_LIST_TEMPLATE_NAME,
                                                         cfront)(const _ZP_LIST_TEMPLATE_TYPE *list) {
    if (list->_head == NULL) {
        return NULL;
    }
    return &list->_head->value;
}

// Inserts an element after @p node by moving it from @p elem.
// If @p node is NULL, the element is prepended to the front.
// Returns true on success, or false if the node allocation failed.
static inline bool _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, insert_after)(_ZP_LIST_TEMPLATE_TYPE *list,
                                                                 _ZP_LIST_TEMPLATE_NODE_TYPE *node,
                                                                 _ZP_LIST_TEMPLATE_ELEM_TYPE *elem) {
    if (node == NULL) {
        return _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, push_front)(list, elem);
    }
    if (node == list->_tail) {
        return _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, push_back)(list, elem);
    }
    _ZP_LIST_TEMPLATE_NODE_TYPE *new_node = _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, _alloc_node)(list, elem);
    if (new_node == NULL) {
        return false;
    }
    new_node->_prev = node;
    new_node->_next = node->_next;
    if (node->_next != NULL) {
        node->_next->_prev = new_node;
    }
    node->_next = new_node;
    list->_size++;
    return true;
}

// Inserts an element before @p node by moving it from @p elem.
// If @p node is NULL, the element is appended to the back.
// Returns true on success, or false if the node allocation failed.
static inline bool _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, insert_before)(_ZP_LIST_TEMPLATE_TYPE *list,
                                                                  _ZP_LIST_TEMPLATE_NODE_TYPE *node,
                                                                  _ZP_LIST_TEMPLATE_ELEM_TYPE *elem) {
    if (node == NULL) {
        return _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, push_back)(list, elem);
    }
    if (node == list->_head) {
        return _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, push_front)(list, elem);
    }
    _ZP_LIST_TEMPLATE_NODE_TYPE *new_node = _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, _alloc_node)(list, elem);
    if (new_node == NULL) {
        return false;
    }
    new_node->_next = node;
    new_node->_prev = node->_prev;
    if (node->_prev != NULL) {
        node->_prev->_next = new_node;
    }
    node->_prev = new_node;
    list->_size++;
    return true;
}

// Removes @p node from the list.
// If @p out is non-NULL the element is moved into it; otherwise it is destroyed in place.
static inline void _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, remove)(_ZP_LIST_TEMPLATE_TYPE *list,
                                                           _ZP_LIST_TEMPLATE_NODE_TYPE *node,
                                                           _ZP_LIST_TEMPLATE_ELEM_TYPE *out) {
    if (node->_prev != NULL) {
        node->_prev->_next = node->_next;
    } else {
        list->_head = node->_next;
    }
    if (node->_next != NULL) {
        node->_next->_prev = node->_prev;
    } else {
        list->_tail = node->_prev;
    }
    list->_size--;
    if (out != NULL) {
        _ZP_LIST_TEMPLATE_ELEM_MOVE_FN(out, &node->value);
    } else {
        _ZP_LIST_TEMPLATE_ELEM_DESTROY_FN(&node->value);
    }
    _ZP_LIST_TEMPLATE_INTERNAL_FREE(list, node);
}

// Returns the head node pointer (for manual iteration). May be NULL if list is empty.
static inline _ZP_LIST_TEMPLATE_NODE_TYPE *_ZP_CAT(_ZP_LIST_TEMPLATE_NAME, head)(_ZP_LIST_TEMPLATE_TYPE *list) {
    return list->_head;
}

// Returns the tail node pointer (for manual iteration). May be NULL if list is empty.
static inline _ZP_LIST_TEMPLATE_NODE_TYPE *_ZP_CAT(_ZP_LIST_TEMPLATE_NAME, tail)(_ZP_LIST_TEMPLATE_TYPE *list) {
    return list->_tail;
}

// Returns the const head node pointer. May be NULL if list is empty.
static inline const _ZP_LIST_TEMPLATE_NODE_TYPE *_ZP_CAT(_ZP_LIST_TEMPLATE_NAME,
                                                         chead)(const _ZP_LIST_TEMPLATE_TYPE *list) {
    return list->_head;
}

// Returns the const tail node pointer. May be NULL if list is empty.
static inline const _ZP_LIST_TEMPLATE_NODE_TYPE *_ZP_CAT(_ZP_LIST_TEMPLATE_NAME,
                                                         ctail)(const _ZP_LIST_TEMPLATE_TYPE *list) {
    return list->_tail;
}

// ---- Iterator ----
// A bidirectional iterator over list nodes — simply a pointer to a node.
// A NULL pointer is the invalid/end sentinel (_ZP_LIST_ITER_INVALID).
// Iterators are invalidated if the pointed-to node is removed from the list.
// Usage (forward):
//   for (<name>_iter_t it = <name>_iter_begin(&list);
//        it != _ZP_LIST_ITER_INVALID;
//        it = <name>_iter_next(it)) { ... &it->value ... }

#ifndef _ZP_LIST_ITER_INVALID
#define _ZP_LIST_ITER_INVALID NULL
#endif

#define _ZP_LIST_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, iter_t)
typedef _ZP_LIST_TEMPLATE_NODE_TYPE *_ZP_LIST_TEMPLATE_ITER_TYPE;

#define _ZP_LIST_TEMPLATE_CITER_TYPE _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, citer_t)
typedef const _ZP_LIST_TEMPLATE_NODE_TYPE *_ZP_LIST_TEMPLATE_CITER_TYPE;

// Returns an iterator pointing to the first element (head), or _ZP_LIST_ITER_INVALID if empty.
static inline _ZP_LIST_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, iter_begin)(_ZP_LIST_TEMPLATE_TYPE *list) {
    return list->_head;
}

// Returns an iterator pointing to the last element (tail), or _ZP_LIST_ITER_INVALID if empty.
static inline _ZP_LIST_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, iter_rbegin)(_ZP_LIST_TEMPLATE_TYPE *list) {
    return list->_tail;
}

// Returns a const iterator pointing to the first element (head), or _ZP_LIST_ITER_INVALID if empty.
static inline _ZP_LIST_TEMPLATE_CITER_TYPE _ZP_CAT(_ZP_LIST_TEMPLATE_NAME,
                                                   iter_cbegin)(const _ZP_LIST_TEMPLATE_TYPE *list) {
    return list->_head;
}

// Returns a const iterator pointing to the last element (tail), or _ZP_LIST_ITER_INVALID if empty.
static inline _ZP_LIST_TEMPLATE_CITER_TYPE _ZP_CAT(_ZP_LIST_TEMPLATE_NAME,
                                                   iter_crbegin)(const _ZP_LIST_TEMPLATE_TYPE *list) {
    return list->_tail;
}

// Advances the iterator to the next element (toward tail). Returns _ZP_LIST_ITER_INVALID when exhausted.
static inline _ZP_LIST_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, iter_next)(_ZP_LIST_TEMPLATE_ITER_TYPE it) {
    return it != _ZP_LIST_ITER_INVALID ? it->_next : _ZP_LIST_ITER_INVALID;
}

// Advances the iterator to the previous element (toward head). Returns _ZP_LIST_ITER_INVALID when exhausted.
static inline _ZP_LIST_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_LIST_TEMPLATE_NAME, iter_prev)(_ZP_LIST_TEMPLATE_ITER_TYPE it) {
    return it != _ZP_LIST_ITER_INVALID ? it->_prev : _ZP_LIST_ITER_INVALID;
}

// Advances the const iterator to the next element (toward tail). Returns _ZP_LIST_ITER_INVALID when exhausted.
static inline _ZP_LIST_TEMPLATE_CITER_TYPE _ZP_CAT(_ZP_LIST_TEMPLATE_NAME,
                                                   iter_cnext)(_ZP_LIST_TEMPLATE_CITER_TYPE it) {
    return it != _ZP_LIST_ITER_INVALID ? it->_next : _ZP_LIST_ITER_INVALID;
}

// Advances the const iterator to the previous element (toward head). Returns _ZP_LIST_ITER_INVALID when exhausted.
static inline _ZP_LIST_TEMPLATE_CITER_TYPE _ZP_CAT(_ZP_LIST_TEMPLATE_NAME,
                                                   iter_cprev)(_ZP_LIST_TEMPLATE_CITER_TYPE it) {
    return it != _ZP_LIST_ITER_INVALID ? it->_prev : _ZP_LIST_ITER_INVALID;
}

#undef _ZP_LIST_TEMPLATE_NODE_TYPE
#undef _ZP_LIST_TEMPLATE_TYPE
#undef _ZP_LIST_TEMPLATE_ELEM_TYPE
#undef _ZP_LIST_TEMPLATE_NAME
#undef _ZP_LIST_TEMPLATE_ELEM_DESTROY_FN
#undef _ZP_LIST_TEMPLATE_ELEM_MOVE_FN
#ifdef _ZP_LIST_TEMPLATE_HAS_ALLOC_CTX
#undef _ZP_LIST_TEMPLATE_ALLOC_CTX_TYPE
#endif
#undef _ZP_LIST_TEMPLATE_HAS_ALLOC_CTX
#undef _ZP_LIST_TEMPLATE_ALLOC_FN
#undef _ZP_LIST_TEMPLATE_FREE_FN
#undef _ZP_LIST_TEMPLATE_INTERNAL_ALLOC
#undef _ZP_LIST_TEMPLATE_INTERNAL_FREE
