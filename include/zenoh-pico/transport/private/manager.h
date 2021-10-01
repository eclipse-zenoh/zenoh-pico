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

#ifndef _ZENOH_PICO_TRANSPORT_PRIVATE_LINK_MANAGER_H
#define _ZENOH_PICO_TRANSPORT_PRIVATE_LINK_MANAGER_H

// TODO: to be moved somewhere
#define TCP_SCHEMA "tcp"
#define UDP_SCHEMA "udp"

#include "zenoh-pico/utils/private/result.h"
#include "zenoh-pico/transport/private/link.h"

/*------------------ Result declarations ------------------*/
_ZN_P_RESULT_DECLARE(_zn_link_t, link)

_zn_link_p_result_t _zn_open_link(const char* locator, clock_t tout);
void _zn_close_link(_zn_link_t *link);

_zn_link_t *_zn_new_tcp_link(char* s_addr, int port);
_zn_link_t *_zn_new_udp_link(char* s_addr, int port, clock_t tout);

#endif /* _ZENOH_PICO_TRANSPORT_PRIVATE_LINK_MANAGER_H */
