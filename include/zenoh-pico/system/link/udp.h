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

#ifndef ZENOH_PICO_SYSTEM_LINK_UDP_H
#define ZENOH_PICO_SYSTEM_LINK_UDP_H

#include <stdint.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/system/platform.h"

#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1

typedef struct {
    _z_sys_net_socket_t _sock;
    _z_sys_net_socket_t _msock;
    _z_sys_net_endpoint_t _rep;
    _z_sys_net_endpoint_t _lep;
} _z_udp_socket_t;

int8_t _z_create_endpoint_udp(_z_sys_net_endpoint_t *ep, const char *s_addr, const char *s_port);
void _z_free_endpoint_udp(_z_sys_net_endpoint_t *ep);

// Unicast
int8_t _z_open_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout);
int8_t _z_listen_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout);
void _z_close_udp_unicast(_z_sys_net_socket_t *sock);
size_t _z_read_exact_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len);
size_t _z_read_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len);
size_t _z_send_udp_unicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                           const _z_sys_net_endpoint_t rep);

// Multicast
int8_t _z_open_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                             uint32_t tout, const char *iface);
int8_t _z_listen_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout, const char *iface);
void _z_close_udp_multicast(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend, const _z_sys_net_endpoint_t rep);
size_t _z_read_exact_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                                   _z_bytes_t *ep);
size_t _z_read_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_bytes_t *ep);
size_t _z_send_udp_multicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t rep);
#endif

#endif /* ZENOH_PICO_SYSTEM_LINK_UDP_H */
