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

#ifndef ZENOH_PICO_SAMPLE_NETAPI_H
#define ZENOH_PICO_SAMPLE_NETAPI_H

#include "zenoh-pico/net/encoding.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/session.h"

/**
 * A zenoh-net data sample.
 *
 * A sample is the value associated to a given resource at a given point in time.
 *
 * Members:
 *   _z_keyexpr_t key: The resource key of this data sample.
 *   _z_slice_t value: The value of this data sample.
 *   _z_encoding_t encoding: The encoding for the value of this data sample.
 */
typedef struct _z_sample_t {
    _z_keyexpr_t keyexpr;
    _z_bytes_t payload;
    _z_timestamp_t timestamp;
    _z_encoding_t encoding;
    z_sample_kind_t kind;
    _z_qos_t qos;
    _z_bytes_t attachment;
} _z_sample_t;
void _z_sample_clear(_z_sample_t *sample);

_z_sample_t _z_sample_null(void);
_Bool _z_sample_check(const _z_sample_t *sample);
void _z_sample_move(_z_sample_t *dst, _z_sample_t *src);

/**
 * Free a :c:type:`_z_sample_t`, including its internal fields.
 *
 * Parameters:
 *     sample: The :c:type:`_z_sample_t` to free.
 */
void _z_sample_free(_z_sample_t **sample);

int8_t _z_sample_copy(_z_sample_t *dst, const _z_sample_t *src);
_z_sample_t _z_sample_duplicate(const _z_sample_t *src);

_z_sample_t _z_sample_create(_z_keyexpr_t *key, const _z_bytes_t payload, const _z_timestamp_t *timestamp,
                             _z_encoding_t *encoding, const z_sample_kind_t kind, const _z_qos_t qos,
                             const _z_bytes_t attachment);

#endif /* ZENOH_PICO_SAMPLE_NETAPI_H */
