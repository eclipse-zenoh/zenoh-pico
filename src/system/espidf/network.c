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

#include <netdb.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/link/bt.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_LINK_TCP == 1
/*------------------ TCP sockets ------------------*/
_z_sys_net_endpoint_t _z_create_endpoint_tcp(const char *s_addr, const char *s_port) {
    _z_sys_net_endpoint_t ep;
    struct addrinfo hints;

    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(s_addr, s_port, &hints, &ep._addr) < 0) {
        goto ERR;
    }

    ep._err = true;
    return ep;

ERR:
    ep._err = false;
    return ep;
}

void _z_free_endpoint_tcp(_z_sys_net_endpoint_t ep) { freeaddrinfo(ep._addr); }

_z_sys_net_socket_t _z_open_tcp(_z_sys_net_endpoint_t rep, uint32_t tout) {
    _z_sys_net_socket_t sock;

    int flags = 1;

    int fd = socket(rep._addr->ai_family, rep._addr->ai_socktype, rep._addr->ai_protocol);
    if (fd < 0) {
        goto _Z_OPEN_TCP_ERROR_1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0) {
        goto _Z_OPEN_TCP_ERROR_2;
    }

#if LWIP_SO_LINGER == 1
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0) {
        goto _Z_OPEN_TCP_ERROR_2;
    }
#endif

    for (struct addrinfo *it = rep._addr; it != NULL; it = it->ai_next) {
        if (connect(fd, it->ai_addr, it->ai_addrlen) < 0) {
            if (it->ai_next == NULL) {
                goto _Z_OPEN_TCP_ERROR_2;
            }
        } else {
            break;
        }
    }

    sock._fd = fd;
    sock._err = false;
    return sock;

_Z_OPEN_TCP_ERROR_2:
    close(fd);

_Z_OPEN_TCP_ERROR_1:
    sock._err = true;
    return sock;
}

_z_sys_net_socket_t _z_listen_tcp(_z_sys_net_endpoint_t lep) {
    _z_sys_net_socket_t sock;
    (void)lep;

    // @TODO: To be implemented

    sock._err = true;
    return sock;
}

void _z_close_tcp(_z_sys_net_socket_t sock) {
    if (sock._err == true) {
        return;
    }

    shutdown(sock._fd, SHUT_RDWR);
    close(sock._fd);
}

size_t _z_read_tcp(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    ssize_t rb = recv(sock._fd, ptr, len, 0);
    if (rb < 0) {
        return SIZE_MAX;
    }

    return rb;
}

size_t _z_read_exact_tcp(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = len;

    do {
        size_t rb = _z_read_tcp(sock, ptr, n);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = _z_ptr_u8_offset(ptr, len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_tcp(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) { return send(sock._fd, ptr, len, 0); }
#endif

#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
_z_sys_net_endpoint_t _z_create_endpoint_udp(const char *s_addr, const char *s_port) {
    _z_sys_net_endpoint_t ep;
    struct addrinfo hints;

    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(s_addr, s_port, &hints, &ep._addr) < 0) {
        goto ERR;
    }

    ep._err = true;
    return ep;

ERR:
    ep._err = true;
    return ep;
}

void _z_free_endpoint_udp(_z_sys_net_endpoint_t ep) {
    if (ep._err == true) {
        return;
    }

    freeaddrinfo(ep._addr);
}
#endif

#if Z_LINK_UDP_UNICAST == 1
_z_sys_net_socket_t _z_open_udp_unicast(_z_sys_net_endpoint_t rep, uint32_t tout) {
    _z_sys_net_socket_t sock;

    int fd = socket(rep._addr->ai_family, rep._addr->ai_socktype, rep._addr->ai_protocol);
    if (fd < 0) {
        goto _Z_OPEN_UDP_UNICAST_ERROR_1;
    }

    z_time_t tv;
    tv.tv_sec = tout / (uint32_t)1000;
    tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        goto _Z_OPEN_UDP_UNICAST_ERROR_2;
    }

    sock._fd = fd;
    sock._err = false;
    return sock;

_Z_OPEN_UDP_UNICAST_ERROR_2:
    close(fd);

_Z_OPEN_UDP_UNICAST_ERROR_1:
    sock._err = true;
    return sock;
}

_z_sys_net_socket_t _z_listen_udp_unicast(_z_sys_net_endpoint_t lep, uint32_t tout) {
    _z_sys_net_socket_t sock;
    (void)lep;
    (void)tout;

    // @TODO: To be implemented

    sock._err = false;
    return sock;
}

void _z_close_udp_unicast(_z_sys_net_socket_t sock) {
    if (sock._err == true) {
        return;
    }

    close(sock._fd);
}

size_t _z_read_udp_unicast(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    struct sockaddr_storage raddr;
    unsigned int addrlen = sizeof(struct sockaddr_storage);

    ssize_t rb = recvfrom(sock._fd, ptr, len, 0, (struct sockaddr *)&raddr, &addrlen);

    if (rb < 0) {
        return SIZE_MAX;
    }

    return rb;
}

size_t _z_read_exact_udp_unicast(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = len;

    do {
        size_t rb = _z_read_udp_unicast(sock, ptr, n);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = _z_ptr_u8_offset(ptr, len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_udp_unicast(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len, _z_sys_net_endpoint_t rep) {
    return sendto(sock._fd, ptr, len, 0, rep._addr->ai_addr, rep._addr->ai_addrlen);
}
#endif

#if Z_LINK_UDP_MULTICAST == 1
_z_sys_net_socket_t _z_open_udp_multicast(_z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep, uint32_t tout,
                                          const char *iface) {
    _z_sys_net_socket_t sock;
    struct addrinfo *laddr = NULL;
    int fd = -1;

    unsigned int addrlen = 0;
    struct sockaddr *lsockaddr = NULL;
    if (rep._addr->ai_family == AF_INET) {
        lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in));
        (void)memset(lsockaddr, 0, sizeof(struct sockaddr_in));
        addrlen = sizeof(struct sockaddr_in);

        struct sockaddr_in *c_laddr = (struct sockaddr_in *)lsockaddr;
        c_laddr->sin_family = AF_INET;
        c_laddr->sin_addr.s_addr = INADDR_ANY;
        c_laddr->sin_port = htons(INADDR_ANY);
    } else if (rep._addr->ai_family == AF_INET6) {
        lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in6));
        (void)memset(lsockaddr, 0, sizeof(struct sockaddr_in6));
        addrlen = sizeof(struct sockaddr_in6);

        struct sockaddr_in6 *c_laddr = (struct sockaddr_in6 *)lsockaddr;
        c_laddr->sin6_family = AF_INET6;
        c_laddr->sin6_addr = in6addr_any;
        c_laddr->sin6_port = htons(INADDR_ANY);
        //        c_laddr->sin6_scope_id; // Not needed to be defined
    } else {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_1;
    }

    fd = socket(rep._addr->ai_family, rep._addr->ai_socktype, rep._addr->ai_protocol);
    if (fd < 0) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_2;
    }

    z_time_t tv;
    tv.tv_sec = tout / (uint32_t)1000;
    tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_3;
    }

    if (bind(fd, lsockaddr, addrlen) < 0) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_3;
    }

    if (getsockname(fd, lsockaddr, &addrlen) == -1) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_3;
    }

    if (lsockaddr->sa_family == AF_INET) {
        if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &((struct sockaddr_in *)lsockaddr)->sin_addr,
                       sizeof(struct in_addr)) < 0) {
            goto _Z_OPEN_UDP_MULTICAST_ERROR_3;
        }
    } else if (lsockaddr->sa_family == AF_INET6) {
        int ifindex = 0;
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0) {
            goto _Z_OPEN_UDP_MULTICAST_ERROR_3;
        }
    } else {
        // Do nothing. It must never not enter here, as it was already checked above.
        // Required to be compliant with MISRA 15.7 rule
    }

    // Create laddr endpoint
    laddr = (struct addrinfo *)z_malloc(sizeof(struct addrinfo));
    laddr->ai_flags = 0;
    laddr->ai_family = rep._addr->ai_family;
    laddr->ai_socktype = rep._addr->ai_socktype;
    laddr->ai_protocol = rep._addr->ai_protocol;
    laddr->ai_addrlen = addrlen;
    laddr->ai_addr = lsockaddr;
    laddr->ai_canonname = NULL;
    laddr->ai_next = NULL;
    lep->_addr = laddr;

    sock._fd = fd;
    sock._err = false;
    return sock;

_Z_OPEN_UDP_MULTICAST_ERROR_3:
    close(fd);

_Z_OPEN_UDP_MULTICAST_ERROR_2:
    z_free(lsockaddr);

_Z_OPEN_UDP_MULTICAST_ERROR_1:
    sock._err = true;
    return sock;
}

_z_sys_net_socket_t _z_listen_udp_multicast(_z_sys_net_endpoint_t rep, uint32_t tout, const char *iface) {
    _z_sys_net_socket_t sock;

    struct sockaddr *laddr = NULL;
    unsigned int addrlen = 0;
    int fd = -1;
    int optflag;

    if (rep._addr->ai_family == AF_INET) {
        laddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in));
        (void)memset(laddr, 0, sizeof(struct sockaddr_in));
        addrlen = sizeof(struct sockaddr_in);

        struct sockaddr_in *c_laddr = (struct sockaddr_in *)laddr;
        c_laddr->sin_family = AF_INET;
        c_laddr->sin_addr.s_addr = INADDR_ANY;
        c_laddr->sin_port = ((struct sockaddr_in *)rep._addr->ai_addr)->sin_port;
    } else if (rep._addr->ai_family == AF_INET6) {
        laddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in6));
        (void)memset(laddr, 0, sizeof(struct sockaddr_in6));
        addrlen = sizeof(struct sockaddr_in6);

        struct sockaddr_in6 *c_laddr = (struct sockaddr_in6 *)laddr;
        c_laddr->sin6_family = AF_INET6;
        c_laddr->sin6_addr = in6addr_any;
        c_laddr->sin6_port = htons(INADDR_ANY);
        c_laddr->sin6_port = ((struct sockaddr_in6 *)rep._addr->ai_addr)->sin6_port;
        //        c_laddr->sin6_scope_id; // Not needed to be defined
    } else {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_1;
    }

    fd = socket(rep._addr->ai_family, rep._addr->ai_socktype, rep._addr->ai_protocol);
    if (fd < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_1;
    }

    z_time_t tv;
    tv.tv_sec = tout / (uint32_t)1000;
    tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
    }

    optflag = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optflag, sizeof(optflag)) < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
    }

    if (bind(fd, laddr, addrlen) < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
    }

    // Join the multicast group
    if (rep._addr->ai_family == AF_INET) {
        struct ip_mreq mreq;
        (void)memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)rep._addr->ai_addr)->sin_addr.s_addr;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
        }
    } else if (rep._addr->ai_family == AF_INET6) {
        struct ipv6_mreq mreq;
        (void)memset(&mreq, 0, sizeof(mreq));
        (void)memcpy(&mreq.ipv6mr_multiaddr, &((struct sockaddr_in6 *)rep._addr->ai_addr)->sin6_addr,
                     sizeof(struct in6_addr));
        mreq.ipv6mr_interface = 1;  // FIXME: 0 is supposed to be the default interface,
                                    //        but it fails on the setsockopt.
                                    //        1 seems to be a working value on the WiFi interface
                                    //        which is the one available by default in ESP32
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0) {
            goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
        }
    } else {
        // Do nothing. It must never not enter here, as it was already checked above.
        // Required to be compliant with MISRA 15.7 rule
    }

    sock._fd = fd;
    sock._err = false;
    return sock;

_Z_LISTEN_UDP_MULTICAST_ERROR_2:
    close(fd);

_Z_LISTEN_UDP_MULTICAST_ERROR_1:
    sock._err = true;
    return sock;
}

void _z_close_udp_multicast(_z_sys_net_socket_t sockrecv, _z_sys_net_socket_t socksend, _z_sys_net_endpoint_t rep) {
    // Both sockrecv and socksend must be compared to err,
    //  because we dont know if the close is trigger by a normal close
    //  or some of the sockets failed during the opening/listening procedure.
    if (sockrecv._err != true) {
        if (rep._addr->ai_family == AF_INET) {
            struct ip_mreq mreq;
            (void)memset(&mreq, 0, sizeof(mreq));
            mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)rep._addr->ai_addr)->sin_addr.s_addr;
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            setsockopt(sockrecv._fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
        } else if (rep._addr->ai_family == AF_INET6) {
            struct ipv6_mreq mreq;
            (void)memset(&mreq, 0, sizeof(mreq));
            (void)memcpy(&mreq.ipv6mr_multiaddr, &((struct sockaddr_in6 *)rep._addr->ai_addr)->sin6_addr,
                         sizeof(struct in6_addr));
            mreq.ipv6mr_interface = 1;  // FIXME: 0 is supposed to be the default interface,
                                        //        but it fails on the setsockopt.
                                        //        1 seems to be a working value on the WiFi interface
                                        //        which is the one available by default in ESP32
            setsockopt(sockrecv._fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &mreq, sizeof(mreq));
        } else {
            // Do nothing. It must never not enter here.
            // Required to be compliant with MISRA 15.7 rule
        }

        close(sockrecv._fd);
    }

    if (socksend._err != true) {
        close(socksend._fd);
    }
}

size_t _z_read_udp_multicast(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len, _z_sys_net_endpoint_t lep,
                             _z_bytes_t *addr) {
    struct sockaddr_storage raddr;
    unsigned int raddrlen = sizeof(struct sockaddr_storage);

    size_t rb = 0;
    do {
        rb = recvfrom(sock._fd, ptr, len, 0, (struct sockaddr *)&raddr, &raddrlen);

        if (rb == SIZE_MAX) {
            return rb;
        }
        if (lep._addr->ai_family == AF_INET) {
            struct sockaddr_in *a = ((struct sockaddr_in *)lep._addr->ai_addr);
            struct sockaddr_in *b = ((struct sockaddr_in *)&raddr);
            if (!((a->sin_port == b->sin_port) && (a->sin_addr.s_addr == b->sin_addr.s_addr))) {
                // If addr is not NULL, it means that the raddr was requested by the upper-layers
                if (addr != NULL) {
                    *addr = _z_bytes_make(sizeof(in_addr_t) + sizeof(in_port_t));
                    (void)memcpy((uint8_t *)addr->start, &b->sin_addr.s_addr, sizeof(in_addr_t));
                    (void)memcpy((uint8_t *)(addr->start + sizeof(in_addr_t)), &b->sin_port, sizeof(in_port_t));
                }
                break;
            }
        } else if (lep._addr->ai_family == AF_INET6) {
            struct sockaddr_in6 *a = ((struct sockaddr_in6 *)lep._addr->ai_addr);
            struct sockaddr_in6 *b = ((struct sockaddr_in6 *)&raddr);
            if ((a->sin6_port != b->sin6_port) ||
                (memcmp(&a->sin6_addr, &b->sin6_addr, sizeof(struct in6_addr)) != 0)) {
                // If addr is not NULL, it means that the raddr was requested by the upper-layers
                if (addr != NULL) {
                    *addr = _z_bytes_make(sizeof(struct in6_addr) + sizeof(in_port_t));
                    (void)memcpy((uint8_t *)addr->start, &b->sin6_addr.s6_addr, sizeof(struct in6_addr));
                    (void)memcpy((uint8_t *)(addr->start + sizeof(struct in6_addr)), &b->sin6_port, sizeof(in_port_t));
                }
                break;
            }
        } else {
            continue;  // FIXME: support error report on invalid packet to the upper layer
        }
    } while (1);

    return rb;
}

size_t _z_read_exact_udp_multicast(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len, _z_sys_net_endpoint_t lep,
                                   _z_bytes_t *addr) {
    size_t n = len;

    do {
        size_t rb = _z_read_udp_multicast(sock, ptr, n, lep, addr);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = _z_ptr_u8_offset(ptr, len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_udp_multicast(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len, _z_sys_net_endpoint_t rep) {
    return sendto(sock._fd, ptr, len, 0, rep._addr->ai_addr, rep._addr->ai_addrlen);
}
#endif

#if Z_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on ESP-IDF port of Zenoh-Pico"
#endif

#if Z_LINK_SERIAL == 1
#error "Serial not supported yet on Arduino port of Zenoh-Pico"
#endif
