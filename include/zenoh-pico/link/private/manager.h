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

#include "zenoh-pico/link/private/result.h"
#include "zenoh-pico/link/types.h"

_zn_link_p_result_t _zn_open_link(const char *locator, const clock_t tout);
void _zn_close_link(_zn_link_t *link);

_zn_link_t *_zn_new_link_tcp(const char *s_addr, const char *port);
_zn_link_t *_zn_new_link_udp(const char *s_addr, const char *port);

#endif /* _ZENOH_PICO_TRANSPORT_PRIVATE_LINK_MANAGER_H */
