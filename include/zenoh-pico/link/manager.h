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

#ifndef ZENOH_PICO_LINK_MANAGER_H
#define ZENOH_PICO_LINK_MANAGER_H

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/link.h"
#include "zenoh-pico/collections/string.h"

typedef struct
{
    // Placeholder for future extensions
} _zn_link_manager_t;

_zn_link_manager_t *_zn_link_manager_init(void);
void _zn_link_manager_free(_zn_link_manager_t **ztm);

#if ZN_LINK_BLUETOOTH == 1
_zn_link_t *_zn_new_link_bt(_zn_endpoint_t endpoint);
#endif
#if ZN_LINK_TCP == 1
_zn_link_t *_zn_new_link_tcp(_zn_endpoint_t endpoint);
#endif
#if ZN_LINK_UDP_UNICAST == 1
_zn_link_t *_zn_new_link_udp_unicast(_zn_endpoint_t endpoint);
#endif
#if ZN_LINK_UDP_MULTICAST == 1
_zn_link_t *_zn_new_link_udp_multicast(_zn_endpoint_t endpoint);
#endif

#endif /* ZENOH_PICO_LINK_MANAGER_H */
