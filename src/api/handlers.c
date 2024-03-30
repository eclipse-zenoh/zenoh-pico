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

// -- Ring
void _z_channel_ring_push(const void *elem, void *context, z_element_free_f element_free) {
    if (elem == NULL || context == NULL) {
        return;
    }

    _z_channel_ring_t *r = (_z_channel_ring_t *)context;
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&r->_mutex);
#endif
    _z_ring_push_force_drop(&r->_ring, (void *)elem, element_free);
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_unlock(&r->_mutex);
#endif
}

int8_t _z_channel_ring_pull(void *dst, void *context, z_element_copy_f element_copy) {
    int8_t ret = _Z_RES_OK;

    _z_channel_ring_t *r = (_z_channel_ring_t *)context;

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&r->_mutex);
#endif
    void *src = _z_ring_pull(&r->_ring);
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_unlock(&r->_mutex);
#endif

    if (src == NULL) {
        dst = NULL;
    } else {
        element_copy(dst, src);
    }
    return ret;
}

_z_channel_ring_t *_z_channel_ring(size_t capacity) {
    _z_channel_ring_t *ring = (_z_channel_ring_t *)zp_malloc(sizeof(_z_channel_ring_t));
    if (ring == NULL) {
        return NULL;
    }

    int8_t res = _z_ring_init(&ring->_ring, capacity);
    if (res != _Z_RES_OK) {
        zp_free(ring);
        return NULL;
    }

#if Z_FEATURE_MULTI_THREAD == 1
    res = zp_mutex_init(&ring->_mutex);
    if (res != _Z_RES_OK) {
        // TODO(sashacmc): add logging
        zp_free(ring);
        return NULL;
    }
#endif

    return ring;
}

// -- Fifo
void _z_channel_fifo_push(const void *elem, void *context, z_element_free_f element_free) {
    if (elem == NULL || context == NULL) {
        return;
    }

    _z_channel_fifo_t *f = (_z_channel_fifo_t *)context;

#if Z_FEATURE_MULTI_THREAD == 1

    zp_mutex_lock(&f->_mutex);
    while (elem != NULL) {
        elem = _z_fifo_push(&f->_fifo, (void *)elem);
        if (elem != NULL) {
            zp_condvar_wait(&f->_cv_not_full, &f->_mutex);
        } else {
            zp_condvar_signal(&f->_cv_not_empty);
        }
    }
    zp_mutex_unlock(&f->_mutex);

#elif  // Z_FEATURE_MULTI_THREAD == 1

    _z_fifo_push_drop(&f->_fifo, elem, element_free);

#endif  // Z_FEATURE_MULTI_THREAD == 1
}

int8_t _z_channel_fifo_pull(void *dst, void *context, z_element_copy_f element_copy) {
    int8_t ret = _Z_RES_OK;

    _z_channel_fifo_t *f = (_z_channel_fifo_t *)context;

#if Z_FEATURE_MULTI_THREAD == 1

    void *src = NULL;
    zp_mutex_lock(&f->_mutex);
    while (src == NULL) {
        src = _z_fifo_pull(&f->_fifo);
        if (src == NULL) {
            zp_condvar_wait(&f->_cv_not_empty, &f->_mutex);
        } else {
            zp_condvar_signal(&f->_cv_not_full);
        }
    }
    zp_mutex_unlock(&f->_mutex);
    element_copy(dst, src);

#elif  // Z_FEATURE_MULTI_THREAD == 1

    void *src = _z_fifo_pull(&f->_fifo);
    if (src != NULL) {
        element_copy(dst, src);
    }

#endif  // Z_FEATURE_MULTI_THREAD == 1

    return ret;
}

_z_channel_fifo_t *_z_channel_fifo(size_t capacity) {
    _z_channel_fifo_t *fifo = (_z_channel_fifo_t *)zp_malloc(sizeof(_z_channel_fifo_t));
    if (fifo == NULL) {
        return NULL;
    }

    int8_t res = _z_fifo_init(&fifo->_fifo, capacity);
    if (res != _Z_RES_OK) {
        zp_free(fifo);
        return NULL;
    }

#if Z_FEATURE_MULTI_THREAD == 1
    // TODO(sashacmc): result error check
    res = zp_mutex_init(&fifo->_mutex);
    res = zp_condvar_init(&fifo->_cv_not_full);
    res = zp_condvar_init(&fifo->_cv_not_empty);
#endif

    return fifo;
}
