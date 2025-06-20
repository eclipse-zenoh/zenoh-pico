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

typedef struct {
    uintptr_t peer;
    uint32_t decl_id;
} _z_filter_target_t;

typedef z_element_eq_f _z_filter_target_eq_f;
#define _z_filter_target_elem_copy _z_noop_copy
#define _z_filter_target_elem_clear _z_noop_clear
_Z_SLIST_DEFINE(_z_filter_target, _z_filter_target_t, false)

typedef enum {
    WRITE_FILTER_ACTIVE = 0,
    WRITE_FILTER_OFF = 1,
} _z_write_filter_state_t;

typedef struct {
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_t mutex;
#endif
    _z_filter_target_slist_t *targets;
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

#if Z_FEATURE_INTEREST == 1
static inline bool _z_write_filter_active(const _z_write_filter_t *filter) {
    return filter->ctx != NULL && filter->ctx->state == WRITE_FILTER_ACTIVE;
}
#else
static inline bool _z_write_filter_active(const _z_write_filter_t *filter) {
    _ZP_UNUSED(filter);
    return false;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_FILTERING_NETAPI_H */
