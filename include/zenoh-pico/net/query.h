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

#ifndef ZENOH_PICO_QUERY_NETAPI_H
#define ZENOH_PICO_QUERY_NETAPI_H

#include <stdint.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"

/**
 * The query to be answered by a queryable.
 */
typedef struct _z_query_t {
    _z_value_t _value;
    _z_keyexpr_t _key;
    uint32_t _request_id;
    _z_session_weak_t _zn;  // Can't be an rc because of cross referencing
    _z_bytes_t attachment;
    char *_parameters;
    _Bool _anyke;
} _z_query_t;

_z_query_t _z_query_null(void);
void _z_query_clear(_z_query_t *q);
int8_t _z_query_copy(_z_query_t *dst, const _z_query_t *src);
void _z_query_free(_z_query_t **query);

_Z_REFCOUNT_DEFINE(_z_query, _z_query)

/**
 * Return type when declaring a queryable.
 */
typedef struct {
    uint32_t _entity_id;
    _z_session_rc_t _zn;
} _z_queryable_t;

#if Z_FEATURE_QUERYABLE == 1
_z_query_t _z_query_create(_z_value_t *value, _z_keyexpr_t *key, const _z_slice_t *parameters, _z_session_rc_t *zn,
                           uint32_t request_id, const _z_bytes_t attachment);
void _z_queryable_clear(_z_queryable_t *qbl);
void _z_queryable_free(_z_queryable_t **qbl);
_z_queryable_t _z_queryable_null(void);
_Bool _z_queryable_check(const _z_queryable_t *queryable);
#endif

#endif /* ZENOH_PICO_QUERY_NETAPI_H */
