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
#ifndef INCLUDE_ZENOH_PICO_API_HANDLERS_H
#define INCLUDE_ZENOH_PICO_API_HANDLERS_H

#include <stdint.h>
#include <stdio.h>

#include "zenoh-pico/api/macros.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/fifo.h"
#include "zenoh-pico/collections/ring.h"
#include "zenoh-pico/system/platform.h"

_Z_ELEM_DEFINE(_z_owned_sample, z_owned_sample_t, _z_noop_size, z_sample_drop, _z_noop_copy)

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

z_owned_sample_channel_t z_owned_sample_channel_null(void);
int8_t z_closure_owned_sample_call(z_owned_closure_owned_sample_t *recv, z_owned_sample_t *dst);

// -- Ring
_Z_RING_DEFINE(_z_owned_sample, z_owned_sample_t)

typedef struct {
    _z_owned_sample_ring_t _ring;
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_t _mutex;
#endif
} z_owned_sample_ring_t;

z_owned_sample_channel_t z_sample_channel_ring_new(size_t capacity);
void z_sample_channel_ring_push(const z_sample_t *src, void *context);
int8_t z_sample_channel_ring_pull(z_owned_sample_t *dst, void *context);

// -- Fifo
_Z_FIFO_DEFINE(_z_owned_sample, z_owned_sample_t)

typedef struct {
    _z_owned_sample_fifo_t _fifo;
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_t _mutex;
    zp_condvar_t _cv_not_full;
    zp_condvar_t _cv_not_empty;
#endif
} z_owned_sample_fifo_t;

z_owned_sample_channel_t z_sample_channel_fifo_new(size_t capacity);
void z_sample_channel_fifo_push(const z_sample_t *src, void *context);
int8_t z_sample_channel_fifo_pull(z_owned_sample_t *dst, void *context);

#endif  // INCLUDE_ZENOH_PICO_API_HANDLERS_H
