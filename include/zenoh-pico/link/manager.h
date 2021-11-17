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

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/link/link.h"

_zn_link_p_result_t _zn_open_link(const z_str_t locator, const clock_t tout);
_zn_link_p_result_t _zn_listen_link(const z_str_t locator, const clock_t tout);

_zn_link_t *_zn_new_link_tcp(_zn_endpoint_t endpoint);
_zn_link_t *_zn_new_link_udp_unicast(_zn_endpoint_t endpoint);
_zn_link_t *_zn_new_link_udp_multicast(_zn_endpoint_t endpoint);

void _zn_link_free(_zn_link_t **zn);

#endif /* ZENOH_PICO_LINK_MANAGER_H */
