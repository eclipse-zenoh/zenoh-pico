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

#ifndef ZENOH_PICO_LINK_BACKEND_UDP_MULTICAST_H
#define ZENOH_PICO_LINK_BACKEND_UDP_MULTICAST_H

#include <stdint.h>

#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/link/backend/udp_unicast.h"

#ifdef __cplusplus
extern "C" {
#endif

#if Z_FEATURE_LINK_UDP_MULTICAST == 1

static inline z_result_t _z_udp_multicast_default_endpoint_init_from_address(_z_sys_net_endpoint_t *ep,
                                                                             const _z_string_t *address) {
    return _z_udp_unicast_endpoint_init_from_address(ep, address);
}

static inline void _z_udp_multicast_default_endpoint_clear(_z_sys_net_endpoint_t *ep) {
    _z_udp_unicast_endpoint_clear(ep);
}

z_result_t _z_udp_multicast_endpoint_init_from_address(_z_sys_net_endpoint_t *ep, const _z_string_t *address);
void _z_udp_multicast_endpoint_clear(_z_sys_net_endpoint_t *ep);

// flawfinder: ignore
z_result_t _z_udp_multicast_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                                 uint32_t tout, const char *iface);
z_result_t _z_udp_multicast_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                                   const char *iface, const char *join);
void _z_udp_multicast_close(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend,
                            const _z_sys_net_endpoint_t rep, const _z_sys_net_endpoint_t lep);

size_t _z_udp_multicast_read_exact(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len,
                                   const _z_sys_net_endpoint_t lep, _z_slice_t *ep);
// flawfinder: ignore
size_t _z_udp_multicast_read(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_slice_t *ep);
size_t _z_udp_multicast_write(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                              const _z_sys_net_endpoint_t rep);

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_BACKEND_UDP_MULTICAST_H */
