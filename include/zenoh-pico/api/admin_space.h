//
// Copyright (c) 2025 ZettaScale Technology
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

#ifndef ZENOH_PICO_API_ADMIN_SPACE_H
#define ZENOH_PICO_API_ADMIN_SPACE_H

#include "zenoh-pico/api/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    _z_session_weak_t _zn;
    z_owned_queryable_t _queryable;
} _z_admin_space_t;

_Z_OWNED_TYPE_VALUE_PREFIX(ze, _z_admin_space_t, admin_space)
_Z_OWNED_FUNCTIONS_NO_COPY_NO_MOVE_DEF_PREFIX(ze, admin_space)

z_result_t ze_declare_admin_space(const z_loaned_session_t *zs, ze_owned_admin_space_t *admin_space);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_API_ADMIN_SPACE_H */
