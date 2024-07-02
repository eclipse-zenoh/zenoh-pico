//
// Copyright (c) 2024 ZettaScale Technology
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
#ifndef INCLUDE_ZENOH_PICO_API_HANDLERS_H
#define INCLUDE_ZENOH_PICO_API_HANDLERS_H

#include <stdint.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/fifo_mt.h"
#include "zenoh-pico/collections/ring_mt.h"
#include "zenoh-pico/utils/logging.h"

// -- Channel
#define _Z_CHANNEL_DEFINE_IMPL(handler_type, handler_name, handler_new_f_name, callback_type, callback_new_f,        \
                               collection_type, collection_new_f, collection_free_f, collection_push_f,              \
                               collection_pull_f, collection_try_pull_f, elem_owned_type, elem_loaned_type,          \
                               elem_copy_f, elem_drop_f)                                                             \
    typedef struct {                                                                                                 \
        collection_type *collection;                                                                                 \
    } handler_type;                                                                                                  \
                                                                                                                     \
    _Z_OWNED_TYPE_PTR(handler_type, handler_name)                                                                    \
    _Z_LOANED_TYPE(handler_type, handler_name)                                                                       \
                                                                                                                     \
    static inline void _z_##handler_name##_elem_free(void **elem) {                                                  \
        elem_drop_f((elem_owned_type *)*elem);                                                                       \
        z_free(*elem);                                                                                               \
        *elem = NULL;                                                                                                \
    }                                                                                                                \
    static inline void _z_##handler_name##_elem_move(void *dst, void *src) {                                         \
        memcpy(dst, src, sizeof(elem_owned_type));                                                                   \
        z_free(src);                                                                                                 \
    }                                                                                                                \
    static inline void _z_##handler_name##_send(const elem_loaned_type *elem, void *context) {                       \
        elem_owned_type *internal_elem = (elem_owned_type *)z_malloc(sizeof(elem_owned_type));                       \
        if (internal_elem == NULL) {                                                                                 \
            _Z_ERROR("Out of memory");                                                                               \
            return;                                                                                                  \
        }                                                                                                            \
        if (elem == NULL) {                                                                                          \
            internal_elem->_val = NULL;                                                                              \
        } else {                                                                                                     \
            internal_elem->_val = (elem_loaned_type *)z_malloc(sizeof(elem_loaned_type));                            \
            if (internal_elem->_val == NULL) {                                                                       \
                z_free(internal_elem);                                                                               \
                _Z_ERROR("Out of memory");                                                                           \
                return;                                                                                              \
            }                                                                                                        \
            elem_copy_f(internal_elem->_val, (elem_loaned_type *)elem);                                              \
        }                                                                                                            \
        int8_t ret = collection_push_f(internal_elem, context, _z_##handler_name##_elem_free);                       \
        if (ret != _Z_RES_OK) {                                                                                      \
            _Z_ERROR("%s failed: %i", #collection_push_f, ret);                                                      \
        }                                                                                                            \
    }                                                                                                                \
    static inline void z_##handler_name##_recv(const z_loaned_##handler_name##_t *handler, elem_owned_type *elem) {  \
        int8_t ret = collection_pull_f(elem, (collection_type *)handler->collection, _z_##handler_name##_elem_move); \
        if (ret != _Z_RES_OK) {                                                                                      \
            _Z_ERROR("%s failed: %i", #collection_pull_f, ret);                                                      \
        }                                                                                                            \
    }                                                                                                                \
    static inline void z_##handler_name##_try_recv(const z_loaned_##handler_name##_t *handler,                       \
                                                   elem_owned_type *elem) {                                          \
        int8_t ret =                                                                                                 \
            collection_try_pull_f(elem, (collection_type *)handler->collection, _z_##handler_name##_elem_move);      \
        if (ret != _Z_RES_OK) {                                                                                      \
            _Z_ERROR("%s failed: %i", #collection_try_pull_f, ret);                                                  \
        }                                                                                                            \
    }                                                                                                                \
                                                                                                                     \
    static inline void _z_##handler_name##_free(handler_type **handler) {                                            \
        handler_type *ptr = *handler;                                                                                \
        if (ptr != NULL) {                                                                                           \
            collection_free_f(ptr->collection, _z_##handler_name##_elem_free);                                       \
            z_free(ptr);                                                                                             \
            *handler = NULL;                                                                                         \
        }                                                                                                            \
    }                                                                                                                \
    static inline void _z_##handler_name##_copy(void *dst, const void *src) {                                        \
        (void)(dst);                                                                                                 \
        (void)(src);                                                                                                 \
    }                                                                                                                \
                                                                                                                     \
    _Z_OWNED_FUNCTIONS_PTR_IMPL(handler_type, handler_name, _z_##handler_name##_copy, _z_##handler_name##_free)      \
                                                                                                                     \
    static inline int8_t handler_new_f_name(callback_type *callback, z_owned_##handler_name##_t *handler,            \
                                            size_t capacity) {                                                       \
        handler->_val = (handler_type *)z_malloc(sizeof(handler_type));                                              \
        handler->_val->collection = collection_new_f(capacity);                                                      \
        callback_new_f(callback, _z_##handler_name##_send, NULL, handler->_val->collection);                         \
        return _Z_RES_OK;                                                                                            \
    }

#define _Z_CHANNEL_DEFINE(item_name, kind_name)                                                             \
    _Z_CHANNEL_DEFINE_IMPL(/* handler_type                    */ _z_##kind_name##_handler_##item_name##_t,  \
                           /* handler_name                    */ kind_name##_handler_##item_name,           \
                           /* handler_new_f_name              */ z_##kind_name##_channel_##item_name##_new, \
                           /* callback_type                   */ z_owned_closure_##item_name##_t,           \
                           /* callback_new_f                  */ z_closure_##item_name,                     \
                           /* collection_type                 */ _z_##kind_name##_mt_t,                     \
                           /* collection_new_f                */ _z_##kind_name##_mt_new,                   \
                           /* collection_free_f               */ _z_##kind_name##_mt_free,                  \
                           /* collection_push_f               */ _z_##kind_name##_mt_push,                  \
                           /* collection_pull_f               */ _z_##kind_name##_mt_pull,                  \
                           /* collection_try_pull_f           */ _z_##kind_name##_mt_try_pull,              \
                           /* elem_owned_type                 */ z_owned_##item_name##_t,                   \
                           /* elem_loaned_type                */ z_loaned_##item_name##_t,                  \
                           /* elem_copy_f                     */ _z_##item_name##_copy,                     \
                           /* elem_drop_f                     */ z_##item_name##_drop)

#define _Z_CHANNEL_RC_DEFINE_IMPL(handler_type, handler_name, handler_new_f_name, callback_type, callback_new_f,     \
                                  collection_type, collection_new_f, collection_free_f, collection_push_f,           \
                                  collection_pull_f, collection_try_pull_f, elem_owned_type, elem_loaned_type,       \
                                  elem_copy_f, elem_drop_f)                                                          \
    typedef struct {                                                                                                 \
        collection_type *collection;                                                                                 \
    } handler_type;                                                                                                  \
                                                                                                                     \
    _Z_OWNED_TYPE_PTR(handler_type, handler_name)                                                                    \
    _Z_LOANED_TYPE(handler_type, handler_name)                                                                       \
                                                                                                                     \
    static inline void _z_##handler_name##_elem_free(void **elem) {                                                  \
        elem_drop_f((elem_owned_type *)*elem);                                                                       \
        z_free(*elem);                                                                                               \
        *elem = NULL;                                                                                                \
    }                                                                                                                \
    static inline void _z_##handler_name##_elem_move(void *dst, void *src) {                                         \
        memcpy(dst, src, sizeof(elem_owned_type));                                                                   \
        z_free(src);                                                                                                 \
    }                                                                                                                \
    static inline void _z_##handler_name##_send(const elem_loaned_type *elem, void *context) {                       \
        elem_owned_type *internal_elem = (elem_owned_type *)z_malloc(sizeof(elem_owned_type));                       \
        if (internal_elem == NULL) {                                                                                 \
            _Z_ERROR("Out of memory");                                                                               \
            return;                                                                                                  \
        }                                                                                                            \
        if (elem == NULL) {                                                                                          \
            internal_elem->_rc.in = NULL;                                                                            \
        } else {                                                                                                     \
            elem_copy_f(&internal_elem->_rc, elem);                                                                  \
        }                                                                                                            \
        int8_t ret = collection_push_f(internal_elem, context, _z_##handler_name##_elem_free);                       \
        if (ret != _Z_RES_OK) {                                                                                      \
            _Z_ERROR("%s failed: %i", #collection_push_f, ret);                                                      \
        }                                                                                                            \
    }                                                                                                                \
    static inline void z_##handler_name##_recv(const z_loaned_##handler_name##_t *handler, elem_owned_type *elem) {  \
        int8_t ret = collection_pull_f(elem, (collection_type *)handler->collection, _z_##handler_name##_elem_move); \
        if (ret != _Z_RES_OK) {                                                                                      \
            _Z_ERROR("%s failed: %i", #collection_pull_f, ret);                                                      \
        }                                                                                                            \
    }                                                                                                                \
    static inline void z_##handler_name##_try_recv(const z_loaned_##handler_name##_t *handler,                       \
                                                   elem_owned_type *elem) {                                          \
        int8_t ret =                                                                                                 \
            collection_try_pull_f(elem, (collection_type *)handler->collection, _z_##handler_name##_elem_move);      \
        if (ret != _Z_RES_OK) {                                                                                      \
            _Z_ERROR("%s failed: %i", #collection_try_pull_f, ret);                                                  \
        }                                                                                                            \
    }                                                                                                                \
                                                                                                                     \
    static inline void _z_##handler_name##_free(handler_type **handler) {                                            \
        handler_type *ptr = *handler;                                                                                \
        if (ptr != NULL) {                                                                                           \
            collection_free_f(ptr->collection, _z_##handler_name##_elem_free);                                       \
            z_free(ptr);                                                                                             \
            *handler = NULL;                                                                                         \
        }                                                                                                            \
    }                                                                                                                \
    static inline void _z_##handler_name##_copy(void *dst, const void *src) {                                        \
        (void)(dst);                                                                                                 \
        (void)(src);                                                                                                 \
    }                                                                                                                \
                                                                                                                     \
    _Z_OWNED_FUNCTIONS_PTR_IMPL(handler_type, handler_name, _z_##handler_name##_copy, _z_##handler_name##_free)      \
                                                                                                                     \
    static inline int8_t handler_new_f_name(callback_type *callback, z_owned_##handler_name##_t *handler,            \
                                            size_t capacity) {                                                       \
        handler->_val = (handler_type *)z_malloc(sizeof(handler_type));                                              \
        handler->_val->collection = collection_new_f(capacity);                                                      \
        callback_new_f(callback, _z_##handler_name##_send, NULL, handler->_val->collection);                         \
        return _Z_RES_OK;                                                                                            \
    }

#define _Z_CHANNEL_RC_DEFINE(item_name, kind_name)                                                             \
    _Z_CHANNEL_RC_DEFINE_IMPL(/* handler_type                    */ _z_##kind_name##_handler_##item_name##_t,  \
                              /* handler_name                    */ kind_name##_handler_##item_name,           \
                              /* handler_new_f_name              */ z_##kind_name##_channel_##item_name##_new, \
                              /* callback_type                   */ z_owned_closure_##item_name##_t,           \
                              /* callback_new_f                  */ z_closure_##item_name,                     \
                              /* collection_type                 */ _z_##kind_name##_mt_t,                     \
                              /* collection_new_f                */ _z_##kind_name##_mt_new,                   \
                              /* collection_free_f               */ _z_##kind_name##_mt_free,                  \
                              /* collection_push_f               */ _z_##kind_name##_mt_push,                  \
                              /* collection_pull_f               */ _z_##kind_name##_mt_pull,                  \
                              /* collection_try_pull_f           */ _z_##kind_name##_mt_try_pull,              \
                              /* elem_owned_type                 */ z_owned_##item_name##_t,                   \
                              /* elem_loaned_type                */ z_loaned_##item_name##_t,                  \
                              /* elem_copy_f                     */ _z_##item_name##_rc_copy,                  \
                              /* elem_drop_f                     */ z_##item_name##_drop)

#define _Z_CHANNEL_DEFINE_DUMMY(item_name, kind_name)       \
    typedef struct {                                        \
        uint8_t _foo;                                       \
    } z_owned_##kind_name##_handler_##item_name##_t;        \
    typedef struct {                                        \
        uint8_t _foo;                                       \
    } z_loaned_##kind_name##_handler_##item_name##_t;       \
    void *z_##kind_name##_handler_##item_name##_loan(void); \
    void *z_##kind_name##_handler_##item_name##_move(void); \
    void *z_##kind_name##_handler_##item_name##_drop(void); \
    void *z_##kind_name##_handler_##item_name##_recv(void); \
    void *z_##kind_name##_handler_##item_name##_try_recv(void);

// This macro defines:
//   z_ring_channel_sample_new()
//   z_owned_ring_handler_sample_t/z_loaned_ring_handler_sample_t
_Z_CHANNEL_DEFINE(sample, ring)

// This macro defines:
//   z_fifo_channel_sample_new()
//   z_owned_fifo_handler_sample_t/z_loaned_fifo_handler_sample_t
_Z_CHANNEL_DEFINE(sample, fifo)

#if Z_FEATURE_QUERYABLE == 1
// This macro defines:
//   z_ring_channel_query_new()
//   z_owned_ring_handler_query_t/z_loaned_ring_handler_query_t
_Z_CHANNEL_RC_DEFINE(query, ring)

// This macro defines:
//   z_fifo_channel_query_new()
//   z_owned_fifo_handler_query_t/z_loaned_fifo_handler_query_t
_Z_CHANNEL_RC_DEFINE(query, fifo)
#else   // Z_FEATURE_QUERYABLE
_Z_CHANNEL_DEFINE_DUMMY(query, ring)
_Z_CHANNEL_DEFINE_DUMMY(query, fifo)
#endif  // Z_FEATURE_QUERYABLE

#if Z_FEATURE_QUERY == 1
// This macro defines:
//   z_ring_channel_reply_new()
//   z_owned_ring_handler_reply_t/z_loaned_ring_handler_reply_t
_Z_CHANNEL_DEFINE(reply, ring)

// This macro defines:
//   z_fifo_channel_reply_new()
//   z_owned_fifo_handler_reply_t/z_loaned_fifo_handler_reply_t
_Z_CHANNEL_DEFINE(reply, fifo)
#else   // Z_FEATURE_QUERY
_Z_CHANNEL_DEFINE_DUMMY(reply, ring)
_Z_CHANNEL_DEFINE_DUMMY(reply, fifo)
#endif  // Z_FEATURE_QUERY

#endif  // INCLUDE_ZENOH_PICO_API_HANDLERS_H
