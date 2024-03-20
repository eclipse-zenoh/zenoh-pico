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

#include "zenoh-pico/api/macros.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/ring.h"
#include "zenoh-pico/system/platform.h"

typedef struct {
    z_keyexpr_t keyexpr;
    z_bytes_t payload;
} z_owned_sample_t;

_Z_ELEM_DEFINE(_z_sample, z_owned_sample_t, _z_noop_size, _z_noop_size, _z_noop_copy)
_Z_RING_DEFINE(_z_sample, z_owned_sample_t)

typedef struct {
    _z_sample_ring_t _ring;
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_t _mutex;
#endif
} z_sample_ring_t;

z_sample_ring_t z_sample_ring_make(size_t capacity);
void z_sample_ring_clear(z_sample_ring_t *r);
void z_sample_ring_free(z_sample_ring_t **r);

void z_sample_ring_push(z_sample_ring_t *r, const z_sample_t *e);
z_owned_sample_t *z_sample_ring_pull(z_sample_ring_t *r);

#endif  // INCLUDE_ZENOH_PICO_API_UTILS_H