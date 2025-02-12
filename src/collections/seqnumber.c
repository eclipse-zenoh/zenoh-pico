//
// Copyright (c) 2025 ZettaScale Technology
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

#include "zenoh-pico/collections/seqnumber.h"

#include "zenoh-pico/utils/logging.h"

z_result_t _z_seqnumber_init(_z_seqnumber_t *seq) {
    if (seq == NULL) {
        return _Z_ERR_INVALID;
    }

#if Z_FEATURE_MULTI_THREAD == 1 && ZENOH_C_STANDARD == 99 && !defined(ZENOH_COMPILER_GCC)
    z_result_t res = _z_mutex_init(&seq->_mutex);
    if (res != _Z_RES_OK) {
        return res;
    }
#endif

    seq->_seq = 0;
    return _Z_RES_OK;
}

z_result_t _z_seqnumber_fetch(_z_seqnumber_t *seq, uint32_t *value) {
    if (seq == NULL || value == NULL) {
        return _Z_ERR_INVALID;
    }

#if Z_FEATURE_MULTI_THREAD == 1 && ZENOH_C_STANDARD == 99 && !defined(ZENOH_COMPILER_GCC)
    z_result_t res = _z_mutex_lock(&seq->_mutex);
    if (res != _Z_RES_OK) {
        return res;
    }
#endif

    *value = seq->_seq;

#if Z_FEATURE_MULTI_THREAD == 1 && ZENOH_C_STANDARD == 99 && !defined(ZENOH_COMPILER_GCC)
    res = _z_mutex_unlock(&seq->_mutex);
    if (res != _Z_RES_OK) {
        return res;
    }
#endif

    return _Z_RES_OK;
}

z_result_t _z_seqnumber_fetch_and_increment(_z_seqnumber_t *seq, uint32_t *value) {
    if (seq == NULL || value == NULL) {
        return _Z_ERR_INVALID;
    }

#if Z_FEATURE_MULTI_THREAD == 1 && ZENOH_C_STANDARD == 99 && !defined(ZENOH_COMPILER_GCC)
    z_result_t res = _z_mutex_lock(&seq->_mutex);
    if (res != _Z_RES_OK) {
        return res;
    }
#endif

#if Z_FEATURE_MULTI_THREAD == 1 && ZENOH_C_STANDARD == 99 && defined(ZENOH_COMPILER_GCC)
    *value = __sync_fetch_and_add(&seq->_seq, 1);
#else
    *value = seq->_seq++;
#endif

#if Z_FEATURE_MULTI_THREAD == 1 && ZENOH_C_STANDARD == 99 && !defined(ZENOH_COMPILER_GCC)
    res = _z_mutex_unlock(&seq->_mutex);
    if (res != _Z_RES_OK) {
        return res;
    }
#endif

    return _Z_RES_OK;
}
