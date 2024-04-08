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

#include "zenoh-pico/protocol/codec/core.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/utils/logging.h"

/*-------- Fifo Buffer Multithreaded --------*/
int8_t _z_fifo_mt_init(_z_fifo_mt_t *fifo, size_t capacity) {
    _Z_RETURN_IF_ERR(_z_fifo_init(&fifo->_fifo, capacity))

#if Z_FEATURE_MULTI_THREAD == 1
    _Z_RETURN_IF_ERR(zp_mutex_init(&fifo->_mutex))
    _Z_RETURN_IF_ERR(zp_condvar_init(&fifo->_cv_not_full))
    _Z_RETURN_IF_ERR(zp_condvar_init(&fifo->_cv_not_empty))
#endif

    return _Z_RES_OK;
}

_z_fifo_mt_t *_z_fifo_mt_new(size_t capacity) {
    _z_fifo_mt_t *fifo = (_z_fifo_mt_t *)zp_malloc(sizeof(_z_fifo_mt_t));
    if (fifo == NULL) {
        _Z_ERROR("zp_malloc failed");
        return NULL;
    }

    int8_t ret = _z_fifo_mt_init(fifo, capacity);
    if (ret != _Z_RES_OK) {
        _Z_ERROR("_z_fifo_mt_init failed: %i", ret);
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
    _Z_RETURN_IF_ERR(zp_mutex_lock(&f->_mutex))
    while (elem != NULL) {
        elem = _z_fifo_push(&f->_fifo, (void *)elem);
        if (elem != NULL) {
            _Z_RETURN_IF_ERR(zp_condvar_wait(&f->_cv_not_full, &f->_mutex))
        } else {
            _Z_RETURN_IF_ERR(zp_condvar_signal(&f->_cv_not_empty))
        }
    }
    _Z_RETURN_IF_ERR(zp_mutex_unlock(&f->_mutex))
#else   // Z_FEATURE_MULTI_THREAD == 1
    _z_fifo_push_drop(&f->_fifo, elem, element_free);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    return _Z_RES_OK;
}

int8_t _z_fifo_mt_pull(void *dst, void *context, z_element_move_f element_move) {
    _z_fifo_mt_t *f = (_z_fifo_mt_t *)context;

#if Z_FEATURE_MULTI_THREAD == 1
    void *src = NULL;
    _Z_RETURN_IF_ERR(zp_mutex_lock(&f->_mutex))
    while (src == NULL) {
        src = _z_fifo_pull(&f->_fifo);
        if (src == NULL) {
            _Z_RETURN_IF_ERR(zp_condvar_wait(&f->_cv_not_empty, &f->_mutex))
        } else {
            _Z_RETURN_IF_ERR(zp_condvar_signal(&f->_cv_not_full))
        }
    }
    _Z_RETURN_IF_ERR(zp_mutex_unlock(&f->_mutex))
    element_move(dst, src);
#else   // Z_FEATURE_MULTI_THREAD == 1
    void *src = _z_fifo_pull(&f->_fifo);
    if (src != NULL) {
        element_move(dst, src);
    }
#endif  // Z_FEATURE_MULTI_THREAD == 1

    return _Z_RES_OK;
}

int8_t _z_fifo_mt_try_pull(void *dst, void *context, z_element_move_f element_move) {
    _z_fifo_mt_t *f = (_z_fifo_mt_t *)context;

#if Z_FEATURE_MULTI_THREAD == 1
    void *src = NULL;
    _Z_RETURN_IF_ERR(zp_mutex_lock(&f->_mutex))
    src = _z_fifo_pull(&f->_fifo);
    if (src != NULL) {
        _Z_RETURN_IF_ERR(zp_condvar_signal(&f->_cv_not_full))
    }
    _Z_RETURN_IF_ERR(zp_mutex_unlock(&f->_mutex))
#else   // Z_FEATURE_MULTI_THREAD == 1
    void *src = _z_fifo_pull(&f->_fifo);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    if (src != NULL) {
        element_move(dst, src);
    }

    return _Z_RES_OK;
}