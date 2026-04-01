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

#ifndef ZENOH_PICO_LINK_BACKEND_DEFAULT_OPS_H
#define ZENOH_PICO_LINK_BACKEND_DEFAULT_OPS_H

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/backend/serial.h"
#include "zenoh-pico/link/backend/tcp.h"
#include "zenoh-pico/link/backend/udp_unicast.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_TLS == 1 || Z_FEATURE_LINK_WS == 1) && defined(ZP_DEFAULT_TCP_OPS)
extern const _z_tcp_ops_t ZP_DEFAULT_TCP_OPS;
#endif

#if (Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1) && defined(ZP_DEFAULT_UDP_UNICAST_OPS)
extern const _z_udp_unicast_ops_t ZP_DEFAULT_UDP_UNICAST_OPS;
#endif

#if Z_FEATURE_LINK_SERIAL == 1 && defined(ZP_DEFAULT_SERIAL_OPS)
extern const _z_serial_ops_t ZP_DEFAULT_SERIAL_OPS;
#endif

#ifdef __cplusplus
}
#endif

static inline const _z_tcp_ops_t *_z_default_tcp_ops(void) {
#if (Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_TLS == 1 || Z_FEATURE_LINK_WS == 1) && defined(ZP_DEFAULT_TCP_OPS)
    return &ZP_DEFAULT_TCP_OPS;
#else
    return NULL;
#endif
}

static inline const _z_udp_unicast_ops_t *_z_default_udp_unicast_ops(void) {
#if (Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1) && defined(ZP_DEFAULT_UDP_UNICAST_OPS)
    return &ZP_DEFAULT_UDP_UNICAST_OPS;
#else
    return NULL;
#endif
}

static inline const _z_serial_ops_t *_z_default_serial_ops(void) {
#if Z_FEATURE_LINK_SERIAL == 1 && defined(ZP_DEFAULT_SERIAL_OPS)
    return &ZP_DEFAULT_SERIAL_OPS;
#else
    return NULL;
#endif
}

#endif /* ZENOH_PICO_LINK_BACKEND_DEFAULT_OPS_H */
