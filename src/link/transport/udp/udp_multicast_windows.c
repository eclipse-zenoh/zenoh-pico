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

#if defined(ZP_PLATFORM_SOCKET_WINDOWS) && (Z_FEATURE_LINK_UDP_MULTICAST == 1)

#include <iphlpapi.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static WSADATA _z_udp_multicast_windows_wsa_data;

static unsigned int __get_ip_from_iface(const char *iface, int sa_family, SOCKADDR **lsockaddr) {
    unsigned int addrlen = 0U;

    unsigned long outBufLen = 15000;
    IP_ADAPTER_ADDRESSES *l_ifaddr = (IP_ADAPTER_ADDRESSES *)z_malloc(outBufLen);
    if (l_ifaddr != NULL) {
        if (GetAdaptersAddresses(sa_family, 0, NULL, l_ifaddr, &outBufLen) != ERROR_BUFFER_OVERFLOW) {
            for (IP_ADAPTER_ADDRESSES *tmp = l_ifaddr; tmp != NULL; tmp = tmp->Next) {
                if (_z_str_eq(tmp->AdapterName, iface) == true) {
                    if (sa_family == AF_INET) {
                        *lsockaddr = (SOCKADDR *)z_malloc(sizeof(SOCKADDR_IN));
                        if (lsockaddr != NULL) {
                            (void)memset(*lsockaddr, 0, sizeof(SOCKADDR_IN));
                            // flawfinder: ignore
                            (void)memcpy(*lsockaddr, tmp->FirstUnicastAddress->Address.lpSockaddr, sizeof(SOCKADDR_IN));
                            addrlen = sizeof(SOCKADDR_IN);
                        }
                    } else if (sa_family == AF_INET6) {
                        *lsockaddr = (SOCKADDR *)z_malloc(sizeof(SOCKADDR_IN6));
                        if (lsockaddr != NULL) {
                            (void)memset(*lsockaddr, 0, sizeof(SOCKADDR_IN6));
                            // flawfinder: ignore
                            (void)memcpy(*lsockaddr, tmp->FirstUnicastAddress->Address.lpSockaddr,
                                         sizeof(SOCKADDR_IN6));
                            addrlen = sizeof(SOCKADDR_IN6);
                        }
                    } else {
                        continue;
                    }

                    break;
                }
            }
        }

        z_free(l_ifaddr);
        l_ifaddr = NULL;
    }

    return addrlen;
}

z_result_t _z_open_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                                 uint32_t tout, const char *iface) {
    z_result_t ret = _Z_RES_OK;

    if (WSAStartup(MAKEWORD(2, 2), &_z_udp_multicast_windows_wsa_data) != 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    SOCKADDR *lsockaddr = NULL;
    unsigned int addrlen = __get_ip_from_iface(iface, rep._ep._iptcp->ai_family, &lsockaddr);
    if (addrlen != 0U) {
        sock->_sock._fd = socket(rep._ep._iptcp->ai_family, rep._ep._iptcp->ai_socktype, rep._ep._iptcp->ai_protocol);
        if (sock->_sock._fd != INVALID_SOCKET) {
            DWORD tv = tout;
            if ((ret == _Z_RES_OK) &&
                (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if (rep._ep._iptcp->ai_family == AF_INET) {
                SOCKADDR_IN address = {
                    .sin_family = AF_INET, .sin_port = htons(0), .sin_addr.s_addr = htonl(INADDR_ANY), .sin_zero = {0}};
                if ((ret == _Z_RES_OK) &&
                    (bind(sock->_sock._fd, (SOCKADDR *)&address, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)) {
                    _Z_ERROR_LOG(_Z_ERR_GENERIC);
                    ret = _Z_ERR_GENERIC;
                }
            } else if (rep._ep._iptcp->ai_family == AF_INET6) {
                SOCKADDR_IN6 address = {.sin6_family = AF_INET6, .sin6_port = htons(0), 0, .sin6_addr = {{{0}}}, 0};
                if ((ret == _Z_RES_OK) &&
                    (bind(sock->_sock._fd, (SOCKADDR *)&address, sizeof(SOCKADDR_IN6)) == SOCKET_ERROR)) {
                    _Z_ERROR_LOG(_Z_ERR_GENERIC);
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if ((ret == _Z_RES_OK) && (getsockname(sock->_sock._fd, lsockaddr, (int *)&addrlen) < 0)) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if (rep._ep._iptcp->ai_family == AF_INET) {
                if ((ret == _Z_RES_OK) &&
                    (setsockopt(sock->_sock._fd, IPPROTO_IP, IP_MULTICAST_IF,
                                (const char *)&((SOCKADDR_IN *)lsockaddr)->sin_addr.s_addr, addrlen) < 0)) {
                    _Z_ERROR_LOG(_Z_ERR_GENERIC);
                    ret = _Z_ERR_GENERIC;
                }
            } else if (rep._ep._iptcp->ai_family == AF_INET6) {
                if ((ret == _Z_RES_OK) &&
                    (setsockopt(sock->_sock._fd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                                (const char *)&((SOCKADDR_IN6 *)lsockaddr)->sin6_scope_id, sizeof(ULONG)) < 0)) {
                    _Z_ERROR_LOG(_Z_ERR_GENERIC);
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if (ret != _Z_RES_OK) {
                closesocket(sock->_sock._fd);
                sock->_sock._fd = INVALID_SOCKET;
                WSACleanup();
                z_free(lsockaddr);
                return ret;
            }

            ADDRINFOA *laddr = (ADDRINFOA *)z_malloc(sizeof(ADDRINFOA));
            if (laddr == NULL) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                closesocket(sock->_sock._fd);
                sock->_sock._fd = INVALID_SOCKET;
                WSACleanup();
                z_free(lsockaddr);
                return _Z_ERR_GENERIC;
            }

            laddr->ai_flags = 0;
            laddr->ai_family = rep._ep._iptcp->ai_family;
            laddr->ai_socktype = rep._ep._iptcp->ai_socktype;
            laddr->ai_protocol = rep._ep._iptcp->ai_protocol;
            laddr->ai_addrlen = addrlen;
            laddr->ai_addr = lsockaddr;
            laddr->ai_canonname = NULL;
            laddr->ai_next = NULL;
            lep->_ep._iptcp = laddr;
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

z_result_t _z_listen_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                                   const char *iface, const char *join) {
    (void)join;
    z_result_t ret = _Z_RES_OK;

    if (WSAStartup(MAKEWORD(2, 2), &_z_udp_multicast_windows_wsa_data) != 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    SOCKADDR *lsockaddr = NULL;
    unsigned int addrlen = __get_ip_from_iface(iface, rep._ep._iptcp->ai_family, &lsockaddr);
    if (addrlen != 0U) {
        sock->_sock._fd = socket(rep._ep._iptcp->ai_family, rep._ep._iptcp->ai_socktype, rep._ep._iptcp->ai_protocol);
        if (sock->_sock._fd != INVALID_SOCKET) {
            DWORD tv = tout;
            if ((ret == _Z_RES_OK) &&
                (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            int optflag = 1;
            if ((ret == _Z_RES_OK) &&
                (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optflag, sizeof(optflag)) < 0)) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if (rep._ep._iptcp->ai_family == AF_INET) {
                SOCKADDR_IN address = {.sin_family = AF_INET,
                                       .sin_port = ((SOCKADDR_IN *)rep._ep._iptcp->ai_addr)->sin_port,
                                       .sin_addr = {0},
                                       .sin_zero = {0}};
                if ((ret == _Z_RES_OK) && (bind(sock->_sock._fd, (SOCKADDR *)&address, sizeof address) < 0)) {
                    _Z_ERROR_LOG(_Z_ERR_GENERIC);
                    ret = _Z_ERR_GENERIC;
                }
            } else if (rep._ep._iptcp->ai_family == AF_INET6) {
                SOCKADDR_IN6 address = {.sin6_family = AF_INET6,
                                        .sin6_port = ((SOCKADDR_IN6 *)rep._ep._iptcp->ai_addr)->sin6_port,
                                        0,
                                        .sin6_addr = {{{0}}},
                                        0};
                if ((ret == _Z_RES_OK) && (bind(sock->_sock._fd, (SOCKADDR *)&address, sizeof address) < 0)) {
                    _Z_ERROR_LOG(_Z_ERR_GENERIC);
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if (rep._ep._iptcp->ai_family == AF_INET) {
                struct ip_mreq mreq;
                (void)memset(&mreq, 0, sizeof(mreq));
                mreq.imr_multiaddr.s_addr = ((SOCKADDR_IN *)rep._ep._iptcp->ai_addr)->sin_addr.s_addr;
                mreq.imr_interface.s_addr = ((SOCKADDR_IN *)lsockaddr)->sin_addr.s_addr;
                if ((ret == _Z_RES_OK) && (setsockopt(sock->_sock._fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                                                      (const char *)&mreq, sizeof(mreq)) < 0)) {
                    _Z_ERROR_LOG(_Z_ERR_GENERIC);
                    ret = _Z_ERR_GENERIC;
                }
            } else if (rep._ep._iptcp->ai_family == AF_INET6) {
                struct ipv6_mreq mreq;
                (void)memset(&mreq, 0, sizeof(mreq));
                // flawfinder: ignore
                (void)memcpy(&mreq.ipv6mr_multiaddr, &((SOCKADDR_IN6 *)rep._ep._iptcp->ai_addr)->sin6_addr,
                             sizeof(IN6_ADDR));
                mreq.ipv6mr_interface = if_nametoindex(iface);
                if ((ret == _Z_RES_OK) && (setsockopt(sock->_sock._fd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
                                                      (const char *)&mreq, sizeof(mreq)) < 0)) {
                    _Z_ERROR_LOG(_Z_ERR_GENERIC);
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if (ret != _Z_RES_OK) {
                closesocket(sock->_sock._fd);
                sock->_sock._fd = INVALID_SOCKET;
                WSACleanup();
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

void _z_close_udp_multicast(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend,
                            const _z_sys_net_endpoint_t rep, const _z_sys_net_endpoint_t lep) {
    _ZP_UNUSED(lep);
    if (sockrecv->_sock._fd != INVALID_SOCKET) {
        if (rep._ep._iptcp->ai_family == AF_INET) {
            struct ip_mreq mreq;
            (void)memset(&mreq, 0, sizeof(mreq));
            mreq.imr_multiaddr.s_addr = ((SOCKADDR_IN *)rep._ep._iptcp->ai_addr)->sin_addr.s_addr;
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            setsockopt(sockrecv->_sock._fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const char *)&mreq, sizeof(mreq));
        } else if (rep._ep._iptcp->ai_family == AF_INET6) {
            struct ipv6_mreq mreq;
            (void)memset(&mreq, 0, sizeof(mreq));
            // flawfinder: ignore
            (void)memcpy(&mreq.ipv6mr_multiaddr, &((SOCKADDR_IN6 *)rep._ep._iptcp->ai_addr)->sin6_addr,
                         sizeof(struct in6_addr));
            setsockopt(sockrecv->_sock._fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP, (const char *)&mreq, sizeof(mreq));
        } else {
            /* Do nothing. */
        }
    }

    if (sockrecv->_sock._fd != INVALID_SOCKET) {
        closesocket(sockrecv->_sock._fd);
        sockrecv->_sock._fd = INVALID_SOCKET;
        WSACleanup();
    }
    if (socksend->_sock._fd != INVALID_SOCKET) {
        closesocket(socksend->_sock._fd);
        socksend->_sock._fd = INVALID_SOCKET;
        WSACleanup();
    }
}

size_t _z_read_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_slice_t *addr) {
    SOCKADDR_STORAGE raddr;
    unsigned int replen = sizeof(SOCKADDR_STORAGE);

    size_t rb = 0;
    do {
        rb = recvfrom(sock._sock._fd, (char *)ptr, (int)len, 0, (SOCKADDR *)&raddr, (int *)&replen);
        if (rb < (size_t)0) {
            rb = SIZE_MAX;
            break;
        }

        if (lep._ep._iptcp->ai_family == AF_INET) {
            SOCKADDR_IN *a = ((SOCKADDR_IN *)lep._ep._iptcp->ai_addr);
            SOCKADDR_IN *b = ((SOCKADDR_IN *)&raddr);
            if (!((a->sin_port == b->sin_port) && (a->sin_addr.s_addr == b->sin_addr.s_addr))) {
                if (addr != NULL) {
                    addr->len = sizeof(IN_ADDR) + sizeof(USHORT);
                    // flawfinder: ignore
                    (void)memcpy((uint8_t *)addr->start, &b->sin_addr.s_addr, sizeof(IN_ADDR));
                    // flawfinder: ignore
                    (void)memcpy((uint8_t *)(addr->start + sizeof(IN_ADDR)), &b->sin_port, sizeof(USHORT));
                }
                break;
            }
        } else if (lep._ep._iptcp->ai_family == AF_INET6) {
            SOCKADDR_IN6 *a = ((SOCKADDR_IN6 *)lep._ep._iptcp->ai_addr);
            SOCKADDR_IN6 *b = ((SOCKADDR_IN6 *)&raddr);
            if (!((a->sin6_port == b->sin6_port) &&
                  (memcmp(a->sin6_addr.s6_addr, b->sin6_addr.s6_addr, sizeof(struct in6_addr)) == 0))) {
                if (addr != NULL) {
                    addr->len = sizeof(struct in6_addr) + sizeof(USHORT);
                    // flawfinder: ignore
                    (void)memcpy((uint8_t *)addr->start, &b->sin6_addr.s6_addr, sizeof(struct in6_addr));
                    // flawfinder: ignore
                    (void)memcpy((uint8_t *)(addr->start + sizeof(struct in6_addr)), &b->sin6_port, sizeof(USHORT));
                }
                break;
            }
        } else {
            continue;
        }
    } while (1);

    return rb;
}

size_t _z_read_exact_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len,
                                   const _z_sys_net_endpoint_t lep, _z_slice_t *addr) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_udp_multicast(sock, pos, len - n, lep, addr);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

size_t _z_send_udp_multicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                             const _z_sys_net_endpoint_t rep) {
    return sendto(sock._sock._fd, (const char *)ptr, (int)len, 0, rep._ep._iptcp->ai_addr,
                  (int)rep._ep._iptcp->ai_addrlen);
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
    _z_close_udp_multicast(sockrecv, socksend, rep, lep);
}

size_t _z_udp_multicast_read_exact(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len,
                                   const _z_sys_net_endpoint_t lep, _z_slice_t *ep) {
    return _z_read_exact_udp_multicast(sock, ptr, len, lep, ep);
}

size_t _z_udp_multicast_read(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_slice_t *ep) {
    return _z_read_udp_multicast(sock, ptr, len, lep, ep);
}

size_t _z_udp_multicast_write(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                              const _z_sys_net_endpoint_t rep) {
    return _z_send_udp_multicast(sock, ptr, len, rep);
}

#endif
