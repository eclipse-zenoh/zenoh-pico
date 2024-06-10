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

#include <winsock2.h>
// The following includes must come after winsock2
#include <iphlpapi.h>
#include <ws2tcpip.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

WSADATA wsaData;

#if Z_FEATURE_LINK_TCP == 1

/*------------------ TCP sockets ------------------*/
int8_t _z_create_endpoint_tcp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    int8_t ret = _Z_RES_OK;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
        ADDRINFOA hints;
        (void)memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;  // Allow IPv4 or IPv6
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = 0;
        hints.ai_protocol = IPPROTO_TCP;

        if (getaddrinfo(s_address, s_port, &hints, &ep->_ep._iptcp) != 0) {
            ret = _Z_ERR_GENERIC;
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_free_endpoint_tcp(_z_sys_net_endpoint_t *ep) {
    freeaddrinfo(ep->_ep._iptcp);
    WSACleanup();
}

/*------------------ TCP sockets ------------------*/
int8_t _z_open_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    int8_t ret = _Z_RES_OK;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ret = _Z_ERR_GENERIC;
    }

    sock->_sock._fd = socket(rep._ep._iptcp->ai_family, rep._ep._iptcp->ai_socktype, rep._ep._iptcp->ai_protocol);
    if (sock->_sock._fd != INVALID_SOCKET) {
        z_time_t tv;
        tv.time = tout / (uint32_t)1000;
        tv.millitm = tout % (uint32_t)1000;
        if ((ret == _Z_RES_OK) && (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }

        int flags = 1;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }

        struct linger ling;
        ling.l_onoff = 1;
        ling.l_linger = Z_TRANSPORT_LEASE / 1000;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }

#if defined(ZENOH_MACOS) || defined(ZENOH_BSD)
        setsockopt(sock->_sock._fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)0, sizeof(int));
#endif

        ADDRINFOA *it = NULL;
        for (it = rep._ep._iptcp; it != NULL; it = it->ai_next) {
            if ((ret == _Z_RES_OK) && (connect(sock->_sock._fd, it->ai_addr, (int)it->ai_addrlen) < 0)) {
                if (it->ai_next == NULL) {
                    ret = _Z_ERR_GENERIC;
                    break;
                }
            } else {
                break;
            }
        }

        if (ret != _Z_RES_OK) {
            closesocket(sock->_sock._fd);
            WSACleanup();
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

int8_t _z_listen_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep) {
    int8_t ret = _Z_RES_OK;
    (void)sock;
    (void)lep;

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

void _z_close_tcp(_z_sys_net_socket_t *sock) {
    shutdown(sock->_sock._fd, SD_BOTH);
    closesocket(sock->_sock._fd);
    WSACleanup();
}

size_t _z_read_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t rb = recv(sock._sock._fd, (char *)ptr, (int)len, 0);
    if (rb < (size_t)0) {
        rb = SIZE_MAX;
    }

    return rb;
}

size_t _z_read_exact_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_tcp(sock, pos, len - n);
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_tcp(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    return send(sock._sock._fd, (const char *)ptr, (int)len, 0);
}
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
int8_t _z_create_endpoint_udp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    int8_t ret = _Z_RES_OK;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
        ADDRINFOA hints;
        (void)memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;  // Allow IPv4 or IPv6
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = 0;
        hints.ai_protocol = IPPROTO_UDP;

        if (getaddrinfo(s_address, s_port, &hints, &ep->_ep._iptcp) != 0) {
            ret = _Z_ERR_GENERIC;
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_free_endpoint_udp(_z_sys_net_endpoint_t *ep) {
    freeaddrinfo(ep->_ep._iptcp);
    WSACleanup();
}
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1
int8_t _z_open_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    int8_t ret = _Z_RES_OK;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ret = _Z_ERR_GENERIC;
    }

    sock->_sock._fd = socket(rep._ep._iptcp->ai_family, rep._ep._iptcp->ai_socktype, rep._ep._iptcp->ai_protocol);
    if (sock->_sock._fd != INVALID_SOCKET) {
        z_time_t tv;
        tv.time = tout / (uint32_t)1000;
        tv.millitm = tout % (uint32_t)1000;
        if ((ret == _Z_RES_OK) && (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }

        if (ret != _Z_RES_OK) {
            closesocket(sock->_sock._fd);
            WSACleanup();
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

int8_t _z_listen_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep, uint32_t tout) {
    (void)sock;
    (void)lep;
    (void)tout;
    int8_t ret = _Z_RES_OK;

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

void _z_close_udp_unicast(_z_sys_net_socket_t *sock) {
    closesocket(sock->_sock._fd);
    WSACleanup();
}

size_t _z_read_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    SOCKADDR_STORAGE raddr;
    unsigned int addrlen = sizeof(SOCKADDR_STORAGE);

    size_t rb = recvfrom(sock._sock._fd, (char *)ptr, (int)len, 0, (SOCKADDR *)&raddr, (int *)&addrlen);
    if (rb < (size_t)0) {
        rb = SIZE_MAX;
    }

    return rb;
}

size_t _z_read_exact_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_udp_unicast(sock, pos, len - n);
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_udp_unicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                           const _z_sys_net_endpoint_t rep) {
    return sendto(sock._sock._fd, (const char *)ptr, (int)len, 0, rep._ep._iptcp->ai_addr,
                  (int)rep._ep._iptcp->ai_addrlen);
}
#endif

#if Z_FEATURE_LINK_UDP_MULTICAST == 1
unsigned int __get_ip_from_iface(const char *iface, int sa_family, SOCKADDR **lsockaddr) {
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
                            (void)memcpy(*lsockaddr, tmp->FirstUnicastAddress->Address.lpSockaddr, sizeof(SOCKADDR_IN));
                            addrlen = sizeof(SOCKADDR_IN);
                        }
                    } else if (sa_family == AF_INET6) {
                        *lsockaddr = (SOCKADDR *)z_malloc(sizeof(SOCKADDR_IN6));
                        if (lsockaddr != NULL) {
                            (void)memset(*lsockaddr, 0, sizeof(SOCKADDR_IN6));
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

int8_t _z_open_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                             uint32_t tout, const char *iface) {
    int8_t ret = _Z_RES_OK;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ret = _Z_ERR_GENERIC;
    }

    SOCKADDR *lsockaddr = NULL;
    unsigned int addrlen = __get_ip_from_iface(iface, rep._ep._iptcp->ai_family, &lsockaddr);
    if (addrlen != 0U) {
        sock->_sock._fd = socket(rep._ep._iptcp->ai_family, rep._ep._iptcp->ai_socktype, rep._ep._iptcp->ai_protocol);
        if (sock->_sock._fd != INVALID_SOCKET) {
            z_time_t tv;
            tv.time = tout / (uint32_t)1000;
            tv.millitm = tout % (uint32_t)1000;
            if ((ret == _Z_RES_OK) &&
                (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

            if (rep._ep._iptcp->ai_family == AF_INET) {
                SOCKADDR_IN address = {
                    .sin_family = AF_INET, .sin_port = htons(0), .sin_addr.s_addr = htonl(INADDR_ANY), .sin_zero = {0}};
                if ((ret == _Z_RES_OK) &&
                    (bind(sock->_sock._fd, (SOCKADDR *)&address, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else if (rep._ep._iptcp->ai_family == AF_INET6) {
                SOCKADDR_IN6 address = {.sin6_family = AF_INET6, .sin6_port = htons(0), 0, .sin6_addr = {{{0}}}, 0};
                if ((ret == _Z_RES_OK) &&
                    (bind(sock->_sock._fd, (SOCKADDR *)&address, sizeof(SOCKADDR_IN6)) == SOCKET_ERROR)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                ret = _Z_ERR_GENERIC;
            }

            // Get the randomly assigned port used to discard loopback messages
            if ((ret == _Z_RES_OK) && (getsockname(sock->_sock._fd, lsockaddr, (int *)&addrlen) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

            if (rep._ep._iptcp->ai_family == AF_INET) {
                if ((ret == _Z_RES_OK) &&
                    (setsockopt(sock->_sock._fd, IPPROTO_IP, IP_MULTICAST_IF,
                                (const char *)&((SOCKADDR_IN *)lsockaddr)->sin_addr.s_addr, addrlen) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else if (rep._ep._iptcp->ai_family == AF_INET6) {
                if ((ret == _Z_RES_OK) &&
                    (setsockopt(sock->_sock._fd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                                (const char *)&((SOCKADDR_IN6 *)lsockaddr)->sin6_scope_id, sizeof(ULONG)) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                ret = _Z_ERR_GENERIC;
            }

            // Create lep endpoint
            if (ret == _Z_RES_OK) {
                ADDRINFOA *laddr = (ADDRINFOA *)z_malloc(sizeof(ADDRINFOA));
                if (laddr != NULL) {
                    laddr->ai_flags = 0;
                    laddr->ai_family = rep._ep._iptcp->ai_family;
                    laddr->ai_socktype = rep._ep._iptcp->ai_socktype;
                    laddr->ai_protocol = rep._ep._iptcp->ai_family;
                    laddr->ai_addrlen = addrlen;
                    laddr->ai_addr = lsockaddr;
                    laddr->ai_canonname = NULL;
                    laddr->ai_next = NULL;
                    lep->_ep._iptcp = laddr;

                    // This is leaking 16 bytes according to valgrind, but it is a know problem on some libc6
                    // implementations:
                    //    https://lists.debian.org/debian-glibc/2016/03/msg00241.html
                    // To avoid a fix to break zenoh-pico, we are let it leak for the moment.
                    // #if defined(ZENOH_LINUX)
                    //    z_free(lsockaddr);
                    // #endif
                } else {
                    ret = _Z_ERR_GENERIC;
                }
            }

            if (ret != _Z_RES_OK) {
                closesocket(sock->_sock._fd);
                WSACleanup();
            }
        } else {
            ret = _Z_ERR_GENERIC;
        }

        if (ret != _Z_RES_OK) {
            z_free(lsockaddr);
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

int8_t _z_listen_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                               const char *iface, const char *join) {
    (void)join;
    int8_t ret = _Z_RES_OK;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ret = _Z_ERR_GENERIC;
    }

    SOCKADDR *lsockaddr = NULL;
    unsigned int addrlen = __get_ip_from_iface(iface, rep._ep._iptcp->ai_family, &lsockaddr);
    if (addrlen != 0U) {
        sock->_sock._fd = socket(rep._ep._iptcp->ai_family, rep._ep._iptcp->ai_socktype, rep._ep._iptcp->ai_protocol);
        if (sock->_sock._fd != INVALID_SOCKET) {
            z_time_t tv;
            tv.time = tout / (uint32_t)1000;
            tv.millitm = tout % (uint32_t)1000;
            if ((ret == _Z_RES_OK) &&
                (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

            int optflag = 1;
            if ((ret == _Z_RES_OK) &&
                (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optflag, sizeof(optflag)) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

            if (rep._ep._iptcp->ai_family == AF_INET) {
                SOCKADDR_IN address = {.sin_family = AF_INET,
                                       .sin_port = ((SOCKADDR_IN *)rep._ep._iptcp->ai_addr)->sin_port,
                                       .sin_addr = {0},
                                       .sin_zero = {0}};
                if ((ret == _Z_RES_OK) && (bind(sock->_sock._fd, (SOCKADDR *)&address, sizeof address) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else if (rep._ep._iptcp->ai_family == AF_INET6) {
                SOCKADDR_IN6 address = {.sin6_family = AF_INET6,
                                        .sin6_port = ((SOCKADDR_IN6 *)rep._ep._iptcp->ai_addr)->sin6_port,
                                        0,
                                        .sin6_addr = {{{0}}},
                                        0};
                if ((ret == _Z_RES_OK) && (bind(sock->_sock._fd, (SOCKADDR *)&address, sizeof address) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                ret = _Z_ERR_GENERIC;
            }

            // Join the multicast group
            if (rep._ep._iptcp->ai_family == AF_INET) {
                struct ip_mreq mreq;
                (void)memset(&mreq, 0, sizeof(mreq));
                mreq.imr_multiaddr.s_addr = ((SOCKADDR_IN *)rep._ep._iptcp->ai_addr)->sin_addr.s_addr;
                mreq.imr_interface.s_addr = ((SOCKADDR_IN *)lsockaddr)->sin_addr.s_addr;
                if ((ret == _Z_RES_OK) && (setsockopt(sock->_sock._fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                                                      (const char *)&mreq, sizeof(mreq)) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else if (rep._ep._iptcp->ai_family == AF_INET6) {
                struct ipv6_mreq mreq;
                (void)memset(&mreq, 0, sizeof(mreq));
                (void)memcpy(&mreq.ipv6mr_multiaddr, &((SOCKADDR_IN6 *)rep._ep._iptcp->ai_addr)->sin6_addr,
                             sizeof(IN6_ADDR));
                mreq.ipv6mr_interface = if_nametoindex(iface);
                if ((ret == _Z_RES_OK) && (setsockopt(sock->_sock._fd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
                                                      (const char *)&mreq, sizeof(mreq)) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                ret = _Z_ERR_GENERIC;
            }

            if (ret != _Z_RES_OK) {
                closesocket(sock->_sock._fd);
                WSACleanup();
            }
        } else {
            ret = _Z_ERR_GENERIC;
        }

        z_free(lsockaddr);
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_close_udp_multicast(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend,
                            const _z_sys_net_endpoint_t rep) {
    if (rep._ep._iptcp->ai_family == AF_INET) {
        struct ip_mreq mreq;
        (void)memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = ((SOCKADDR_IN *)rep._ep._iptcp->ai_addr)->sin_addr.s_addr;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        setsockopt(sockrecv->_sock._fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const char *)&mreq, sizeof(mreq));
    } else if (rep._ep._iptcp->ai_family == AF_INET6) {
        struct ipv6_mreq mreq;
        (void)memset(&mreq, 0, sizeof(mreq));
        (void)memcpy(&mreq.ipv6mr_multiaddr, &((SOCKADDR_IN6 *)rep._ep._iptcp->ai_addr)->sin6_addr,
                     sizeof(struct in6_addr));
        // mreq.ipv6mr_interface = ifindex;
        setsockopt(sockrecv->_sock._fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP, (const char *)&mreq, sizeof(mreq));
    } else {
        // Do nothing. It must never not enter here.
        // Required to be compliant with MISRA 15.7 rule
    }

    closesocket(sockrecv->_sock._fd);
    WSACleanup();
    closesocket(socksend->_sock._fd);
    WSACleanup();
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
                // If addr is not NULL, it means that the rep was requested by the upper-layers
                if (addr != NULL) {
                    *addr = _z_slice_make(sizeof(IN_ADDR) + sizeof(USHORT));
                    (void)memcpy((uint8_t *)addr->start, &b->sin_addr.s_addr, sizeof(IN_ADDR));
                    (void)memcpy((uint8_t *)(addr->start + sizeof(IN_ADDR)), &b->sin_port, sizeof(USHORT));
                }
                break;
            }
        } else if (lep._ep._iptcp->ai_family == AF_INET6) {
            SOCKADDR_IN6 *a = ((SOCKADDR_IN6 *)lep._ep._iptcp->ai_addr);
            SOCKADDR_IN6 *b = ((SOCKADDR_IN6 *)&raddr);
            if (!((a->sin6_port == b->sin6_port) &&
                  (memcmp(a->sin6_addr.s6_addr, b->sin6_addr.s6_addr, sizeof(struct in6_addr)) == 0))) {
                // If addr is not NULL, it means that the rep was requested by the upper-layers
                if (addr != NULL) {
                    *addr = _z_slice_make(sizeof(struct in6_addr) + sizeof(USHORT));
                    (void)memcpy((uint8_t *)addr->start, &b->sin6_addr.s6_addr, sizeof(struct in6_addr));
                    (void)memcpy((uint8_t *)(addr->start + sizeof(struct in6_addr)), &b->sin6_port, sizeof(USHORT));
                }
                break;
            }
        } else {
            continue;  // FIXME: support error report on invalid packet to the upper layer
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
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_udp_multicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                             const _z_sys_net_endpoint_t rep) {
    return sendto(sock._sock._fd, (const char *)ptr, (int)len, 0, rep._ep._iptcp->ai_addr,
                  (int)rep._ep._iptcp->ai_addrlen);
}
#endif

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on Windows port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_SERIAL == 1
#error "Serial not supported yet on Windows port of Zenoh-Pico"
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported yet on Windows port of Zenoh-Pico"
#endif
