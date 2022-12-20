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

#include <windows.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_LINK_TCP == 1

/*------------------ TCP sockets ------------------*/
int8_t _z_create_endpoint_tcp(_z_sys_net_endpoint_t *ep, const char *s_addr, const char *s_port) {
    (void)(ep);
    (void)(s_addr);
    (void)(s_port);
    return -1;
}

void _z_free_endpoint_tcp(_z_sys_net_endpoint_t *ep) { (void)(ep); }

/*------------------ TCP sockets ------------------*/
int8_t _z_open_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    (void)(sock);
    (void)(rep);
    (void)(tout);
    return -1;
}

int8_t _z_listen_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep) {
    (void)sock;
    (void)lep;
    return -1;
}

void _z_close_tcp(_z_sys_net_socket_t *sock) {
    (void)sock;
    return -1;
}

size_t _z_read_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    (void)(sock);
    (void)(ptr);
    (void)(len);
    return -1;
}

size_t _z_read_exact_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    (void)(sock);
    (void)(ptr);
    (void)(len);
    return -1;
}

size_t _z_send_tcp(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    (void)(sock);
    (void)(ptr);
    (void)(len);
    return -1;
}
#endif

#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
int8_t _z_create_endpoint_udp(_z_sys_net_endpoint_t *ep, const char *s_addr, const char *s_port) {
    (void)(ep);
    (void)(s_addr);
    (void)(s_port);
    return -1;
}

void _z_free_endpoint_udp(_z_sys_net_endpoint_t *ep) { (void)(ep); }
#endif

#if Z_LINK_UDP_UNICAST == 1
int8_t _z_open_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    (void)sock;
    (void)lep;
    (void)tout;
    return -1;
}

int8_t _z_listen_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep, uint32_t tout) {
    (void)sock;
    (void)lep;
    (void)tout;
    return -1;
}

void _z_close_udp_unicast(_z_sys_net_socket_t *sock) { (void)(sock); }

size_t _z_read_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    (void)(sock);
    (void)(ptr);
    (void)(len);
    return -1;
}

size_t _z_read_exact_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    (void)(sock);
    (void)(ptr);
    (void)(len);
    return -1;
}

size_t _z_send_udp_unicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                           const _z_sys_net_endpoint_t rep) {
    (void)(sock);
    (void)(ptr);
    (void)(len);
    (void)(rep);
    return -1;
}
#endif

#if Z_LINK_UDP_MULTICAST == 1
int8_t _z_open_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                             uint32_t tout, const char *iface) {
    (void)(sock);
    (void)(rep);
    (void)(tout);
    (void)(iface);
    return -1;
}

int8_t _z_listen_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                               const char *iface) {
    (void)(sock);
    (void)(rep);
    (void)(tout);
    (void)(iface);
    return -1;
}

void _z_close_udp_multicast(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend,
                            const _z_sys_net_endpoint_t rep) {
    (void)(sockrecv);
    (void)(socksend);
    (void)(rep);
}

size_t _z_read_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_bytes_t *addr) {
    (void)(sock);
    (void)(ptr);
    (void)(len);
    (void)(lep);
    (void)(addr);
    return -1;
}

size_t _z_read_exact_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len,
                                   const _z_sys_net_endpoint_t lep, _z_bytes_t *addr) {
    (void)(sock);
    (void)(ptr);
    (void)(len);
    (void)(lep);
    (void)(addr);
    return -1;
}

size_t _z_send_udp_multicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                             const _z_sys_net_endpoint_t rep) {
    (void)(sock);
    (void)(ptr);
    (void)(len);
    (void)(rep);
    return -1;
}

#endif

#if Z_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on Windows port of Zenoh-Pico"
#endif

#if Z_LINK_SERIAL == 1
#error "Serial not supported yet on Windows port of Zenoh-Pico"
#endif
