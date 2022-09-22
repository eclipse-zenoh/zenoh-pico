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

#ifndef ZENOH_PICO_MEMORY_NETAPI_H
#define ZENOH_PICO_MEMORY_NETAPI_H

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/session.h"

/**
 * Free a :c:type:`_z_sample_t`, including its internal fields.
 *
 * Parameters:
 *     sample: The :c:type:`_z_sample_t` to free.
 */
void _z_sample_move(_z_sample_t *dst, _z_sample_t *src);
void _z_sample_clear(_z_sample_t *sample);
void _z_sample_free(_z_sample_t **sample);

#endif /* ZENOH_PICO_MEMORY_NETAPI_H */
