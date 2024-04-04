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
void _z_owned_sample_move(z_owned_sample_t *dst, const z_owned_sample_t *src);
z_owned_sample_t *_z_sample_to_owned_ptr(const _z_sample_t *src);

// -- Channel
#define _Z_CHANNEL_DEFINE(name, send_closure_name, recv_closure_name, send_type, recv_type, collection_type,      \
                          collection_new_f, collection_free_f, collection_push_f, collection_pull_f, elem_move_f, \
                          elem_convert_f, elem_free_f)                                                            \
    typedef struct {                                                                                              \
        z_owned_##send_closure_name##_t send;                                                                     \
        z_owned_##recv_closure_name##_t recv;                                                                     \
        collection_type *collection;                                                                              \
    } z_owned_##name##_t;                                                                                         \
                                                                                                                  \
    static inline void _z_##name##_elem_free(void **elem) {                                                       \
        elem_free_f((recv_type *)*elem);                                                                          \
        *elem = NULL;                                                                                             \
    }                                                                                                             \
    static inline void _z_##name##_elem_move(void *dst, const void *src) {                                        \
        elem_move_f((recv_type *)dst, (const recv_type *)src);                                                    \
    }                                                                                                             \
    static inline void _z_##name##_push(const send_type *elem, void *context) {                                   \
        void *internal_elem = elem_convert_f(elem);                                                               \
        if (internal_elem == NULL) {                                                                              \
            return;                                                                                               \
        }                                                                                                         \
        int8_t res = collection_push_f(internal_elem, context, _z_##name##_elem_free);                            \
        if (res) {                                                                                                \
            _Z_ERROR("%s failed: %i", #collection_push_f, res);                                                   \
        }                                                                                                         \
    }                                                                                                             \
    static inline void _z_##name##_pull(recv_type *elem, void *context) {                                         \
        int8_t res = collection_pull_f(elem, context, _z_##name##_elem_move);                                     \
        if (res) {                                                                                                \
            _Z_ERROR("%s failed: %i", #collection_pull_f, res);                                                   \
        }                                                                                                         \
    }                                                                                                             \
                                                                                                                  \
    static inline z_owned_##name##_t z_##name(size_t capacity) {                                                  \
        z_owned_##name##_t channel;                                                                               \
        channel.collection = collection_new_f(capacity);                                                          \
        channel.send = z_##send_closure_name(_z_##name##_push, NULL, channel.collection);                         \
        channel.recv = z_##recv_closure_name(_z_##name##_pull, NULL, channel.collection);                         \
        return channel;                                                                                           \
    }                                                                                                             \
    static inline z_owned_##name##_t *z_##name##_move(z_owned_##name##_t *val) { return val; }                    \
    static inline void z_##name##_drop(z_owned_##name##_t *channel) {                                             \
        collection_free_f(channel->collection, _z_##name##_elem_free);                                            \
        z_##send_closure_name##_drop(&channel->send);                                                             \
        z_##recv_closure_name##_drop(&channel->recv);                                                             \
    }

// z_owned_sample_ring_channel_t
_Z_CHANNEL_DEFINE(sample_ring_channel, closure_sample, closure_owned_sample, z_sample_t, z_owned_sample_t, _z_ring_mt_t,
                  _z_ring_mt, _z_ring_mt_free, _z_ring_mt_push, _z_ring_mt_pull, _z_owned_sample_move,
                  _z_sample_to_owned_ptr, z_sample_drop)

// z_owned_sample_fifo_channel_t
_Z_CHANNEL_DEFINE(sample_fifo_channel, closure_sample, closure_owned_sample, z_sample_t, z_owned_sample_t, _z_fifo_mt_t,
                  _z_fifo_mt, _z_fifo_mt_free, _z_fifo_mt_push, _z_fifo_mt_pull, _z_owned_sample_move,
                  _z_sample_to_owned_ptr, z_sample_drop)

#endif  // INCLUDE_ZENOH_PICO_API_HANDLERS_H
