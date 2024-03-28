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
#include "zenoh-pico/api/handlers.h"
#include "zenoh-pico/api/macros.h"
#include "zenoh-pico/net/memory.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/system/platform.h"

z_owned_sample_t z_sample_to_owned(const _z_sample_t *src) {
    z_owned_sample_t dst = z_sample_null();

    if (src) {
        dst._value = (_z_sample_t *)zp_malloc(sizeof(_z_sample_t));
        _z_sample_copy(dst._value, src);
    }

    return dst;
}

// -- Channel
z_owned_sample_channel_t z_owned_sample_channel_null() {
    z_owned_sample_channel_t ch = {.send = NULL, .recv = NULL};
    return ch;
}

int8_t z_closure_owned_sample_call(z_owned_closure_owned_sample_t *recv, z_owned_sample_t *dst) {
    int8_t res = _Z_ERR_GENERIC;
    if (recv != NULL) {
        res = (recv->call)(dst, recv->context);
    }
    return res;
}

// -- Ring
void z_sample_channel_ring_push(const z_sample_t *src, void *context) {
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

int8_t z_sample_channel_ring_pull(z_owned_sample_t *dst, void *context) {
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

z_owned_sample_channel_t z_sample_channel_ring_new(size_t capacity) {
    z_owned_sample_channel_t ch = z_owned_sample_channel_null();

    z_owned_sample_ring_t *ring = (z_owned_sample_ring_t *)zp_malloc(sizeof(z_owned_sample_ring_t));
    if (ring != NULL) {
        int8_t res = _z_owned_sample_ring_init(&ring->_ring, capacity);
#if Z_FEATURE_MULTI_THREAD == 1
        res = zp_mutex_init(&ring->_mutex);
#endif
        if (res == _Z_RES_OK) {
            z_owned_closure_sample_t send = z_closure(z_sample_channel_ring_push, NULL, ring);
            ch.send = send;
            z_owned_closure_owned_sample_t recv = z_closure(z_sample_channel_ring_pull, NULL, ring);
            ch.recv = recv;
        } else {
            zp_free(ring);
        }
    }

    return ch;
}

// -- Fifo
void z_sample_channel_fifo_push(const z_sample_t *src, void *context) {
    if (src == NULL || context == NULL) {
        return;
    }

    z_owned_sample_fifo_t *f = (z_owned_sample_fifo_t *)context;
    z_owned_sample_t *dst = (z_owned_sample_t *)zp_malloc(sizeof(z_owned_sample_t));
    if (dst == NULL) {
        return;
    }
    *dst = z_sample_to_owned(src);

#if Z_FEATURE_MULTI_THREAD == 1

    zp_mutex_lock(&f->_mutex);
    while (dst != NULL) {
        dst = _z_owned_sample_fifo_push(&f->_fifo, dst);
        if (dst != NULL) {
            zp_condvar_wait(&f->_cv_not_full, &f->_mutex);
        } else {
            zp_condvar_signal(&f->_cv_not_empty);
        }
    }
    zp_mutex_unlock(&f->_mutex);

#elif  // Z_FEATURE_MULTI_THREAD == 1

    _z_owned_sample_fifo_push_drop(&f->_fifo, dst);

#endif  // Z_FEATURE_MULTI_THREAD == 1
}

int8_t z_sample_channel_fifo_pull(z_owned_sample_t *dst, void *context) {
    int8_t ret = _Z_RES_OK;

    z_owned_sample_fifo_t *f = (z_owned_sample_fifo_t *)context;

#if Z_FEATURE_MULTI_THREAD == 1

    z_owned_sample_t *src = NULL;
    zp_mutex_lock(&f->_mutex);
    while (src == NULL) {
        src = _z_owned_sample_fifo_pull(&f->_fifo);
        if (src == NULL) {
            zp_condvar_wait(&f->_cv_not_empty, &f->_mutex);
        } else {
            zp_condvar_signal(&f->_cv_not_full);
        }
    }
    zp_mutex_unlock(&f->_mutex);
    memcpy(dst, src, sizeof(z_owned_sample_t));

#elif  // Z_FEATURE_MULTI_THREAD == 1

    z_owned_sample_t *src = _z_owned_sample_fifo_pull(&f->_fifo);
    if (src != NULL) {
        memcpy(dst, src, sizeof(z_owned_sample_t));
    }

#endif  // Z_FEATURE_MULTI_THREAD == 1

    return ret;
}

z_owned_sample_channel_t z_sample_channel_fifo_new(size_t capacity) {
    z_owned_sample_channel_t ch = z_owned_sample_channel_null();

    z_owned_sample_fifo_t *fifo = (z_owned_sample_fifo_t *)zp_malloc(sizeof(z_owned_sample_fifo_t));
    if (fifo != NULL) {
        int8_t res = _z_owned_sample_fifo_init(&fifo->_fifo, capacity);
#if Z_FEATURE_MULTI_THREAD == 1
        res = zp_mutex_init(&fifo->_mutex);
        res = zp_condvar_init(&fifo->_cv_not_full);
        res = zp_condvar_init(&fifo->_cv_not_empty);
#endif
        if (res == _Z_RES_OK) {
            z_owned_closure_sample_t send = z_closure(z_sample_channel_fifo_push, NULL, fifo);
            ch.send = send;
            z_owned_closure_owned_sample_t recv = z_closure(z_sample_channel_fifo_pull, NULL, fifo);
            ch.recv = recv;
        } else {
            zp_free(fifo);
        }
    }

    return ch;
}
