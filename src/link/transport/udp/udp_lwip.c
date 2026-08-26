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

#include "zenoh-pico/link/transport/udp_unicast.h"

#if defined(ZP_PLATFORM_SOCKET_LWIP) && (Z_FEATURE_LINK_UDP_UNICAST == 1)

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "lwip/ip4_addr.h"
#include "lwip/netdb.h"
#include "lwip/netif.h"
#include "lwip/sockets.h"
#include "zenoh-pico/link/transport/lwip_socket.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static z_result_t _z_udp_lwip_endpoint_init(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    struct addrinfo hints;
    ep->_iptcp = NULL;
    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(s_address, s_port, &hints, &ep->_iptcp) != 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    } else if (ep->_iptcp != NULL && ep->_iptcp->ai_addr != NULL) {
        ep->_iptcp->ai_addr->sa_family = ep->_iptcp->ai_family;
    }

    return ret;
}

static void _z_udp_lwip_endpoint_clear(_z_sys_net_endpoint_t *ep) { freeaddrinfo(ep->_iptcp); }

static z_result_t _z_udp_lwip_configure_interface(_z_sys_net_socket_t sock, const _z_sys_net_endpoint_t endpoint,
                                                  const char *iface);

static z_result_t _z_udp_lwip_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout,
                                   const char *iface) {
    z_result_t ret = _Z_RES_OK;

    _z_lwip_socket_set(sock,
                       socket(endpoint._iptcp->ai_family, endpoint._iptcp->ai_socktype, endpoint._iptcp->ai_protocol));
    if (_z_lwip_socket_get(*sock) != -1) {
        z_time_t tv;
        tv.tv_sec = tout / (uint32_t)1000;
        tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(_z_lwip_socket_get(*sock), SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
        _Z_SET_IF_OK(ret, _z_udp_lwip_configure_interface(*sock, endpoint, iface));

        if (ret != _Z_RES_OK) {
            close(_z_lwip_socket_get(*sock));
            _z_lwip_socket_set(sock, -1);
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static z_result_t _z_udp_lwip_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(endpoint);
    _ZP_UNUSED(tout);
    _Z_ERROR_LOG(_Z_ERR_GENERIC);
    return _Z_ERR_GENERIC;
}

static void _z_udp_lwip_close(_z_sys_net_socket_t *sock) {
    if (_z_lwip_socket_get(*sock) >= 0) {
        close(_z_lwip_socket_get(*sock));
        _z_lwip_socket_set(sock, -1);
    }
}

static size_t _z_udp_lwip_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    struct sockaddr_storage raddr;
    unsigned int addrlen = sizeof(struct sockaddr_storage);

    ssize_t rb = recvfrom(_z_lwip_socket_get(sock), ptr, len, 0, (struct sockaddr *)&raddr, &addrlen);
    if (rb < (ssize_t)0) {
        return SIZE_MAX;
    }
    return (size_t)rb;
}

static size_t _z_udp_lwip_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_udp_lwip_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, (ptrdiff_t)rb);
    } while (n != len);

    return n;
}

static size_t _z_udp_lwip_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                                const _z_sys_net_endpoint_t endpoint) {
    return (size_t)sendto(_z_lwip_socket_get(sock), ptr, len, 0, endpoint._iptcp->ai_addr, endpoint._iptcp->ai_addrlen);
}

static bool _z_udp_lwip_select_interface(_z_sys_net_socket_t sock, const _z_sys_net_endpoint_t endpoint,
                                         const struct netif *iface) {
    int ret = -1;
#if LWIP_IPV4
    if (endpoint._iptcp->ai_family == AF_INET) {
        struct in_addr addr;
        inet_addr_from_ip4addr(&addr, netif_ip4_addr(iface));
        ret = setsockopt(_z_lwip_socket_get(sock), IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof(addr));
    }
#endif
#if LWIP_IPV6 && defined(IPV6_MULTICAST_IF)
    if (endpoint._iptcp->ai_family == AF_INET6) {
        unsigned int ifindex = netif_get_index(iface);
        ret = setsockopt(_z_lwip_socket_get(sock), IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex));
    }
#endif
    return ret == 0;
}

static bool _z_udp_lwip_family_supported(int family) {
#if LWIP_IPV4
    if (family == AF_INET) {
        return true;
    }
#endif
#if LWIP_IPV6 && defined(IPV6_MULTICAST_IF)
    if (family == AF_INET6) {
        return true;
    }
#endif
    return false;
}

static z_result_t _z_udp_lwip_configure_interface(_z_sys_net_socket_t sock, const _z_sys_net_endpoint_t endpoint,
                                                  const char *iface) {
    if (iface == NULL) {
        return _Z_RES_OK;
    }

    struct netif *selected = netif_find(iface);
    if ((selected == NULL) || !netif_is_up(selected) || !netif_is_link_up(selected) ||
        !_z_udp_lwip_family_supported(endpoint._iptcp->ai_family)) {
        return _Z_ERR_GENERIC;
    }

    struct sockaddr_storage local;
    socklen_t addrlen = 0;
    (void)memset(&local, 0, sizeof(local));
#if LWIP_IPV4
    if (endpoint._iptcp->ai_family == AF_INET) {
        struct sockaddr_in *local4 = (struct sockaddr_in *)&local;
        local4->sin_family = AF_INET;
        if (ip4_addr_isany_val(*netif_ip4_addr(selected))) {
            return _Z_ERR_NOT_READY;
        }
        inet_addr_from_ip4addr(&local4->sin_addr, netif_ip4_addr(selected));
        addrlen = sizeof(*local4);
    }
#endif
#if LWIP_IPV6 && defined(IPV6_MULTICAST_IF)
    if (endpoint._iptcp->ai_family == AF_INET6) {
        struct sockaddr_in6 *local6 = (struct sockaddr_in6 *)&local;
        // Let lwIP choose a valid source address from the netif selected via IPV6_MULTICAST_IF.
        local6->sin6_family = AF_INET6;
        addrlen = sizeof(*local6);
    }
#endif
    return (addrlen != 0U) && (bind(_z_lwip_socket_get(sock), (struct sockaddr *)&local, addrlen) == 0) &&
                   _z_udp_lwip_select_interface(sock, endpoint, selected)
               ? _Z_RES_OK
               : _Z_ERR_GENERIC;
}

z_result_t _z_udp_unicast_interface_iterator_init(_z_udp_unicast_interface_iterator_t *iter,
                                                  const _z_string_t *address) {
    iter->_current = NULL;
    iter->_family = 0;
    iter->_name[0] = '\0';
    iter->_started = false;

    _z_sys_net_endpoint_t endpoint;
    z_result_t ret = _z_udp_unicast_endpoint_init_from_address(&endpoint, address);
    if (ret != _Z_RES_OK) {
        return ret;
    }

    iter->_family = endpoint._iptcp->ai_family;
    _z_udp_unicast_endpoint_clear(&endpoint);
    return _Z_RES_OK;
}

bool _z_udp_unicast_interface_iterator_next(_z_udp_unicast_interface_iterator_t *iter) {
#if LWIP_SINGLE_NETIF
    if (iter->_started) {
        iter->_current = NULL;
        return false;
    }
    iter->_current = netif_default;
    iter->_started = true;
    if ((iter->_current != NULL) && (!netif_is_up(iter->_current) || !netif_is_link_up(iter->_current) ||
                                     !_z_udp_lwip_family_supported(iter->_family))) {
        iter->_current = NULL;
    }
#else
    if (iter->_started && (iter->_current == NULL)) {
        return false;
    }
    iter->_current = iter->_started ? iter->_current->next : netif_list;
    iter->_started = true;
    while ((iter->_current != NULL) && (!netif_is_up(iter->_current) || !netif_is_link_up(iter->_current) ||
                                        !_z_udp_lwip_family_supported(iter->_family))) {
        iter->_current = iter->_current->next;
    }
#endif
    if (iter->_current != NULL) {
        (void)snprintf(iter->_name, sizeof(iter->_name), "%c%c%u", iter->_current->name[0], iter->_current->name[1],
                       (unsigned int)iter->_current->num);
    }
    return iter->_current != NULL;
}

const char *_z_udp_unicast_interface_iterator_deref(const _z_udp_unicast_interface_iterator_t *iter) {
    return iter->_current != NULL ? iter->_name : NULL;
}

void _z_udp_unicast_interface_iterator_clear(_z_udp_unicast_interface_iterator_t *iter) {
    iter->_current = NULL;
    iter->_started = true;
}

z_result_t _z_udp_unicast_endpoint_init(_z_sys_net_endpoint_t *ep, const char *address, const char *port) {
    return _z_udp_lwip_endpoint_init(ep, address, port);
}

void _z_udp_unicast_endpoint_clear(_z_sys_net_endpoint_t *ep) { _z_udp_lwip_endpoint_clear(ep); }

z_result_t _z_udp_unicast_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout,
                               const char *iface) {
    return _z_udp_lwip_open(sock, endpoint, tout, iface);
}

z_result_t _z_udp_unicast_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    return _z_udp_lwip_listen(sock, endpoint, tout);
}

void _z_udp_unicast_close(_z_sys_net_socket_t *sock) { _z_udp_lwip_close(sock); }

size_t _z_udp_unicast_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    return _z_udp_lwip_read(sock, ptr, len);
}

size_t _z_udp_unicast_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    return _z_udp_lwip_read_exact(sock, ptr, len);
}

size_t _z_udp_unicast_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                            const _z_sys_net_endpoint_t endpoint) {
    return _z_udp_lwip_write(sock, ptr, len, endpoint);
}

#endif /* defined(ZP_PLATFORM_SOCKET_LWIP) */
