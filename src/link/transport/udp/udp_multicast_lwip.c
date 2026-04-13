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

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/transport/udp_multicast.h"

#if defined(ZENOH_FREERTOS_LWIP) && (Z_FEATURE_LINK_UDP_MULTICAST == 1)

#include <string.h>

#include "lwip/ip4_addr.h"
#include "lwip/netdb.h"
#include "lwip/netif.h"
#include "udp_multicast_lwip_common.h"

static unsigned long __get_ip_from_iface(const char *iface, int sa_family, struct sockaddr **lsockaddr) {
    unsigned int addrlen = 0U;
    _ZP_UNUSED(sa_family);

    struct netif *netif = netif_find(iface);
    if (netif != NULL && netif_is_up(netif)) {
        struct sockaddr_in *lsockaddr_in = (struct sockaddr_in *)z_malloc(sizeof(struct sockaddr_in));
        if (lsockaddr != NULL) {
            (void)memset(lsockaddr_in, 0, sizeof(struct sockaddr_in));
            const ip4_addr_t *ip4_addr = netif_ip4_addr(netif);
            inet_addr_from_ip4addr(&lsockaddr_in->sin_addr, ip_2_ip4(ip4_addr));
            lsockaddr_in->sin_family = AF_INET;
            lsockaddr_in->sin_port = htons(0);

            addrlen = sizeof(struct sockaddr_in);
            *lsockaddr = (struct sockaddr *)lsockaddr_in;
        }
    }

    return addrlen;
}

static z_result_t _z_open_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep,
                                        _z_sys_net_endpoint_t *lep, uint32_t tout, const char *iface) {
    return _z_lwip_udp_multicast_open(sock, rep, lep, tout, iface, __get_ip_from_iface);
}

static z_result_t _z_listen_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                                          const char *iface, const char *join) {
    return _z_lwip_udp_multicast_listen(sock, rep, tout, iface, join, __get_ip_from_iface);
}

z_result_t _z_udp_multicast_endpoint_init_from_address(_z_sys_net_endpoint_t *ep, const _z_string_t *address) {
    return _z_udp_multicast_default_endpoint_init_from_address(ep, address);
}

void _z_udp_multicast_endpoint_clear(_z_sys_net_endpoint_t *ep) { _z_udp_multicast_default_endpoint_clear(ep); }

z_result_t _z_udp_multicast_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                                 uint32_t tout, const char *iface) {
    return _z_open_udp_multicast(sock, rep, lep, tout, iface);
}

z_result_t _z_udp_multicast_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                                   const char *iface, const char *join) {
    return _z_listen_udp_multicast(sock, rep, tout, iface, join);
}

void _z_udp_multicast_close(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend,
                            const _z_sys_net_endpoint_t rep, const _z_sys_net_endpoint_t lep) {
    _z_lwip_udp_multicast_close(sockrecv, socksend, rep, lep);
}

size_t _z_udp_multicast_read_exact(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len,
                                   const _z_sys_net_endpoint_t lep, _z_slice_t *ep) {
    return _z_lwip_udp_multicast_read_exact(sock, ptr, len, lep, ep);
}

size_t _z_udp_multicast_read(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_slice_t *ep) {
    return _z_lwip_udp_multicast_read(sock, ptr, len, lep, ep);
}

size_t _z_udp_multicast_write(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                              const _z_sys_net_endpoint_t rep) {
    return _z_lwip_udp_multicast_write(sock, ptr, len, rep);
}

#endif
