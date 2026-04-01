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

#include "udp_multicast_lwip_common.h"

#include "zenoh-pico/config.h"

#if defined(ZP_PLATFORM_SOCKET_LWIP) && (Z_FEATURE_LINK_UDP_MULTICAST == 1)

#include <string.h>

#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/link/backend/lwip_socket.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

z_result_t _z_lwip_udp_multicast_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep,
                                      _z_sys_net_endpoint_t *lep, uint32_t tout, const char *iface,
                                      _z_lwip_udp_multicast_iface_addr_fn get_ip_from_iface) {
    z_result_t ret = _Z_RES_OK;

    struct sockaddr *lsockaddr = NULL;
    unsigned long addrlen = get_ip_from_iface(iface, rep._iptcp->ai_family, &lsockaddr);
    if (addrlen != 0U) {
        _z_lwip_socket_set(sock, socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol));
        if (_z_lwip_socket_get(*sock) != -1) {
            z_time_t tv;
            tv.tv_sec = tout / (uint32_t)1000;
            tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
            if ((ret == _Z_RES_OK) &&
                (setsockopt(_z_lwip_socket_get(*sock), SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if ((ret == _Z_RES_OK) && (bind(_z_lwip_socket_get(*sock), lsockaddr, addrlen) < 0)) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if ((ret == _Z_RES_OK) && (getsockname(_z_lwip_socket_get(*sock), lsockaddr, &addrlen) < 0)) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if (ret != _Z_RES_OK) {
                close(_z_lwip_socket_get(*sock));
                _z_lwip_socket_set(sock, -1);
                z_free(lsockaddr);
                return ret;
            }

            struct addrinfo *laddr = (struct addrinfo *)z_malloc(sizeof(struct addrinfo));
            if (laddr == NULL) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                close(_z_lwip_socket_get(*sock));
                _z_lwip_socket_set(sock, -1);
                z_free(lsockaddr);
                return _Z_ERR_GENERIC;
            }

            laddr->ai_flags = 0;
            laddr->ai_family = rep._iptcp->ai_family;
            laddr->ai_socktype = rep._iptcp->ai_socktype;
            laddr->ai_protocol = rep._iptcp->ai_protocol;
            laddr->ai_addrlen = addrlen;
            laddr->ai_addr = lsockaddr;
            laddr->ai_canonname = NULL;
            laddr->ai_next = NULL;
            lep->_iptcp = laddr;
        } else {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        if (ret != _Z_RES_OK) {
            z_free(lsockaddr);
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

z_result_t _z_lwip_udp_multicast_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                                        const char *iface, const char *join,
                                        _z_lwip_udp_multicast_iface_addr_fn get_ip_from_iface) {
    z_result_t ret = _Z_RES_OK;

    struct sockaddr *lsockaddr = NULL;
    unsigned int addrlen = get_ip_from_iface(iface, rep._iptcp->ai_family, &lsockaddr);
    if (addrlen != 0U) {
        _z_lwip_socket_set(sock, socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol));
        if (_z_lwip_socket_get(*sock) != -1) {
            z_time_t tv;
            tv.tv_sec = tout / (uint32_t)1000;
            tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
            if ((ret == _Z_RES_OK) &&
                (setsockopt(_z_lwip_socket_get(*sock), SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if (rep._iptcp->ai_family == AF_INET) {
                struct sockaddr_in address;
                (void)memset(&address, 0, sizeof(address));
                address.sin_family = (sa_family_t)rep._iptcp->ai_family;
                address.sin_port = ((struct sockaddr_in *)rep._iptcp->ai_addr)->sin_port;
                inet_pton(address.sin_family, "0.0.0.0", &address.sin_addr);
                if ((ret == _Z_RES_OK) &&
                    (bind(_z_lwip_socket_get(*sock), (struct sockaddr *)&address, sizeof(address)) < 0)) {
                    _Z_ERROR_LOG(_Z_ERR_GENERIC);
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if (rep._iptcp->ai_family == AF_INET) {
                struct ip_mreq mreq;
                (void)memset(&mreq, 0, sizeof(mreq));
                mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)rep._iptcp->ai_addr)->sin_addr.s_addr;
                mreq.imr_interface.s_addr = ((struct sockaddr_in *)lsockaddr)->sin_addr.s_addr;
                if ((ret == _Z_RES_OK) &&
                    (setsockopt(_z_lwip_socket_get(*sock), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)) {
                    _Z_ERROR_LOG(_Z_ERR_GENERIC);
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if (join != NULL) {
                char *joins = _z_str_clone(join);
                char *joins_cursor = joins;
                for (char *ip = strsep(&joins_cursor, "|"); ip != NULL; ip = strsep(&joins_cursor, "|")) {
                    if (rep._iptcp->ai_family == AF_INET) {
                        struct ip_mreq mreq;
                        (void)memset(&mreq, 0, sizeof(mreq));
                        inet_pton(rep._iptcp->ai_family, ip, &mreq.imr_multiaddr);
                        mreq.imr_interface.s_addr = ((struct sockaddr_in *)lsockaddr)->sin_addr.s_addr;
                        if ((ret == _Z_RES_OK) && (setsockopt(_z_lwip_socket_get(*sock), IPPROTO_IP, IP_ADD_MEMBERSHIP,
                                                              &mreq, sizeof(mreq)) < 0)) {
                            _Z_ERROR_LOG(_Z_ERR_GENERIC);
                            ret = _Z_ERR_GENERIC;
                        }
                    }
                }
                z_free(joins);
            }

            if (ret != _Z_RES_OK) {
                close(_z_lwip_socket_get(*sock));
                _z_lwip_socket_set(sock, -1);
            }
        } else {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        z_free(lsockaddr);
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_lwip_udp_multicast_close(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend,
                                 const _z_sys_net_endpoint_t rep, const _z_sys_net_endpoint_t lep) {
    if (_z_lwip_socket_get(*sockrecv) >= 0) {
        if (rep._iptcp->ai_family == AF_INET) {
            struct ip_mreq mreq;
            (void)memset(&mreq, 0, sizeof(mreq));
            mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)rep._iptcp->ai_addr)->sin_addr.s_addr;
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            setsockopt(_z_lwip_socket_get(*sockrecv), IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
        }
    }
    _ZP_UNUSED(lep);

    if (_z_lwip_socket_get(*sockrecv) >= 0) {
        close(_z_lwip_socket_get(*sockrecv));
        _z_lwip_socket_set(sockrecv, -1);
    }
    if (_z_lwip_socket_get(*socksend) >= 0) {
        close(_z_lwip_socket_get(*socksend));
        _z_lwip_socket_set(socksend, -1);
    }
}

size_t _z_lwip_udp_multicast_read(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len,
                                  const _z_sys_net_endpoint_t lep, _z_slice_t *addr) {
    struct sockaddr_storage raddr;
    unsigned long replen = sizeof(struct sockaddr_storage);

    ssize_t rb = 0;
    do {
        rb = recvfrom(_z_lwip_socket_get(sock), ptr, len, 0, (struct sockaddr *)&raddr, &replen);
        if (rb < (ssize_t)0) {
            return SIZE_MAX;
        }

        if (lep._iptcp->ai_family == AF_INET) {
            struct sockaddr_in *a = ((struct sockaddr_in *)lep._iptcp->ai_addr);
            struct sockaddr_in *b = ((struct sockaddr_in *)&raddr);
            if (!((a->sin_port == b->sin_port) && (a->sin_addr.s_addr == b->sin_addr.s_addr))) {
                if (addr != NULL) {
                    addr->len = sizeof(in_addr_t) + sizeof(in_port_t);
                    // flawfinder: ignore
                    (void)memcpy((uint8_t *)addr->start, &b->sin_addr.s_addr, sizeof(in_addr_t));
                    // flawfinder: ignore
                    (void)memcpy((uint8_t *)(addr->start + sizeof(in_addr_t)), &b->sin_port, sizeof(in_port_t));
                }
                break;
            }
        } else {
            continue;
        }
    } while (1);

    return (size_t)rb;
}

size_t _z_lwip_udp_multicast_read_exact(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len,
                                        const _z_sys_net_endpoint_t lep, _z_slice_t *addr) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_lwip_udp_multicast_read(sock, pos, len - n, lep, addr);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, (ptrdiff_t)rb);
    } while (n != len);

    return n;
}

size_t _z_lwip_udp_multicast_write(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                                   const _z_sys_net_endpoint_t rep) {
    return (size_t)sendto(_z_lwip_socket_get(sock), ptr, len, 0, rep._iptcp->ai_addr, rep._iptcp->ai_addrlen);
}

#endif /* defined(ZP_PLATFORM_SOCKET_LWIP) && (Z_FEATURE_LINK_UDP_MULTICAST == 1) */
