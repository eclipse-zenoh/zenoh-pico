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

#include "zenoh-pico/api/handlers.h"

#include "zenoh-pico/net/memory.h"
#include "zenoh-pico/system/platform.h"

// -- Sample
void _z_owned_sample_move(z_owned_sample_t *dst, const z_owned_sample_t *src) {
    memcpy(dst, src, sizeof(z_owned_sample_t));
}

z_owned_sample_t *_z_sample_to_owned_ptr(const _z_sample_t *src) {
    z_owned_sample_t *dst = (z_owned_sample_t *)zp_malloc(sizeof(z_owned_sample_t));
    if (dst == NULL) {
        return NULL;
    }
    if (src != NULL) {
        dst->_value = (_z_sample_t *)zp_malloc(sizeof(_z_sample_t));
        _z_sample_copy(dst->_value, src);
    } else {
        dst->_value = NULL;
    }
    return dst;
}
