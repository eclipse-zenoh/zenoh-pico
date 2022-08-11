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

#ifndef ZENOH_PICO_LINK_MANAGER_H
#define ZENOH_PICO_LINK_MANAGER_H

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/link.h"
#include "zenoh-pico/collections/string.h"

typedef struct
{
    uint8_t __dummy; // Just to avoid empty structures that might cause undefined behavior
} _z_link_manager_t;

_z_link_manager_t *_z_link_manager_init(void);
void _z_link_manager_free(_z_link_manager_t **ztm);

#if Z_LINK_TCP == 1
_z_link_t *_z_new_link_tcp(_z_endpoint_t endpoint);
#endif
#if Z_LINK_UDP_UNICAST == 1
_z_link_t *_z_new_link_udp_unicast(_z_endpoint_t endpoint);
#endif
#if Z_LINK_UDP_MULTICAST == 1
_z_link_t *_z_new_link_udp_multicast(_z_endpoint_t endpoint);
#endif
#if Z_LINK_BLUETOOTH == 1
_z_link_t *_z_new_link_bt(_z_endpoint_t endpoint);
#endif
#if Z_LINK_SERIAL == 1
_z_link_t *_z_new_link_serial(_z_endpoint_t endpoint);
#endif

#endif /* ZENOH_PICO_LINK_MANAGER_H */
