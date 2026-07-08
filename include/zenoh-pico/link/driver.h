//
// Copyright (c) 2026 ZettaScale Technology
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

#ifndef ZENOH_PICO_LINK_DRIVER_H
#define ZENOH_PICO_LINK_DRIVER_H

#include "zenoh-pico/link/link.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef z_result_t (*_z_link_driver_validate_f)(_z_endpoint_t *endpoint);
typedef z_result_t (*_z_link_driver_create_f)(_z_link_t *link, _z_endpoint_t *endpoint, const _z_config_t *session_cfg);
typedef z_result_t (*_z_link_driver_activate_f)(_z_link_t *self);
typedef _z_link_driver_activate_f _z_f_link_open;
typedef _z_link_driver_activate_f _z_f_link_listen;

typedef struct {
    _z_link_driver_validate_f _validate_f;
    _z_link_driver_create_f _create_f;
    _z_f_link_open _open_f;
    _z_f_link_listen _listen_f;
} _z_link_driver_t;

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_DRIVER_H */
