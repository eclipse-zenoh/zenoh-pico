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
#include "zenoh-pico/protocol/keyexpr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The query to be answered by a queryable.
 */
typedef struct _z_query_t {
    _z_keyexpr_t _key;
    _z_value_t _value;
    uint32_t _request_id;
    _z_session_weak_t _zn;
    _z_bytes_t _attachment;
    _z_string_t _parameters;
    bool _anyke;
} _z_query_t;

// Warning: None of the sub-types require a non-0 initialization. Add a init function if it changes.
static inline _z_query_t _z_query_null(void) { return (_z_query_t){0}; }
static inline bool _z_query_check(const _z_query_t *query) {
    return _z_keyexpr_check(&query->_key) || _z_value_check(&query->_value) || _z_bytes_check(&query->_attachment) ||
           _z_string_check(&query->_parameters);
}
z_result_t _z_query_send_reply_final(_z_query_t *q);
void _z_query_clear(_z_query_t *q);
void _z_query_free(_z_query_t **query);

_Z_REFCOUNT_DEFINE(_z_query, _z_query)

/**
 * Return type when declaring a queryable.
 */
typedef struct {
    uint32_t _entity_id;
    _z_session_weak_t _zn;
} _z_queryable_t;

#if Z_FEATURE_QUERYABLE == 1
// Warning: None of the sub-types require a non-0 initialization. Add a init function if it changes.
static inline _z_queryable_t _z_queryable_null(void) { return (_z_queryable_t){0}; }
static inline bool _z_queryable_check(const _z_queryable_t *queryable) { return !_Z_RC_IS_NULL(&queryable->_zn); }
static inline _z_query_t _z_query_alias(_z_value_t *value, _z_keyexpr_t *key, const _z_slice_t *parameters,
                                        _z_session_rc_t *zn, uint32_t request_id, const _z_bytes_t *attachment,
                                        bool anyke) {
    _z_query_t ret;
    ret._key = _z_keyexpr_alias(key);
    ret._value = _z_value_alias(value);
    ret._request_id = request_id;
    ret._zn = _z_session_rc_clone_as_weak(zn);
    ret._attachment = _z_bytes_alias(*attachment);
    ret._parameters = _z_string_alias_slice(parameters);
    ret._anyke = anyke;
    return ret;
}
void _z_queryable_clear(_z_queryable_t *qbl);
void _z_queryable_free(_z_queryable_t **qbl);

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_QUERY_NETAPI_H */
