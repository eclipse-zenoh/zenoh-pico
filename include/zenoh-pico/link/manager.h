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

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/link.h"

#if Z_FEATURE_LINK_TCP == 1
int8_t _z_endpoint_tcp_valid(_z_endpoint_t *ep);
int8_t _z_new_link_tcp(_z_link_t *zl, _z_endpoint_t *ep);
#endif
#if Z_FEATURE_LINK_UDP_UNICAST == 1
int8_t _z_endpoint_udp_unicast_valid(_z_endpoint_t *ep);
int8_t _z_new_link_udp_unicast(_z_link_t *zl, _z_endpoint_t ep);
#endif
#if Z_FEATURE_LINK_UDP_MULTICAST == 1
int8_t _z_endpoint_udp_multicast_valid(_z_endpoint_t *ep);
int8_t _z_new_link_udp_multicast(_z_link_t *zl, _z_endpoint_t ep);
#endif
#if Z_FEATURE_LINK_BLUETOOTH == 1
int8_t _z_endpoint_bt_valid(_z_endpoint_t *ep);
int8_t _z_new_link_bt(_z_link_t *zl, _z_endpoint_t ep);
#endif
#if Z_FEATURE_LINK_SERIAL == 1
int8_t _z_endpoint_serial_valid(_z_endpoint_t *ep);
int8_t _z_new_link_serial(_z_link_t *zl, _z_endpoint_t ep);
#endif
#if Z_FEATURE_LINK_WS == 1
int8_t _z_endpoint_ws_valid(_z_endpoint_t *ep);
int8_t _z_new_link_ws(_z_link_t *zl, _z_endpoint_t *ep);
#endif

#endif /* ZENOH_PICO_LINK_MANAGER_H */
