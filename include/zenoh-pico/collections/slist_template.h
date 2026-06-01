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

// Generic singly-linked list template.
//
// User needs to define the following macros before including this file:
// - _ZP_SLIST_TEMPLATE_ELEM_TYPE: the type of the elements in the list (required)
// - _ZP_SLIST_TEMPLATE_NAME: the name prefix for generated types and functions,
//   without the trailing _t suffix (optional, default derived from element type)
// - _ZP_SLIST_TEMPLATE_ELEM_DESTROY_FN(p): destroys the element pointed to by p
//   (optional, default is a no-op)
// - _ZP_SLIST_TEMPLATE_ELEM_MOVE_FN(dst, src): moves the element from src into dst
//   (optional, default is plain assignment without destroying the source element)
// - _ZP_SLIST_TEMPLATE_ALLOC_CTX_TYPE: type of the allocator context stored inside
//   the list struct (optional). When defined:
//     - _ZP_SLIST_TEMPLATE_ALLOC_FN must have signature: void*(ctx_type *ctx, size_t bytes)
//     - _ZP_SLIST_TEMPLATE_FREE_FN  must have signature: void(ctx_type *ctx, void *ptr)
//     - the generated _new() function takes a _ZP_SLIST_TEMPLATE_ALLOC_CTX_TYPE argument
//   When not defined, the bare two-argument forms below are used instead.
// - _ZP_SLIST_TEMPLATE_ALLOC_FN: allocator.
//   Without context: void*(size_t bytes)               (optional, default is malloc)
//   With context:    void*(ctx_type *ctx, size_t bytes) (required when ALLOC_CTX_TYPE is set)
// - _ZP_SLIST_TEMPLATE_FREE_FN: deallocator.
//   Without context: void(void *ptr)                   (optional, default is free)
//   With context:    void(ctx_type *ctx, void *ptr)     (required when ALLOC_CTX_TYPE is set)
//
// Generated API (prefix = _ZP_SLIST_TEMPLATE_NAME):
//   <name>_t          — the list struct type
//   <name>node_t      — the node struct type  (value + _next)
//   <name>iter_t      — forward iterator (node pointer; NULL == end)
//   <name>citer_t     — const forward iterator
//
//   <name>_new()                              — create empty list
//   <name>_size(list)                         — element count
//   <name>_is_empty(list)                     — true when size == 0
//   <name>_destroy(list)                      — destroy all elements and free all nodes
//   <name>_front(list)                        — pointer to head element, or NULL
//   <name>_cfront(list)                       — const pointer to head element, or NULL
//   <name>_push_front(list, elem)             — prepend (move), O(1)
//   <name>_pop_front(list, out)               — remove head (move/destroy), O(1)
//   <name>_push_back(list, elem)              — append (move), O(1) via stored tail pointer
//   <name>_insert_after(list, node, elem)     — insert after node (NULL prepends to front)
//   <name>_remove_after(list, prev, out)      — remove node after prev (NULL removes front)
//   <name>_iter_begin(list)                   — iterator to first element
//   <name>_iter_cbegin(list)                  — const iterator to first element
//   <name>_iter_next(it)                      — advance iterator
//   <name>_iter_cnext(it)                     — advance const iterator

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "zenoh-pico/collections/cat.h"

#ifndef _ZP_SLIST_TEMPLATE_ELEM_TYPE
#error "_ZP_SLIST_TEMPLATE_ELEM_TYPE must be defined before including slist_template.h"
#define _ZP_SLIST_TEMPLATE_ELEM_TYPE int
#endif
#ifndef _ZP_SLIST_TEMPLATE_NAME
#define _ZP_SLIST_TEMPLATE_NAME _ZP_CAT(_ZP_SLIST_TEMPLATE_ELEM_TYPE, slist)
#endif

#ifndef _ZP_SLIST_TEMPLATE_ELEM_DESTROY_FN
#define _ZP_SLIST_TEMPLATE_ELEM_DESTROY_FN(x) (void)(x)
#endif
#ifndef _ZP_SLIST_TEMPLATE_ELEM_MOVE_FN
#define _ZP_SLIST_TEMPLATE_ELEM_MOVE_FN(dst, src) *(dst) = *(src)
#endif

#ifdef _ZP_SLIST_TEMPLATE_ALLOC_CTX_TYPE
// Context-aware allocator: user must supply both ALLOC_FN and FREE_FN.
#ifndef _ZP_SLIST_TEMPLATE_ALLOC_FN
#error "_ZP_SLIST_TEMPLATE_ALLOC_FN must be defined when _ZP_SLIST_TEMPLATE_ALLOC_CTX_TYPE is set"
#endif
#ifndef _ZP_SLIST_TEMPLATE_FREE_FN
#error "_ZP_SLIST_TEMPLATE_FREE_FN must be defined when _ZP_SLIST_TEMPLATE_ALLOC_CTX_TYPE is set"
#endif
#define _ZP_SLIST_TEMPLATE_INTERNAL_ALLOC(list_ptr, bytes) _ZP_SLIST_TEMPLATE_ALLOC_FN(&(list_ptr)->_alloc_ctx, (bytes))
#define _ZP_SLIST_TEMPLATE_INTERNAL_FREE(list_ptr, ptr) _ZP_SLIST_TEMPLATE_FREE_FN(&(list_ptr)->_alloc_ctx, (ptr))
#define _ZP_SLIST_TEMPLATE_HAS_ALLOC_CTX
#else
// Context-free allocator (default).
#ifndef _ZP_SLIST_TEMPLATE_ALLOC_FN
#define _ZP_SLIST_TEMPLATE_ALLOC_FN(bytes) malloc(bytes)
#endif
#ifndef _ZP_SLIST_TEMPLATE_FREE_FN
#define _ZP_SLIST_TEMPLATE_FREE_FN(ptr) free(ptr)
#endif
#define _ZP_SLIST_TEMPLATE_INTERNAL_ALLOC(list_ptr, bytes) _ZP_SLIST_TEMPLATE_ALLOC_FN((bytes))
#define _ZP_SLIST_TEMPLATE_INTERNAL_FREE(list_ptr, ptr) _ZP_SLIST_TEMPLATE_FREE_FN((ptr))
#endif

// --- Node type ---
#define _ZP_SLIST_TEMPLATE_NODE_TYPE _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, node_t)
typedef struct _ZP_SLIST_TEMPLATE_NODE_TYPE {
    _ZP_SLIST_TEMPLATE_ELEM_TYPE value;
    struct _ZP_SLIST_TEMPLATE_NODE_TYPE *_next;
} _ZP_SLIST_TEMPLATE_NODE_TYPE;

// --- List type ---
// _tail is a pointer-to-pointer that always points at the _next field where
// the next push_back should write the new node. When the list is empty it
// points at _head; when the list has elements it points at the last node's
// _next field. This gives O(1) push_back without a separate tail pointer.
#define _ZP_SLIST_TEMPLATE_TYPE _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, t)
typedef struct _ZP_SLIST_TEMPLATE_TYPE {
    _ZP_SLIST_TEMPLATE_NODE_TYPE *_head;
    _ZP_SLIST_TEMPLATE_NODE_TYPE **_tail;  // points at the slot for the next node
    size_t _size;
#ifdef _ZP_SLIST_TEMPLATE_HAS_ALLOC_CTX
    _ZP_SLIST_TEMPLATE_ALLOC_CTX_TYPE _alloc_ctx;
#endif
} _ZP_SLIST_TEMPLATE_TYPE;

// ---- Constructors ----

#ifdef _ZP_SLIST_TEMPLATE_HAS_ALLOC_CTX
static inline _ZP_SLIST_TEMPLATE_TYPE _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME,
                                              new)(_ZP_SLIST_TEMPLATE_ALLOC_CTX_TYPE alloc_ctx) {
    _ZP_SLIST_TEMPLATE_TYPE list;
    list._head = NULL;
    list._tail = &list._head;
    list._size = 0;
    list._alloc_ctx = alloc_ctx;
    return list;
}
#else
// Creates a new, empty singly-linked list.
static inline _ZP_SLIST_TEMPLATE_TYPE _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, new)(void) {
    _ZP_SLIST_TEMPLATE_TYPE list;
    list._head = NULL;
    list._tail = &list._head;
    list._size = 0;
    return list;
}
#endif

// ---- Basic queries ----

// Returns the number of elements currently stored in the list.
static inline size_t _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, size)(const _ZP_SLIST_TEMPLATE_TYPE *list) { return list->_size; }

// Returns true if the list contains no elements.
static inline bool _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, is_empty)(const _ZP_SLIST_TEMPLATE_TYPE *list) {
    return list->_size == 0;
}

// ---- Element access ----

// Returns a pointer to the front element, or NULL if the list is empty.
static inline _ZP_SLIST_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, front)(_ZP_SLIST_TEMPLATE_TYPE *list) {
    return list->_head != NULL ? &list->_head->value : NULL;
}

// Returns a const pointer to the front element, or NULL if the list is empty.
static inline const _ZP_SLIST_TEMPLATE_ELEM_TYPE *_ZP_CAT(_ZP_SLIST_TEMPLATE_NAME,
                                                          cfront)(const _ZP_SLIST_TEMPLATE_TYPE *list) {
    return list->_head != NULL ? &list->_head->value : NULL;
}

// ---- Internal helpers ----

// Allocates a new node and moves @p elem into it.  Returns NULL on failure.
static inline _ZP_SLIST_TEMPLATE_NODE_TYPE *_ZP_CAT(_ZP_SLIST_TEMPLATE_NAME,
                                                    _alloc_node)(_ZP_SLIST_TEMPLATE_TYPE *list,
                                                                 _ZP_SLIST_TEMPLATE_ELEM_TYPE *elem) {
    _ZP_SLIST_TEMPLATE_NODE_TYPE *node =
        (_ZP_SLIST_TEMPLATE_NODE_TYPE *)_ZP_SLIST_TEMPLATE_INTERNAL_ALLOC(list, sizeof(_ZP_SLIST_TEMPLATE_NODE_TYPE));
    if (node == NULL) {
        return NULL;
    }
    _ZP_SLIST_TEMPLATE_ELEM_MOVE_FN(&node->value, elem);
    node->_next = NULL;
    return node;
}

// ---- Mutation ----

// Prepends an element to the front of the list by moving it from @p elem.
// Returns true on success, or false if the node allocation failed.
static inline bool _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, push_front)(_ZP_SLIST_TEMPLATE_TYPE *list,
                                                                _ZP_SLIST_TEMPLATE_ELEM_TYPE *elem) {
    _ZP_SLIST_TEMPLATE_NODE_TYPE *node = _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, _alloc_node)(list, elem);
    if (node == NULL) {
        return false;
    }
    node->_next = list->_head;
    list->_head = node;
    // If the list was empty, the tail must now point at the new node's _next
    if (node->_next == NULL) {
        list->_tail = &node->_next;
    }
    list->_size++;
    return true;
}

// Appends an element to the back of the list by moving it from @p elem.
// Returns true on success, or false if the node allocation failed.  O(1).
static inline bool _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, push_back)(_ZP_SLIST_TEMPLATE_TYPE *list,
                                                               _ZP_SLIST_TEMPLATE_ELEM_TYPE *elem) {
    _ZP_SLIST_TEMPLATE_NODE_TYPE *node = _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, _alloc_node)(list, elem);
    if (node == NULL) {
        return false;
    }
    // *_tail is the _next field of the current last node (or _head when empty)
    *list->_tail = node;
    list->_tail = &node->_next;
    list->_size++;
    return true;
}

// Removes the element at the front of the list.
// If @p out is non-NULL the element is moved into it; otherwise it is destroyed in place.
// Returns true on success, or false if the list is empty.
static inline bool _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, pop_front)(_ZP_SLIST_TEMPLATE_TYPE *list,
                                                               _ZP_SLIST_TEMPLATE_ELEM_TYPE *out) {
    if (list->_head == NULL) {
        return false;
    }
    _ZP_SLIST_TEMPLATE_NODE_TYPE *node = list->_head;
    list->_head = node->_next;
    if (list->_head == NULL) {
        // List is now empty: reset tail to point at _head
        list->_tail = &list->_head;
    }
    list->_size--;
    if (out != NULL) {
        _ZP_SLIST_TEMPLATE_ELEM_MOVE_FN(out, &node->value);
    } else {
        _ZP_SLIST_TEMPLATE_ELEM_DESTROY_FN(&node->value);
    }
    _ZP_SLIST_TEMPLATE_INTERNAL_FREE(list, node);
    return true;
}

// Inserts an element after @p node by moving it from @p elem.
// If @p node is NULL, the element is prepended to the front.
// Returns true on success, or false if the node allocation failed.
static inline bool _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, insert_after)(_ZP_SLIST_TEMPLATE_TYPE *list,
                                                                  _ZP_SLIST_TEMPLATE_NODE_TYPE *node,
                                                                  _ZP_SLIST_TEMPLATE_ELEM_TYPE *elem) {
    if (node == NULL) {
        return _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, push_front)(list, elem);
    }
    _ZP_SLIST_TEMPLATE_NODE_TYPE *new_node = _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, _alloc_node)(list, elem);
    if (new_node == NULL) {
        return false;
    }
    new_node->_next = node->_next;
    node->_next = new_node;
    // If we inserted at the back, update the tail pointer
    if (new_node->_next == NULL) {
        list->_tail = &new_node->_next;
    }
    list->_size++;
    return true;
}

// Removes the node immediately after @p prev.
// If @p prev is NULL, the front node is removed.
// If @p out is non-NULL the element is moved into it; otherwise it is destroyed in place.
// Returns true on success, or false if there is no node to remove.
static inline bool _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, remove_after)(_ZP_SLIST_TEMPLATE_TYPE *list,
                                                                  _ZP_SLIST_TEMPLATE_NODE_TYPE *prev,
                                                                  _ZP_SLIST_TEMPLATE_ELEM_TYPE *out) {
    _ZP_SLIST_TEMPLATE_NODE_TYPE *node;
    if (prev == NULL) {
        // Remove front
        if (list->_head == NULL) {
            return false;
        }
        node = list->_head;
        list->_head = node->_next;
        if (list->_head == NULL) {
            list->_tail = &list->_head;
        }
    } else {
        node = prev->_next;
        if (node == NULL) {
            return false;
        }
        prev->_next = node->_next;
        if (prev->_next == NULL) {
            list->_tail = &prev->_next;
        }
    }
    list->_size--;
    if (out != NULL) {
        _ZP_SLIST_TEMPLATE_ELEM_MOVE_FN(out, &node->value);
    } else {
        _ZP_SLIST_TEMPLATE_ELEM_DESTROY_FN(&node->value);
    }
    _ZP_SLIST_TEMPLATE_INTERNAL_FREE(list, node);
    return true;
}

// Destroys all elements in the list by calling the configured destroy function on each one,
// frees all nodes, and resets the list to an empty state.
// Does not free the list struct itself.
static inline void _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, destroy)(_ZP_SLIST_TEMPLATE_TYPE *list) {
    _ZP_SLIST_TEMPLATE_NODE_TYPE *node = list->_head;
    while (node != NULL) {
        _ZP_SLIST_TEMPLATE_NODE_TYPE *next = node->_next;
        _ZP_SLIST_TEMPLATE_ELEM_DESTROY_FN(&node->value);
        _ZP_SLIST_TEMPLATE_INTERNAL_FREE(list, node);
        node = next;
    }
    list->_head = NULL;
    list->_tail = &list->_head;
    list->_size = 0;
}

// ---- Iterator ----
// A forward-only iterator — simply a pointer to a node.
// NULL is the invalid / end sentinel (_ZP_SLIST_ITER_INVALID).
// Usage:
//   for (<name>_iter_t it = <name>_iter_begin(&list);
//        it != _ZP_SLIST_ITER_INVALID;
//        it = <name>_iter_next(it)) { ... it->value ... }

#ifndef _ZP_SLIST_ITER_INVALID
#define _ZP_SLIST_ITER_INVALID NULL
#endif

#define _ZP_SLIST_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, iter_t)
typedef _ZP_SLIST_TEMPLATE_NODE_TYPE *_ZP_SLIST_TEMPLATE_ITER_TYPE;

#define _ZP_SLIST_TEMPLATE_CITER_TYPE _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, citer_t)
typedef const _ZP_SLIST_TEMPLATE_NODE_TYPE *_ZP_SLIST_TEMPLATE_CITER_TYPE;

// Returns an iterator pointing to the first element, or _ZP_SLIST_ITER_INVALID if empty.
static inline _ZP_SLIST_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME, iter_begin)(_ZP_SLIST_TEMPLATE_TYPE *list) {
    return list->_head;
}

// Returns a const iterator pointing to the first element, or _ZP_SLIST_ITER_INVALID if empty.
static inline _ZP_SLIST_TEMPLATE_CITER_TYPE _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME,
                                                    iter_cbegin)(const _ZP_SLIST_TEMPLATE_TYPE *list) {
    return list->_head;
}

// Advances the iterator to the next element. Returns _ZP_SLIST_ITER_INVALID when exhausted.
static inline _ZP_SLIST_TEMPLATE_ITER_TYPE _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME,
                                                   iter_next)(_ZP_SLIST_TEMPLATE_ITER_TYPE it) {
    return it != _ZP_SLIST_ITER_INVALID ? it->_next : _ZP_SLIST_ITER_INVALID;
}

// Advances the const iterator to the next element. Returns _ZP_SLIST_ITER_INVALID when exhausted.
static inline _ZP_SLIST_TEMPLATE_CITER_TYPE _ZP_CAT(_ZP_SLIST_TEMPLATE_NAME,
                                                    iter_cnext)(_ZP_SLIST_TEMPLATE_CITER_TYPE it) {
    return it != _ZP_SLIST_ITER_INVALID ? it->_next : _ZP_SLIST_ITER_INVALID;
}

// ---- Cleanup ----

#undef _ZP_SLIST_TEMPLATE_NODE_TYPE
#undef _ZP_SLIST_TEMPLATE_TYPE
#undef _ZP_SLIST_TEMPLATE_ITER_TYPE
#undef _ZP_SLIST_TEMPLATE_CITER_TYPE
#undef _ZP_SLIST_TEMPLATE_ELEM_TYPE
#undef _ZP_SLIST_TEMPLATE_NAME
#undef _ZP_SLIST_TEMPLATE_ELEM_DESTROY_FN
#undef _ZP_SLIST_TEMPLATE_ELEM_MOVE_FN
#ifdef _ZP_SLIST_TEMPLATE_HAS_ALLOC_CTX
#undef _ZP_SLIST_TEMPLATE_ALLOC_CTX_TYPE
#endif
#undef _ZP_SLIST_TEMPLATE_HAS_ALLOC_CTX
#undef _ZP_SLIST_TEMPLATE_ALLOC_FN
#undef _ZP_SLIST_TEMPLATE_FREE_FN
#undef _ZP_SLIST_TEMPLATE_INTERNAL_ALLOC
#undef _ZP_SLIST_TEMPLATE_INTERNAL_FREE
