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

#include "zenoh-pico/collections/fifo_mt.h"

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/utils/logging.h"

/*-------- Fifo Buffer Multithreaded --------*/
int8_t _z_fifo_mt_init(_z_fifo_mt_t *fifo, size_t capacity) {
    int8_t res = _z_fifo_init(&fifo->_fifo, capacity);
    if (res) {
        return res;
    }

#if Z_FEATURE_MULTI_THREAD == 1
    res = zp_mutex_init(&fifo->_mutex);
    if (res) {
        return res;
    }
    res = zp_condvar_init(&fifo->_cv_not_full);
    if (res) {
        return res;
    }
    res = zp_condvar_init(&fifo->_cv_not_empty);
    if (res) {
        return res;
    }
#endif

    return _Z_RES_OK;
}

_z_fifo_mt_t *_z_fifo_mt(size_t capacity) {
    _z_fifo_mt_t *fifo = (_z_fifo_mt_t *)zp_malloc(sizeof(_z_fifo_mt_t));
    if (fifo == NULL) {
        _Z_ERROR("zp_malloc failed");
        return NULL;
    }

    int8_t res = _z_fifo_mt_init(fifo, capacity);
    if (res) {
        _Z_ERROR("_z_fifo_mt_init failed: %i", res);
        zp_free(fifo);
        return NULL;
    }

    return fifo;
}

void _z_fifo_mt_clear(_z_fifo_mt_t *fifo, z_element_free_f free_f) {
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_free(&fifo->_mutex);
    zp_condvar_free(&fifo->_cv_not_full);
    zp_condvar_free(&fifo->_cv_not_empty);
#endif

    _z_fifo_clear(&fifo->_fifo, free_f);
}

void _z_fifo_mt_free(_z_fifo_mt_t *fifo, z_element_free_f free_f) {
    _z_fifo_mt_clear(fifo, free_f);
    zp_free(fifo);
}

int8_t _z_fifo_mt_push(const void *elem, void *context, z_element_free_f element_free) {
    if (elem == NULL || context == NULL) {
        return _Z_ERR_GENERIC;
    }

    _z_fifo_mt_t *f = (_z_fifo_mt_t *)context;

#if Z_FEATURE_MULTI_THREAD == 1
    int res = zp_mutex_lock(&f->_mutex);
    if (res) {
        return res;
    }
    while (elem != NULL) {
        elem = _z_fifo_push(&f->_fifo, (void *)elem);
        if (elem != NULL) {
            res = zp_condvar_wait(&f->_cv_not_full, &f->_mutex);
            if (res) {
                return res;
            }
        } else {
            res = zp_condvar_signal(&f->_cv_not_empty);
            if (res) {
                return res;
            }
        }
    }
    res = zp_mutex_unlock(&f->_mutex);
    if (res) {
        return res;
    }
#else   // Z_FEATURE_MULTI_THREAD == 1
    _z_fifo_push_drop(&f->_fifo, elem, element_free);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    return _Z_RES_OK;
}

int8_t _z_fifo_mt_pull(void *dst, void *context, z_element_move_f element_move) {
    _z_fifo_mt_t *f = (_z_fifo_mt_t *)context;

#if Z_FEATURE_MULTI_THREAD == 1
    void *src = NULL;
    int res = zp_mutex_lock(&f->_mutex);
    if (res) {
        return res;
    }
    while (src == NULL) {
        src = _z_fifo_pull(&f->_fifo);
        if (src == NULL) {
            res = zp_condvar_wait(&f->_cv_not_empty, &f->_mutex);
            if (res) {
                return res;
            }
        } else {
            res = zp_condvar_signal(&f->_cv_not_full);
            if (res) {
                return res;
            }
        }
    }
    res = zp_mutex_unlock(&f->_mutex);
    if (res) {
        return res;
    }
    element_move(dst, src);
#else   // Z_FEATURE_MULTI_THREAD == 1
    void *src = _z_fifo_pull(&f->_fifo);
    if (src) {
        element_move(dst, src);
    }
#endif  // Z_FEATURE_MULTI_THREAD == 1

    return _Z_RES_OK;
}
