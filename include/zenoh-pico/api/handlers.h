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

#ifdef __cplusplus
extern "C" {
#endif
// -- Channel
#define _Z_CHANNEL_DEFINE_IMPL(handler_type, handler_name, handler_new_f_name, callback_type, callback_new_f,          \
                               collection_type, collection_new_f, collection_free_f, collection_push_f,                \
                               collection_pull_f, collection_try_pull_f, collection_close_f, elem_owned_type,          \
                               elem_loaned_type, elem_clone_f, elem_drop_f, elem_null_f)                               \
    typedef struct {                                                                                                   \
        collection_type *collection;                                                                                   \
    } handler_type;                                                                                                    \
                                                                                                                       \
    _Z_OWNED_TYPE_VALUE(handler_type, handler_name)                                                                    \
    _Z_LOANED_TYPE(handler_type, handler_name)                                                                         \
                                                                                                                       \
    static inline void _z_##handler_name##_elem_free(void **elem) {                                                    \
        elem_drop_f((elem_owned_type *)*elem);                                                                         \
        z_free(*elem);                                                                                                 \
        *elem = NULL;                                                                                                  \
    }                                                                                                                  \
    static inline void _z_##handler_name##_elem_move(void *dst, void *src) {                                           \
        memcpy(dst, src, sizeof(elem_owned_type));                                                                     \
        z_free(src);                                                                                                   \
    }                                                                                                                  \
    static inline void _z_##handler_name##_close(void *context) {                                                      \
        int8_t ret = collection_close_f((collection_type *)context);                                                   \
        if (ret < 0) {                                                                                                 \
            _Z_ERROR("%s failed: %i", #collection_push_f, ret);                                                        \
        }                                                                                                              \
    }                                                                                                                  \
    static inline void _z_##handler_name##_send(const elem_loaned_type *elem, void *context) {                         \
        elem_owned_type *internal_elem = (elem_owned_type *)z_malloc(sizeof(elem_owned_type));                         \
        if (internal_elem == NULL) {                                                                                   \
            _Z_ERROR("Out of memory");                                                                                 \
            return;                                                                                                    \
        }                                                                                                              \
        elem_clone_f(internal_elem, elem);                                                                             \
        int8_t ret = collection_push_f(internal_elem, context, _z_##handler_name##_elem_free);                         \
        if (ret != _Z_RES_OK) {                                                                                        \
            _Z_ERROR("%s failed: %i", #collection_push_f, ret);                                                        \
        }                                                                                                              \
    }                                                                                                                  \
    static inline _Bool z_##handler_name##_recv(const z_loaned_##handler_name##_t *handler, elem_owned_type *elem) {   \
        elem_null_f(elem);                                                                                             \
        int8_t ret = collection_pull_f(elem, (collection_type *)handler->collection, _z_##handler_name##_elem_move);   \
        if (ret == _Z_RES_CHANNEL_CLOSED) {                                                                            \
            return false;                                                                                              \
        }                                                                                                              \
        if (ret != _Z_RES_OK) {                                                                                        \
            _Z_ERROR("%s failed: %i", #collection_pull_f, ret);                                                        \
            return false;                                                                                              \
        }                                                                                                              \
        return true;                                                                                                   \
    }                                                                                                                  \
    static inline _Bool z_##handler_name##_try_recv(const z_loaned_##handler_name##_t *handler,                        \
                                                    elem_owned_type *elem) {                                           \
        elem_null_f(elem);                                                                                             \
        int8_t ret =                                                                                                   \
            collection_try_pull_f(elem, (collection_type *)handler->collection, _z_##handler_name##_elem_move);        \
        if (ret == _Z_RES_CHANNEL_CLOSED) {                                                                            \
            return false;                                                                                              \
        }                                                                                                              \
        if (ret != _Z_RES_OK) {                                                                                        \
            _Z_ERROR("%s failed: %i", #collection_try_pull_f, ret);                                                    \
            return false;                                                                                              \
        }                                                                                                              \
        return true;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    static inline void _z_##handler_name##_clear(handler_type *handler) {                                              \
        if (handler != NULL && handler->collection != NULL) {                                                          \
            collection_free_f(handler->collection, _z_##handler_name##_elem_free);                                     \
            handler->collection = NULL;                                                                                \
        }                                                                                                              \
    }                                                                                                                  \
    static inline _Bool _z_##handler_name##_check(const handler_type *handler) { return handler->collection == NULL; } \
    static inline handler_type _z_##handler_name##_null(void) {                                                        \
        handler_type h;                                                                                                \
        h.collection = NULL;                                                                                           \
        return h;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    _Z_OWNED_FUNCTIONS_VALUE_NO_COPY_INLINE_IMPL(handler_type, handler_name, _z_##handler_name##_check,                \
                                                 _z_##handler_name##_null, _z_##handler_name##_clear)                  \
                                                                                                                       \
    static inline int8_t handler_new_f_name(callback_type *callback, z_owned_##handler_name##_t *handler,              \
                                            size_t capacity) {                                                         \
        handler->_val.collection = collection_new_f(capacity);                                                         \
        if (handler->_val.collection == NULL) {                                                                        \
            return _Z_ERR_SYSTEM_OUT_OF_MEMORY;                                                                        \
        }                                                                                                              \
        callback_new_f(callback, _z_##handler_name##_send, _z_##handler_name##_close, handler->_val.collection);       \
        return _Z_RES_OK;                                                                                              \
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
                           /* collection_close_f              */ _z_##kind_name##_mt_close,                 \
                           /* elem_owned_type                 */ z_owned_##item_name##_t,                   \
                           /* elem_loaned_type                */ z_loaned_##item_name##_t,                  \
                           /* elem_clone_f                     */ z_##item_name##_clone,                    \
                           /* elem_drop_f                     */ z_##item_name##_drop,                      \
                           /* elem_null                       */ z_##item_name##_null)

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
_Z_CHANNEL_DEFINE(query, ring)

// This macro defines:
//   z_fifo_channel_query_new()
//   z_owned_fifo_handler_query_t/z_loaned_fifo_handler_query_t
_Z_CHANNEL_DEFINE(query, fifo)
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

#ifdef __cplusplus
}
#endif

#endif  // INCLUDE_ZENOH_PICO_API_HANDLERS_H
