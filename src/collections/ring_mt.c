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

#include "zenoh-pico/collections/ring_mt.h"

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/utils/logging.h"

/*-------- Ring Buffer Multithreaded --------*/
int8_t _z_ring_mt_init(_z_ring_mt_t *ring, size_t capacity) {
    int8_t res = _z_ring_init(&ring->_ring, capacity);
    if (res) {
        return res;
    }

#if Z_FEATURE_MULTI_THREAD == 1
    res = zp_mutex_init(&ring->_mutex);
    if (res) {
        return res;
    }
#endif
    return _Z_RES_OK;
}

_z_ring_mt_t *_z_ring_mt(size_t capacity) {
    _z_ring_mt_t *ring = (_z_ring_mt_t *)zp_malloc(sizeof(_z_ring_mt_t));
    if (ring == NULL) {
        _Z_ERROR("zp_malloc failed");
        return NULL;
    }

    int8_t res = _z_ring_mt_init(ring, capacity);
    if (res) {
        _Z_ERROR("_z_ring_mt_init failed: %i", res);
        return NULL;
    }

    return ring;
}

void _z_ring_mt_clear(_z_ring_mt_t *ring, z_element_free_f free_f) {
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_free(&ring->_mutex);
#endif

    _z_ring_clear(&ring->_ring, free_f);
}

void _z_ring_mt_free(_z_ring_mt_t *ring, z_element_free_f free_f) {
    _z_ring_mt_clear(ring, free_f);

    zp_free(ring);
}

int8_t _z_ring_mt_push(const void *elem, void *context, z_element_free_f element_free) {
    if (elem == NULL || context == NULL) {
        return _Z_ERR_GENERIC;
    }

    _z_ring_mt_t *r = (_z_ring_mt_t *)context;

#if Z_FEATURE_MULTI_THREAD == 1
    int8_t res = zp_mutex_lock(&r->_mutex);
    if (res) {
        return res;
    }
#endif

    _z_ring_push_force_drop(&r->_ring, (void *)elem, element_free);

#if Z_FEATURE_MULTI_THREAD == 1
    res = zp_mutex_unlock(&r->_mutex);
    if (res) {
        return res;
    }
#endif
    return _Z_RES_OK;
}

int8_t _z_ring_mt_pull(void *dst, void *context, z_element_move_f element_move) {
    _z_ring_mt_t *r = (_z_ring_mt_t *)context;

#if Z_FEATURE_MULTI_THREAD == 1
    int res = zp_mutex_lock(&r->_mutex);
    if (res) {
        return res;
    }
#endif

    void *src = _z_ring_pull(&r->_ring);

#if Z_FEATURE_MULTI_THREAD == 1
    res = zp_mutex_unlock(&r->_mutex);
    if (res) {
        return res;
    }
#endif

    if (src) {
        element_move(dst, src);
    }
    return _Z_RES_OK;
}
