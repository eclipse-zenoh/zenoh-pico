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
#ifndef INCLUDE_ZENOH_PICO_COLLECTIONS_ALGORITHMS_TEMPLATE_H
#define INCLUDE_ZENOH_PICO_COLLECTIONS_ALGORITHMS_TEMPLATE_H

#include <stdbool.h>

#define _ZP_TRANSFORM_IDENTITY(x) (x)
#define _ZP_TRANSFORM_VAL(x) (&(x)->val)

#define _ZP_FOREACH_TRANSFORM(collection_name, collection_ptr, var_name, transform)        \
    for (collection_name##_iter_t __iter = collection_name##_begin(collection_ptr);        \
         __iter == collection_name##_end(collection_ptr)                                   \
             ? false                                                                       \
             : (var_name = transform(collection_name##_at(collection_ptr, __iter)), true); \
         __iter = collection_name##_iter_next(collection_ptr, __iter))

#define _ZP_CFOREACH_TRANSFORM(collection_name, collection_ptr, var_name, transform)             \
    for (collection_name##_iter_t __iter = collection_name##_begin(collection_ptr);              \
         __iter == collection_name##_end(collection_ptr)                                         \
             ? false                                                                             \
             : (var_name = transform(collection_name##_const_at(collection_ptr, __iter)), true); \
         __iter = collection_name##_iter_next(collection_ptr, __iter))

#define _ZP_FIND_TRANSFORM(collection_name, collection_ptr, var_name, predicate, transform) \
    var_name = NULL;                                                                        \
    for (collection_name##_iter_t __iter = collection_name##_begin(collection_ptr);         \
         __iter != collection_name##_end(collection_ptr);                                   \
         __iter = collection_name##_iter_next(collection_ptr, __iter)) {                    \
        collection_name##_elem_t *node = collection_name##_at(collection_ptr, __iter);      \
        if (predicate(transform(node))) {                                                   \
            var_name = transform(node);                                                     \
            break;                                                                          \
        }                                                                                   \
    }

#define _ZP_CFIND_TRANSFORM(collection_name, collection_ptr, var_name, predicate, transform)       \
    var_name = NULL;                                                                               \
    for (collection_name##_iter_t __iter = collection_name##_begin(collection_ptr);                \
         __iter != collection_name##_end(collection_ptr);                                          \
         __iter = collection_name##_iter_next(collection_ptr, __iter)) {                           \
        const collection_name##_elem_t *node = collection_name##_const_at(collection_ptr, __iter); \
        if (predicate(transform(node))) {                                                          \
            var_name = transform(node);                                                            \
            break;                                                                                 \
        }                                                                                          \
    }

// For loop over collection elements.  var_name is a pointer to the element type which should be declared by user before
// the loop.
#define _ZP_FOREACH(collection_name, collection_ptr, var_name) \
    _ZP_FOREACH_TRANSFORM(collection_name, collection_ptr, var_name, _ZP_TRANSFORM_IDENTITY)
// For loop over const collection elements.  var_name is a pointer to the element type which should be declared by user
// before the loop.
#define _ZP_CFOREACH(collection_name, collection_ptr, var_name) \
    _ZP_CFOREACH_TRANSFORM(collection_name, collection_ptr, var_name, _ZP_TRANSFORM_IDENTITY)

// Find first const element matching predicate.  var_name is a pointer to the element type which should be declared by
// user before the loop.  It is set to NULL if no matching element is found.
#define _ZP_FIND(collection_name, collection_ptr, var_name, predicate) \
    _ZP_FIND_TRANSFORM(collection_name, collection_ptr, var_name, predicate, _ZP_TRANSFORM_IDENTITY)

// Find first const element matching predicate.  var_name is a pointer to the element type which should be declared by
// user before the loop.  It is set to NULL if no matching element is found.
#define _ZP_CFIND(collection_name, collection_ptr, var_name, predicate) \
    _ZP_CFIND_TRANSFORM(collection_name, collection_ptr, var_name, predicate, _ZP_TRANSFORM_IDENTITY)

// For loop over hashmap values.  var_name is a pointer to the value type which should be declared by user before the
// loop.
#define _ZP_FOREACH_VAL(collection_name, collection_ptr, var_name) \
    _ZP_FOREACH_TRANSFORM(collection_name, collection_ptr, var_name, _ZP_TRANSFORM_VAL)
// For loop over const hashmap values.  var_name is a pointer to the value type which should be declared by user before
// the loop.
#define _ZP_CFOREACH_VAL(collection_name, collection_ptr, var_name) \
    _ZP_CFOREACH_TRANSFORM(collection_name, collection_ptr, var_name, _ZP_TRANSFORM_VAL)

// Find first hashmap value matching predicate.  var_name is a pointer to the value type which should be declared by
// user before the loop.  It is set to NULL if no matching element is found.
#define _ZP_FIND_VAL(collection_name, collection_ptr, var_name, predicate) \
    _ZP_FIND_TRANSFORM(collection_name, collection_ptr, var_name, predicate, _ZP_TRANSFORM_VAL)

// Find first const hashmap value matching predicate.  var_name is a pointer to the value type which should be declared
// by user before the loop.  It is set to NULL if no matching element is found.
#define _ZP_CFIND_VAL(collection_name, collection_ptr, var_name, predicate) \
    _ZP_CFIND_TRANSFORM(collection_name, collection_ptr, var_name, predicate, _ZP_TRANSFORM_VAL)

// Remove all elements matching predicate.  Behaviour is undefined if predicate has side effects that modify the
// collection.
#define _ZP_REMOVE(collection_name, collection_ptr, predicate)                                     \
    for (collection_name##_iter_t __iter = collection_name##_begin(collection_ptr);                \
         __iter != collection_name##_end(collection_ptr);) {                                       \
        const collection_name##_elem_t *node = collection_name##_const_at(collection_ptr, __iter); \
        if (predicate(node)) {                                                                     \
            collection_name##_remove_at(collection_ptr, __iter, NULL, &__iter);                    \
        } else {                                                                                   \
            __iter = collection_name##_iter_next(collection_ptr, __iter);                          \
        }                                                                                          \
    }
#endif