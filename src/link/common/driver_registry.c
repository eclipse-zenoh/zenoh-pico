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

#include "zenoh-pico/link/driver_registry.h"

#include <stddef.h>

#include "zenoh-pico/config.h"

#if Z_FEATURE_LINK_TCP == 1
extern const _z_link_driver_t _z_link_driver_tcp;
#endif
#if Z_FEATURE_LINK_TLS == 1
extern const _z_link_driver_t _z_link_driver_tls;
#endif
#if Z_FEATURE_LINK_UDP_UNICAST == 1
extern const _z_link_driver_t _z_link_driver_udp_unicast;
#endif
#if Z_FEATURE_LINK_UDP_MULTICAST == 1
extern const _z_link_driver_t _z_link_driver_udp_multicast;
#endif
#if Z_FEATURE_LINK_BLUETOOTH == 1
extern const _z_link_driver_t _z_link_driver_bt;
#endif
#if Z_FEATURE_LINK_SERIAL == 1
extern const _z_link_driver_t _z_link_driver_serial;
#endif
#if Z_FEATURE_LINK_WS == 1
extern const _z_link_driver_t _z_link_driver_ws;
#endif
#if Z_FEATURE_RAWETH_TRANSPORT == 1
extern const _z_link_driver_t _z_link_driver_raweth;
#endif

/*
 * Ordered link driver dispatch table.
 *
 * Drivers are tried in order. For a given open/listen operation, the first
 * operation-capable driver whose validator accepts the endpoint is selected.
 */
static const _z_link_driver_t *const _z_link_drivers[] = {
#if Z_FEATURE_LINK_TCP == 1
    &_z_link_driver_tcp,
#endif
#if Z_FEATURE_LINK_TLS == 1
    &_z_link_driver_tls,
#endif
#if Z_FEATURE_LINK_UDP_UNICAST == 1
    &_z_link_driver_udp_unicast,
#endif
#if Z_FEATURE_LINK_UDP_MULTICAST == 1
    &_z_link_driver_udp_multicast,
#endif
#if Z_FEATURE_LINK_BLUETOOTH == 1
    &_z_link_driver_bt,
#endif
#if Z_FEATURE_LINK_SERIAL == 1
    &_z_link_driver_serial,
#endif
#if Z_FEATURE_LINK_WS == 1
    &_z_link_driver_ws,
#endif
#if Z_FEATURE_RAWETH_TRANSPORT == 1
    &_z_link_driver_raweth,
#endif
    NULL,
};

const _z_link_driver_t *const *_z_link_driver_registry(void) { return _z_link_drivers; }
