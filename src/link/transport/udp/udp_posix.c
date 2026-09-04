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

#if defined(ZP_PLATFORM_SOCKET_POSIX)

#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static z_result_t _z_udp_posix_endpoint_init(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
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
    }

    return ret;
}

static void _z_udp_posix_endpoint_clear(_z_sys_net_endpoint_t *ep) { freeaddrinfo(ep->_iptcp); }

static z_result_t _z_udp_posix_configure_interface(_z_sys_net_socket_t sock, const _z_sys_net_endpoint_t endpoint,
                                                   const char *iface);

static z_result_t _z_udp_posix_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout,
                                    const char *iface) {
    z_result_t ret = _Z_RES_OK;

    sock->_fd = socket(endpoint._iptcp->ai_family, endpoint._iptcp->ai_socktype, endpoint._iptcp->ai_protocol);
    if (sock->_fd != -1) {
        z_time_t tv;
        tv.tv_sec = (time_t)(tout / (uint32_t)1000);
        tv.tv_usec = (suseconds_t)((tout % (uint32_t)1000) * (uint32_t)1000);
        if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
        _Z_SET_IF_OK(ret, _z_udp_posix_configure_interface(*sock, endpoint, iface));

        if (ret != _Z_RES_OK) {
            close(sock->_fd);
            sock->_fd = -1;
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static z_result_t _z_udp_posix_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(endpoint);
    _ZP_UNUSED(tout);
    _Z_ERROR_LOG(_Z_ERR_GENERIC);
    return _Z_ERR_GENERIC;
}

static void _z_udp_posix_close(_z_sys_net_socket_t *sock) {
    if (sock->_fd >= 0) {
        close(sock->_fd);
        sock->_fd = -1;
    }
}

static size_t _z_udp_posix_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    struct sockaddr_storage raddr;
    unsigned int addrlen = sizeof(struct sockaddr_storage);

    ssize_t rb = recvfrom(sock._fd, ptr, len, 0, (struct sockaddr *)&raddr, &addrlen);
    if (rb < (ssize_t)0) {
        return SIZE_MAX;
    }
    return (size_t)rb;
}

static size_t _z_udp_posix_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_udp_posix_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n += rb;
        pos = _z_ptr_u8_offset(pos, (ptrdiff_t)rb);
    } while (n != len);

    return n;
}

static size_t _z_udp_posix_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                                 const _z_sys_net_endpoint_t endpoint) {
    return (size_t)sendto(sock._fd, ptr, len, 0, endpoint._iptcp->ai_addr, endpoint._iptcp->ai_addrlen);
}

static bool _z_udp_posix_iface_was_visited(const struct ifaddrs *head, const struct ifaddrs *current, int family) {
    for (const struct ifaddrs *it = head; it != current; it = it->ifa_next) {
        if ((it->ifa_addr != NULL) && (it->ifa_addr->sa_family == family) &&
            (strcmp(it->ifa_name, current->ifa_name) == 0)) {
            return true;
        }
    }
    return false;
}

static bool _z_udp_posix_select_interface(_z_sys_net_socket_t sock, const _z_sys_net_endpoint_t endpoint,
                                          const struct ifaddrs *iface) {
    int ret = -1;
    if (endpoint._iptcp->ai_family == AF_INET) {
        const struct in_addr *addr = &((const struct sockaddr_in *)iface->ifa_addr)->sin_addr;
        ret = setsockopt(sock._fd, IPPROTO_IP, IP_MULTICAST_IF, addr, sizeof(*addr));
    } else if (endpoint._iptcp->ai_family == AF_INET6) {
        unsigned int ifindex = if_nametoindex(iface->ifa_name);
        if (ifindex != 0U) {
            ret = setsockopt(sock._fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex));
        }
    }
    return ret == 0;
}

static z_result_t _z_udp_posix_configure_interface(_z_sys_net_socket_t sock, const _z_sys_net_endpoint_t endpoint,
                                                   const char *iface) {
    if (iface == NULL) {
        return _Z_RES_OK;
    }

    z_result_t ret = _Z_ERR_GENERIC;
    struct ifaddrs *ifaddrs = NULL;
    if (getifaddrs(&ifaddrs) != 0) {
        return ret;
    }
    for (const struct ifaddrs *it = ifaddrs; it != NULL; it = it->ifa_next) {
        if ((it->ifa_addr == NULL) || (it->ifa_addr->sa_family != endpoint._iptcp->ai_family) ||
            ((it->ifa_flags & IFF_UP) == 0U) || (strcmp(it->ifa_name, iface) != 0)) {
            continue;
        }

        struct sockaddr_storage local;
        socklen_t addrlen = 0;
        (void)memset(&local, 0, sizeof(local));
        if (endpoint._iptcp->ai_family == AF_INET) {
            // Flawfinder: ignore [CWE-120]
            (void)memcpy(&local, it->ifa_addr, sizeof(struct sockaddr_in));
            ((struct sockaddr_in *)&local)->sin_port = 0;
            addrlen = sizeof(struct sockaddr_in);
        } else if (endpoint._iptcp->ai_family == AF_INET6) {
            // Flawfinder: ignore [CWE-120]
            (void)memcpy(&local, it->ifa_addr, sizeof(struct sockaddr_in6));
            ((struct sockaddr_in6 *)&local)->sin6_port = 0;
            addrlen = sizeof(struct sockaddr_in6);
        }
        if ((addrlen != 0U) && (bind(sock._fd, (struct sockaddr *)&local, addrlen) == 0) &&
            _z_udp_posix_select_interface(sock, endpoint, it)) {
            ret = _Z_RES_OK;
        }
        break;
    }
    freeifaddrs(ifaddrs);
    return ret;
}

z_result_t _z_udp_unicast_interface_iterator_init(_z_udp_unicast_interface_iterator_t *iter,
                                                  const _z_string_t *address) {
    iter->_head = NULL;
    iter->_current = NULL;
    iter->_family = 0;
    iter->_started = false;

    _z_sys_net_endpoint_t endpoint;
    z_result_t ret = _z_udp_unicast_endpoint_init_from_address(&endpoint, address);
    if (ret != _Z_RES_OK) {
        return ret;
    }

    iter->_family = endpoint._iptcp->ai_family;
    _z_udp_unicast_endpoint_clear(&endpoint);

    if (getifaddrs(&iter->_head) != 0) {
        return _Z_ERR_GENERIC;
    }
    return _Z_RES_OK;
}

bool _z_udp_unicast_interface_iterator_next(_z_udp_unicast_interface_iterator_t *iter) {
    if (iter->_started && (iter->_current == NULL)) {
        return false;
    }
    iter->_current = iter->_started ? iter->_current->ifa_next : iter->_head;
    iter->_started = true;
    while ((iter->_current != NULL) &&
           ((iter->_current->ifa_addr == NULL) || (iter->_current->ifa_addr->sa_family != iter->_family) ||
            ((iter->_current->ifa_flags & IFF_UP) == 0U) ||
            _z_udp_posix_iface_was_visited(iter->_head, iter->_current, iter->_family))) {
        iter->_current = iter->_current->ifa_next;
    }
    return iter->_current != NULL;
}

const char *_z_udp_unicast_interface_iterator_deref(const _z_udp_unicast_interface_iterator_t *iter) {
    return iter->_current != NULL ? iter->_current->ifa_name : NULL;
}

void _z_udp_unicast_interface_iterator_clear(_z_udp_unicast_interface_iterator_t *iter) {
    if (iter->_head != NULL) {
        freeifaddrs(iter->_head);
        iter->_head = NULL;
        iter->_current = NULL;
    }
}

z_result_t _z_udp_unicast_endpoint_init(_z_sys_net_endpoint_t *ep, const char *address, const char *port) {
    return _z_udp_posix_endpoint_init(ep, address, port);
}

void _z_udp_unicast_endpoint_clear(_z_sys_net_endpoint_t *ep) { _z_udp_posix_endpoint_clear(ep); }

z_result_t _z_udp_unicast_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout,
                               const char *iface) {
    return _z_udp_posix_open(sock, endpoint, tout, iface);
}

z_result_t _z_udp_unicast_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    return _z_udp_posix_listen(sock, endpoint, tout);
}

void _z_udp_unicast_close(_z_sys_net_socket_t *sock) { _z_udp_posix_close(sock); }

size_t _z_udp_unicast_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    return _z_udp_posix_read(sock, ptr, len);
}

size_t _z_udp_unicast_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    return _z_udp_posix_read_exact(sock, ptr, len);
}

size_t _z_udp_unicast_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                            const _z_sys_net_endpoint_t endpoint) {
    return _z_udp_posix_write(sock, ptr, len, endpoint);
}

#endif /* defined(ZP_PLATFORM_SOCKET_POSIX) */
