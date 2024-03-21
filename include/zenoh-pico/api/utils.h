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
#ifndef INCLUDE_ZENOH_PICO_API_UTILS_H
#define INCLUDE_ZENOH_PICO_API_UTILS_H

#include <stdint.h>
#include <stdio.h>

#include "zenoh-pico/api/macros.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/ring.h"
#include "zenoh-pico/system/platform.h"

typedef struct {
    z_owned_keyexpr_t keyexpr;
    z_bytes_t payload;
    // @TODO: implement the rest of the fields
} z_owned_sample_t;

static inline z_owned_sample_t z_sample_null(void) {
    z_owned_sample_t sample;
    sample.keyexpr = z_keyexpr_null();
    sample.payload = z_bytes_null();
    return sample;
}

static inline z_owned_sample_t *z_sample_move(z_owned_sample_t *sample) { return sample; }
static inline z_owned_sample_t *z_sample_loan(z_owned_sample_t *sample) { return sample; }

static inline bool z_sample_check(const z_owned_sample_t *sample) {
    return z_keyexpr_check(&sample->keyexpr) && z_bytes_check(&sample->payload);
}

static inline z_owned_sample_t z_sample_to_owned(const z_sample_t *src) {
    z_owned_sample_t dst = z_sample_null();

    if (src == NULL) {
        return dst;
    }

    _z_keyexpr_t *ke = (_z_keyexpr_t *)zp_malloc(sizeof(_z_keyexpr_t));
    if (ke == NULL) {
        return dst;
    }
    *ke = _z_keyexpr_duplicate(src->keyexpr);
    dst.keyexpr._value = ke;
    dst.payload = _z_bytes_duplicate(&src->payload);

    return dst;
}

static inline void z_sample_drop(z_owned_sample_t *s) {
    if (s != NULL) {
        z_keyexpr_drop(&s->keyexpr);
        _z_bytes_clear(&s->payload);
    }
}

// -- Channel
typedef int8_t (*_z_owned_sample_handler_t)(z_owned_sample_t *dst, void *context);

typedef struct {
    void *context;
    _z_owned_sample_handler_t call;
    _z_dropper_handler_t drop;
} z_owned_closure_owned_sample_t;

typedef struct {
    z_owned_closure_sample_t send;
    z_owned_closure_owned_sample_t recv;
} z_owned_sample_channel_t;

static inline z_owned_sample_channel_t z_owned_sample_channel_null() {
    z_owned_sample_channel_t ch = {.send = NULL, .recv = NULL};
    return ch;
}

static inline int8_t z_closure_owned_sample_call(z_owned_closure_owned_sample_t *recv, z_owned_sample_t *dst) {
    int8_t res = _Z_ERR_GENERIC;
    if (recv != NULL) {
        res = (recv->call)(dst, recv->context);
    }
    return res;
}

// -- Ring
_Z_ELEM_DEFINE(_z_owned_sample, z_owned_sample_t, _z_noop_size, _z_noop_clear, _z_noop_copy)
_Z_RING_DEFINE(_z_owned_sample, z_owned_sample_t)

typedef struct {
    _z_owned_sample_ring_t _ring;
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_t _mutex;
#endif
} z_owned_sample_ring_t;

static inline void z_owned_sample_ring_push(const z_sample_t *src, void *context) {
    if (src == NULL || context == NULL) {
        return;
    }

    z_owned_sample_ring_t *r = (z_owned_sample_ring_t *)context;
    z_owned_sample_t *dst = (z_owned_sample_t *)zp_malloc(sizeof(z_owned_sample_t));
    if (dst == NULL) {
        return;
    }
    *dst = z_sample_to_owned(src);

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&r->_mutex);
#endif
    _z_owned_sample_ring_push_force_drop(&r->_ring, dst);
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_unlock(&r->_mutex);
#endif
}

static inline int8_t z_owned_sample_ring_pull(z_owned_sample_t *dst, void *context) {
    int8_t ret = _Z_RES_OK;

    z_owned_sample_ring_t *r = (z_owned_sample_ring_t *)context;

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&r->_mutex);
#endif
    z_owned_sample_t *src = _z_owned_sample_ring_pull(&r->_ring);
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_unlock(&r->_mutex);
#endif

    if (src == NULL) {
        *dst = z_sample_null();
    } else {
        memcpy(dst, src, sizeof(z_owned_sample_t));
    }
    return ret;
}

static inline z_owned_sample_channel_t z_sample_ring_new(size_t capacity) {
    z_owned_sample_channel_t ch = z_owned_sample_channel_null();

    z_owned_sample_ring_t *ring = (z_owned_sample_ring_t *)zp_malloc(sizeof(z_owned_sample_ring_t));
    if (ring != NULL) {
        int8_t res = _z_owned_sample_ring_init(&ring->_ring, capacity);
        if (res == _Z_RES_OK) {
            z_owned_closure_sample_t send = z_closure(z_owned_sample_ring_push, NULL, ring);
            ch.send = send;
            z_owned_closure_owned_sample_t recv = z_closure(z_owned_sample_ring_pull, NULL, ring);
            ch.recv = recv;
        } else {
            zp_free(ring);
        }
    }

    return ch;
}

#endif  // INCLUDE_ZENOH_PICO_API_UTILS_H