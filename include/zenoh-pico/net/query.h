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

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/protocol/core.h"

/**
 * The query to be answered by a queryable.
 */
typedef struct {
    _z_value_t _value;
    _z_keyexpr_t _key;
    _z_zint_t _qid;
    void *_zn;  // FIXME: _z_session_t *zn;
    char *_parameters;
    _Bool _anyke;
} z_query_t;

/**
 * Return type when declaring a queryable.
 */
typedef struct {
    _z_zint_t _id;
    void *_zn;  // FIXME: _z_session_t *zn;
} _z_queryable_t;

void _z_queryable_clear(_z_queryable_t *qbl);
void _z_queryable_free(_z_queryable_t **qbl);

#endif /* ZENOH_PICO_QUERY_NETAPI_H */
