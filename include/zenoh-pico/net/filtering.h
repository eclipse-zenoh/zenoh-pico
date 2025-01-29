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

#ifndef ZENOH_PICO_FILTERING_NETAPI_H
#define ZENOH_PICO_FILTERING_NETAPI_H

#include <stdint.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WRITE_FILTER_INIT = 0,
    WRITE_FILTER_ACTIVE = 1,
    WRITE_FILTER_OFF = 2,
} _z_write_filter_state_t;

typedef struct {
    uint32_t decl_id;
    uint8_t state;
} _z_writer_filter_ctx_t;

/**
 * Return type when declaring a queryable.
 */
typedef struct _z_write_filter_t {
    uint32_t _interest_id;
    _z_writer_filter_ctx_t *ctx;
} _z_write_filter_t;

z_result_t _z_write_filter_create(_z_session_t *zn, _z_write_filter_t *filter, _z_keyexpr_t keyexpr,
                                  uint8_t interest_flag);
z_result_t _z_write_filter_destroy(_z_session_t *zn, _z_write_filter_t *filter);
bool _z_write_filter_active(const _z_write_filter_t *filter);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_FILTERING_NETAPI_H */
