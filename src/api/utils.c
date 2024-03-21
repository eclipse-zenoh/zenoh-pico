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
#include "zenoh-pico/api/utils.h"

#include "zenoh-pico/api/macros.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/system/platform.h"

// z_owned_sample_ring_t z_sample_channel_ring_new(size_t capacity) {
//     z_owned_sample_ring_t ring;
//     ring._ring = _z_sample_ring_new(capacity);

// #if Z_FEATURE_MULTI_THREAD == 1
//     zp_mutex_init(&ring._mutex);
// #endif

//     return ring;
// }

// void z_owned_sample_ring_drop(z_owned_sample_ring_t *r) {
// #if Z_FEATURE_MULTI_THREAD == 1
//     zp_mutex_lock(&r->_mutex);
// #endif
//     _z_owned_sample_ring_clear(&r->_ring);
// #if Z_FEATURE_MULTI_THREAD == 1
//     zp_mutex_unlock(&r->_mutex);
//     zp_mutex_free(&r->_mutex);
// #endif
// }

// void z_sample_channel_ring_push(z_owned_sample_ring_t *r, const z_sample_t *s) {
//     z_owned_sample_t *os = (z_owned_sample_t *)zp_malloc(sizeof(z_owned_sample_t));
//     memcpy(&os->keyexpr, &s->keyexpr, sizeof(_z_keyexpr_t));
//     memcpy(&os->payload, &s->payload, sizeof(z_bytes_t));
// #if Z_FEATURE_MULTI_THREAD == 1
//     zp_mutex_lock(&r->_mutex);
// #endif
//     _z_owned_sample_ring_push_force_drop(&r->_ring, os);
// #if Z_FEATURE_MULTI_THREAD == 1
//     zp_mutex_unlock(&r->_mutex);
// #endif
// }

// z_owned_sample_t *z_sample_channel_ring_pull(z_owned_sample_ring_t *r) {
// #if Z_FEATURE_MULTI_THREAD == 1
//     zp_mutex_lock(&r->_mutex);
// #endif
//     z_owned_sample_t *ret = _z_owned_sample_ring_pull(&r->_ring);
// #if Z_FEATURE_MULTI_THREAD == 1
//     zp_mutex_unlock(&r->_mutex);
// #endif
//     return ret;
// }
