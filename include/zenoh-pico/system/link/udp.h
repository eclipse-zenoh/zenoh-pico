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

#if ZN_LINK_UDP_UNICAST == 1 || ZN_LINK_UDP_MULTICAST == 1

typedef struct
{
    void *sock;
    void *msock;
    void *raddr;
    void *laddr;
} _zn_udp_socket_t;

void *_zn_create_endpoint_udp(const z_str_t s_addr, const z_str_t port);
void _zn_free_endpoint_udp(void *addr_arg);

// Unicast
void *_zn_open_udp_unicast(void *raddr_arg, const clock_t tout);
void *_zn_listen_udp_unicast(void *raddr_arg, const clock_t tout);
void _zn_close_udp_unicast(void *sock_arg);
size_t _zn_read_exact_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len);
size_t _zn_read_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len);
size_t _zn_send_udp_unicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg);

// Multicast
void *_zn_open_udp_multicast(void *raddr_arg, void **laddr_arg, const clock_t tout, const z_str_t iface);
void *_zn_listen_udp_multicast(void *raddr_arg, const clock_t tout, const z_str_t iface);
void _zn_close_udp_multicast(void *sockrecv_arg, void *socksend_arg, void *raddr_arg);
size_t _zn_read_exact_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *laddr_arg, z_bytes_t *addr);
size_t _zn_read_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *laddr_arg, z_bytes_t *addr);
size_t _zn_send_udp_multicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg);
#endif

#endif /* ZENOH_PICO_SYSTEM_LINK_UDP_H */
