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

#ifndef ZENOH_PICO_LINK_BACKEND_DATAGRAM_UDP_MULTICAST_LWIP_COMMON_H
#define ZENOH_PICO_LINK_BACKEND_DATAGRAM_UDP_MULTICAST_LWIP_COMMON_H

#include "zenoh-pico/link/backend/udp_multicast.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long (*_z_lwip_udp_multicast_iface_addr_fn)(const char *iface, int sa_family,
                                                             struct sockaddr **lsockaddr);

z_result_t _z_lwip_udp_multicast_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep,
                                      _z_sys_net_endpoint_t *lep, uint32_t tout, const char *iface,
                                      _z_lwip_udp_multicast_iface_addr_fn get_ip_from_iface);

z_result_t _z_lwip_udp_multicast_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                                        const char *iface, const char *join,
                                        _z_lwip_udp_multicast_iface_addr_fn get_ip_from_iface);

void _z_lwip_udp_multicast_close(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend,
                                 const _z_sys_net_endpoint_t rep, const _z_sys_net_endpoint_t lep);

size_t _z_lwip_udp_multicast_read(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len,
                                  const _z_sys_net_endpoint_t lep, _z_slice_t *addr);

size_t _z_lwip_udp_multicast_read_exact(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len,
                                        const _z_sys_net_endpoint_t lep, _z_slice_t *addr);

size_t _z_lwip_udp_multicast_write(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                                   const _z_sys_net_endpoint_t rep);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_BACKEND_DATAGRAM_UDP_MULTICAST_LWIP_COMMON_H */
