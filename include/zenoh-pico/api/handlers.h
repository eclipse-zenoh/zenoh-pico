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

#include "zenoh-pico/api/macros.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/fifo_mt.h"
#include "zenoh-pico/collections/ring_mt.h"
#include "zenoh-pico/utils/logging.h"

// -- Samples handler
void _z_owned_sample_move(z_owned_sample_t *dst, z_owned_sample_t *src);
z_owned_sample_t *_z_sample_to_owned_ptr(const _z_sample_t *src);

// -- Queries handler
void _z_owned_query_move(z_owned_query_t *dst, z_owned_query_t *src);
z_owned_query_t *_z_query_to_owned_ptr(const z_query_t *src);

// -- Reply handler
void _z_owned_reply_move(z_owned_reply_t *dst, z_owned_reply_t *src);
z_owned_reply_t *_z_reply_clone(const z_owned_reply_t *src);

// -- Channel
#define _Z_CHANNEL_DEFINE(name, send_closure_name, recv_closure_name, send_type, recv_type, collection_type, \
                          collection_new_f, collection_free_f, collection_push_f, collection_pull_f,         \
                          collection_try_pull_f, elem_move_f, elem_convert_f, elem_drop_f)                   \
    typedef struct {                                                                                         \
        z_owned_##send_closure_name##_t send;                                                                \
        z_owned_##recv_closure_name##_t recv;                                                                \
        z_owned_##recv_closure_name##_t try_recv;                                                            \
        collection_type *collection;                                                                         \
    } z_owned_##name##_t;                                                                                    \
                                                                                                             \
    static inline void _z_##name##_elem_free(void **elem) {                                                  \
        elem_drop_f((recv_type *)*elem);                                                                     \
        zp_free(*elem);                                                                                      \
        *elem = NULL;                                                                                        \
    }                                                                                                        \
    static inline void _z_##name##_elem_move(void *dst, void *src) {                                         \
        elem_move_f((recv_type *)dst, (recv_type *)src);                                                     \
    }                                                                                                        \
    static inline void _z_##name##_send(send_type *elem, void *context) {                                    \
        void *internal_elem = elem_convert_f(elem);                                                          \
        if (internal_elem == NULL) {                                                                         \
            return;                                                                                          \
        }                                                                                                    \
        int8_t ret = collection_push_f(internal_elem, context, _z_##name##_elem_free);                       \
        if (ret != _Z_RES_OK) {                                                                              \
            _Z_ERROR("%s failed: %i", #collection_push_f, ret);                                              \
        }                                                                                                    \
    }                                                                                                        \
    static inline void _z_##name##_recv(recv_type *elem, void *context) {                                    \
        int8_t ret = collection_pull_f(elem, context, _z_##name##_elem_move);                                \
        if (ret != _Z_RES_OK) {                                                                              \
            _Z_ERROR("%s failed: %i", #collection_pull_f, ret);                                              \
        }                                                                                                    \
    }                                                                                                        \
    static inline void _z_##name##_try_recv(recv_type *elem, void *context) {                                \
        int8_t ret = collection_try_pull_f(elem, context, _z_##name##_elem_move);                            \
        if (ret != _Z_RES_OK) {                                                                              \
            _Z_ERROR("%s failed: %i", #collection_try_pull_f, ret);                                          \
        }                                                                                                    \
    }                                                                                                        \
                                                                                                             \
    static inline z_owned_##name##_t z_##name##_new(size_t capacity) {                                       \
        z_owned_##name##_t channel;                                                                          \
        channel.collection = collection_new_f(capacity);                                                     \
        channel.send = z_##send_closure_name(_z_##name##_send, NULL, channel.collection);                    \
        channel.recv = z_##recv_closure_name(_z_##name##_recv, NULL, channel.collection);                    \
        channel.try_recv = z_##recv_closure_name(_z_##name##_try_recv, NULL, channel.collection);            \
        return channel;                                                                                      \
    }                                                                                                        \
    static inline z_owned_##name##_t *z_##name##_move(z_owned_##name##_t *val) { return val; }               \
    static inline void z_##name##_drop(z_owned_##name##_t *channel) {                                        \
        collection_free_f(channel->collection, _z_##name##_elem_free);                                       \
        z_##send_closure_name##_drop(&channel->send);                                                        \
        z_##recv_closure_name##_drop(&channel->recv);                                                        \
    }

// z_owned_sample_ring_channel_t
_Z_CHANNEL_DEFINE(sample_ring_channel, closure_sample, closure_owned_sample, const z_sample_t, z_owned_sample_t,
                  _z_ring_mt_t, _z_ring_mt_new, _z_ring_mt_free, _z_ring_mt_push, _z_ring_mt_pull, _z_ring_mt_try_pull,
                  _z_owned_sample_move, _z_sample_to_owned_ptr, z_sample_drop)

// z_owned_sample_fifo_channel_t
_Z_CHANNEL_DEFINE(sample_fifo_channel, closure_sample, closure_owned_sample, const z_sample_t, z_owned_sample_t,
                  _z_fifo_mt_t, _z_fifo_mt_new, _z_fifo_mt_free, _z_fifo_mt_push, _z_fifo_mt_pull, _z_fifo_mt_try_pull,
                  _z_owned_sample_move, _z_sample_to_owned_ptr, z_sample_drop)

// z_owned_query_ring_channel_t
_Z_CHANNEL_DEFINE(query_ring_channel, closure_query, closure_owned_query, const z_query_t, z_owned_query_t,
                  _z_ring_mt_t, _z_ring_mt_new, _z_ring_mt_free, _z_ring_mt_push, _z_ring_mt_pull, _z_ring_mt_try_pull,
                  _z_owned_query_move, _z_query_to_owned_ptr, z_query_drop)

// z_owned_query_fifo_channel_t
_Z_CHANNEL_DEFINE(query_fifo_channel, closure_query, closure_owned_query, const z_query_t, z_owned_query_t,
                  _z_fifo_mt_t, _z_fifo_mt_new, _z_fifo_mt_free, _z_fifo_mt_push, _z_fifo_mt_pull, _z_fifo_mt_try_pull,
                  _z_owned_query_move, _z_query_to_owned_ptr, z_query_drop)

// z_owned_reply_ring_channel_t
_Z_CHANNEL_DEFINE(reply_ring_channel, closure_reply, closure_reply, z_owned_reply_t, z_owned_reply_t, _z_ring_mt_t,
                  _z_ring_mt_new, _z_ring_mt_free, _z_ring_mt_push, _z_ring_mt_pull, _z_ring_mt_try_pull,
                  _z_owned_reply_move, _z_reply_clone, z_reply_drop)

// z_owned_reply_fifo_channel_t
_Z_CHANNEL_DEFINE(reply_fifo_channel, closure_reply, closure_reply, z_owned_reply_t, z_owned_reply_t, _z_fifo_mt_t,
                  _z_fifo_mt_new, _z_fifo_mt_free, _z_fifo_mt_push, _z_fifo_mt_pull, _z_fifo_mt_try_pull,
                  _z_owned_reply_move, _z_reply_clone, z_reply_drop)

#endif  // INCLUDE_ZENOH_PICO_API_HANDLERS_H
