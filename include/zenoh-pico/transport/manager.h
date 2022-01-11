/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#ifndef ZENOH_PICO_TRANSPORT_MANAGER_H
#define ZENOH_PICO_TRANSPORT_MANAGER_H

#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/collections/bytes.h"

typedef struct
{
    z_bytes_t local_pid;
    // FIXME: remote_pids

    _zn_link_manager_t *link_manager;
} _zn_transport_manager_t;

_zn_transport_manager_t *_zn_transport_manager_init();
void _zn_transport_manager_free(_zn_transport_manager_t **ztm);

_zn_transport_p_result_t _zn_new_transport(_zn_transport_manager_t *ztm, z_str_t locator, uint8_t mode);
void _zn_free_transport(_zn_transport_manager_t *ztm, _zn_transport_t **zt);

#endif /* ZENOH_PICO_TRANSPORT_MANAGER_H */
