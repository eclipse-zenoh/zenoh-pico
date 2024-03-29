//
// Copyright (c) 2022 ZettaScale Technology
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
#include <stdio.h>

#include "zenoh-pico/api/macros.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/fifo.h"
#include "zenoh-pico/collections/ring.h"
#include "zenoh-pico/net/memory.h"
#include "zenoh-pico/system/platform.h"

// -- Channel
// TODO(sashacmc): clenup
#define _Z_CHANNEL_DEFINE(name, storage_type, send_closure_name, recv_closure_name, send_type, recv_type,           \
                          channel_push_f, channel_pull_f, storage_init_f, elem_copy_f, elem_convert_f, elem_free_f) \
    typedef struct {                                                                                                \
        z_owned_##send_closure_name##_t send;                                                                       \
        z_owned_##recv_closure_name##_t recv;                                                                       \
        storage_type *storage;                                                                                      \
    } z_owned_##name##_channel_t;                                                                                   \
    static inline void z_##name##_channel_elem_free(void **elem) {                                                  \
        elem_free_f((recv_type *)*elem);                                                                            \
        *elem = NULL;                                                                                               \
    }                                                                                                               \
    static inline void z_##name##_channel_elem_copy(void *dst, const void *src) {                                   \
        elem_copy_f((recv_type *)src, (const recv_type *)dst);                                                      \
    }                                                                                                               \
    static inline void z_##name##_channel_push(const send_type *elem, void *context) {                              \
        void *internal_elem = elem_convert_f(elem);                                                                 \
        if (internal_elem == NULL) {                                                                                \
            return;                                                                                                 \
        }                                                                                                           \
        channel_push_f(internal_elem, context, z_##name##_channel_elem_free);                                       \
    }                                                                                                               \
    static inline void z_##name##_channel_pull(recv_type *elem, void *context) {                                    \
        channel_pull_f(elem, context, z_##name##_channel_elem_copy);                                                \
    }                                                                                                               \
    static inline z_owned_##name##_channel_t z_##name##_channel(size_t capacity) {                                  \
        z_owned_##name##_channel_t ch;                                                                              \
        ch.storage = storage_init_f(capacity);                                                                      \
        z_owned_##send_closure_name##_t send = z_##send_closure_name(z_##name##_channel_push, NULL, ch.storage);    \
        ch.send = send;                                                                                             \
        z_owned_##recv_closure_name##_t recv = z_##recv_closure_name(z_##name##_channel_pull, NULL, ch.storage);    \
        ch.recv = recv;                                                                                             \
        return ch;                                                                                                  \
    }

// -- Ring channel
typedef struct {
    _z_ring_t _ring;
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_t _mutex;
#endif
} _z_channel_ring_t;  // TODO(sashacmc): rename to sync_ring?

_z_channel_ring_t *_z_channel_ring(size_t capacity);
void _z_channel_ring_push(const void *src, void *context, z_element_free_f element_free);
int8_t _z_channel_ring_pull(void *dst, void *context, z_element_copy_f element_copy);
/*
TODO(sashacmc): implement
// -- Fifo
_Z_FIFO_DEFINE(_z_owned_sample, z_owned_sample_t)

typedef struct {
    _z_owned_sample_fifo_t _fifo;
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_t _mutex;
    zp_condvar_t _cv_not_full;
    zp_condvar_t _cv_not_empty;
#endif
} z_owned_sample_fifo_t;

z_owned_sample_channel_t z_sample_channel_fifo_new(size_t capacity);
void z_sample_channel_fifo_push(const z_sample_t *src, void *context);
int8_t z_sample_channel_fifo_pull(z_owned_sample_t *dst, void *context);
*/

// -- Samples handler
// _Z_RING_DEFINE(_z_owned_sample, z_owned_sample_t)
static inline size_t _z_owned_sample_size(z_owned_sample_t *s) { return sizeof(*s); }

static inline void _z_owned_sample_copy(z_owned_sample_t *dst, const z_owned_sample_t *src) {
    _z_sample_copy(dst->_value, src->_value);
}

static inline z_owned_sample_t *_z_sample_to_owned_ptr(const _z_sample_t *src) {
    z_owned_sample_t *dst = (z_owned_sample_t *)zp_malloc(sizeof(z_owned_sample_t));
    if (dst && src) {
        dst->_value = (_z_sample_t *)zp_malloc(sizeof(_z_sample_t));
        _z_sample_copy(dst->_value, src);
    }
    return dst;
}

_Z_ELEM_DEFINE(_z_owned_sample, z_owned_sample_t, _z_owned_sample_size, z_sample_drop, _z_owned_sample_copy)

_Z_CHANNEL_DEFINE(sample_ring, _z_channel_ring_t, closure_sample, closure_owned_sample, z_sample_t, z_owned_sample_t,
                  _z_channel_ring_push, _z_channel_ring_pull, _z_channel_ring, _z_owned_sample_copy,
                  _z_sample_to_owned_ptr, z_sample_drop)

#endif  // INCLUDE_ZENOH_PICO_API_HANDLERS_H
