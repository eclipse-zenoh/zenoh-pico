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
//

#ifndef ZENOH_PICO_TRANSPORT_MANAGER_H
#define ZENOH_PICO_TRANSPORT_MANAGER_H

#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/transport/transport.h"

typedef struct {
    _z_bytes_t _local_pid;
    // FIXME: remote_pids

    _z_link_manager_t *_link_manager;
} _z_transport_manager_t;

_z_transport_manager_t *_z_transport_manager_init(void);
void _z_transport_manager_free(_z_transport_manager_t **ztm);

_z_transport_p_result_t _z_new_transport(_z_transport_manager_t *ztm, char *locator, uint8_t mode);
void _z_free_transport(_z_transport_manager_t *ztm, _z_transport_t **zt);

#endif /* ZENOH_PICO_TRANSPORT_MANAGER_H */
