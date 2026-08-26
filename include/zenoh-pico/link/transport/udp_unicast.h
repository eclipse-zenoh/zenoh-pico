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

#ifndef ZENOH_PICO_LINK_TRANSPORT_UDP_UNICAST_H
#define ZENOH_PICO_LINK_TRANSPORT_UDP_UNICAST_H

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/system/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    _z_sys_net_socket_t _sock;
    _z_sys_net_socket_t _msock;
    _z_sys_net_endpoint_t _rep;
    _z_sys_net_endpoint_t _lep;
} _z_udp_socket_t;

z_result_t _z_udp_unicast_address_valid(const _z_string_t *address);
z_result_t _z_udp_unicast_endpoint_init(_z_sys_net_endpoint_t *ep, const char *address, const char *port);
void _z_udp_unicast_endpoint_clear(_z_sys_net_endpoint_t *ep);
z_result_t _z_udp_unicast_endpoint_init_from_address(_z_sys_net_endpoint_t *ep, const _z_string_t *address);

#if defined(ZP_PLATFORM_SOCKET_POSIX)
struct ifaddrs;
typedef struct {
    struct ifaddrs *_head;
    struct ifaddrs *_current;
    int _family;
    bool _started;
} _z_udp_unicast_interface_iterator_t;
#define ZP_PLATFORM_HAS_UDP_INTERFACE_ITERATOR
#elif defined(ZP_PLATFORM_SOCKET_LWIP)
struct netif;
typedef struct {
    struct netif *_current;
    int _family;
    char _name[7];
    bool _started;
} _z_udp_unicast_interface_iterator_t;
#define ZP_PLATFORM_HAS_UDP_INTERFACE_ITERATOR
#endif

#if defined(ZP_PLATFORM_HAS_UDP_INTERFACE_ITERATOR)
z_result_t _z_udp_unicast_interface_iterator_init(_z_udp_unicast_interface_iterator_t *iter,
                                                  const _z_string_t *address);
bool _z_udp_unicast_interface_iterator_next(_z_udp_unicast_interface_iterator_t *iter);
// The returned view remains valid until the iterator is advanced or cleared.
const char *_z_udp_unicast_interface_iterator_deref(const _z_udp_unicast_interface_iterator_t *iter);
void _z_udp_unicast_interface_iterator_clear(_z_udp_unicast_interface_iterator_t *iter);
#endif

// flawfinder: ignore
z_result_t _z_udp_unicast_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout,
                               const char *iface);
z_result_t _z_udp_unicast_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout);
void _z_udp_unicast_close(_z_sys_net_socket_t *sock);

// flawfinder: ignore
size_t _z_udp_unicast_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len);
size_t _z_udp_unicast_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len);
size_t _z_udp_unicast_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                            const _z_sys_net_endpoint_t endpoint);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_TRANSPORT_UDP_UNICAST_H */
