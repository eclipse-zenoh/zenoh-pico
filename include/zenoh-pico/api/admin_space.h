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

#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_ADMIN_SPACE == 1

/**
 * Starts the admin space for a session.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to start the admin space on.
 *
 * Return:
 *   ``0`` if start operation is successful, ``negative value`` otherwise.
 *
 * .. warning:: This API has been marked as unstable: it works as advertised, but it may be changed in a future release.
 */
z_result_t zp_start_admin_space(z_loaned_session_t *zs);

/**
 * Stops the admin space for a session.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to stop the admin space on.
 *
 * Return:
 *   ``0`` if stop operation is successful, ``negative value`` otherwise.
 *
 * .. warning:: This API has been marked as unstable: it works as advertised, but it may be changed in a future release.
 */
z_result_t zp_stop_admin_space(z_loaned_session_t *zs);

#endif  // Z_FEATURE_ADMIN_SPACE == 1
#endif  // Z_FEATURE_UNSTABLE_API

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_API_ADMIN_SPACE_H */
