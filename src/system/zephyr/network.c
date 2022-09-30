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
#include <stddef.h>
#include <sys/socket.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"

typedef struct {
    int _fd;
} __z_net_socket;

#if Z_LINK_TCP == 1
/*------------------ TCP sockets ------------------*/
void *_z_create_endpoint_tcp(const char *s_addr, const char *s_port) {
    struct addrinfo hints;
    struct addrinfo *addr = NULL;

    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(s_addr, s_port, &hints, &addr) < 0) {
        return NULL;
    }

    return addr;
}

void _z_free_endpoint_tcp(void *arg) {
    struct addrinfo *self = (struct addrinfo *)arg;

    freeaddrinfo(self);
}

void *_z_open_tcp(void *arg, uint32_t tout) {
    __z_net_socket *ret = (__z_net_socket *)z_malloc(sizeof(__z_net_socket));
    struct addrinfo *raddr = (struct addrinfo *)arg;

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0) {
        goto _Z_OPEN_TCP_ERROR_1;
    }

#if LWIP_SO_LINGER == 1
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0) {
        goto _Z_OPEN_TCP_ERROR_2;
    }
#endif

    struct addrinfo *it = NULL;
    for (it = raddr; it != NULL; it = it->ai_next) {
        if (connect(sock, it->ai_addr, it->ai_addrlen) < 0) {
            if (it->ai_next == NULL) {
                goto _Z_OPEN_TCP_ERROR_2;
            }
        } else {
            break;
        }
    }

    ret->_fd = sock;
    return ret;

_Z_OPEN_TCP_ERROR_2:
    close(sock);

_Z_OPEN_TCP_ERROR_1:
    z_free(ret);
    return NULL;
}

void *_z_listen_tcp(void *arg) {
    struct addrinfo *laddr = (struct addrinfo *)arg;
    (void)laddr;

    // @TODO: To be implemented

    return NULL;
}

void _z_close_tcp(void *sock_arg) {
    __z_net_socket *sock = (__z_net_socket *)sock_arg;
    if (sock == NULL) {
        return;
    }

    // shutdown(sock->_fd, SHUT_RDWR); // Not implemented in Zephyr
    close(sock->_fd);
    z_free(sock);
}

size_t _z_read_tcp(void *sock_arg, uint8_t *ptr, size_t len) {
    __z_net_socket *sock = (__z_net_socket *)sock_arg;
    ssize_t rb = recv(sock->_fd, ptr, len, 0);
    if (rb < 0) {
        return SIZE_MAX;
    }

    return rb;
}

size_t _z_read_exact_tcp(void *sock_arg, uint8_t *ptr, size_t len) {
    size_t n = len;

    do {
        size_t rb = _z_read_tcp(sock_arg, ptr, n);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_tcp(void *sock_arg, const uint8_t *ptr, size_t len) {
    __z_net_socket *sock = (__z_net_socket *)sock_arg;
    return send(sock->_fd, ptr, len, 0);
}
#endif

#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
void *_z_create_endpoint_udp(const char *s_addr, const char *s_port) {
    struct addrinfo hints;
    struct addrinfo *addr = NULL;

    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(s_addr, s_port, &hints, &addr) < 0) {
        return NULL;
    }

    return addr;
}

void _z_free_endpoint_udp(void *arg) {
    struct addrinfo *self = (struct addrinfo *)arg;

    freeaddrinfo(self);
}
#endif

#if Z_LINK_UDP_UNICAST == 1
void *_z_open_udp_unicast(void *arg, uint32_t tout) {
    __z_net_socket *ret = (__z_net_socket *)z_malloc(sizeof(__z_net_socket));
    struct addrinfo *raddr = (struct addrinfo *)arg;

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0) {
        goto _Z_OPEN_UDP_UNICAST_ERROR_1;
    }

    z_time_t tv;
    tv.tv_sec = tout / (uint32_t)1000;
    tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        goto _Z_OPEN_UDP_UNICAST_ERROR_2;
    }

    ret->_fd = sock;
    return ret;

_Z_OPEN_UDP_UNICAST_ERROR_2:
    close(sock);

_Z_OPEN_UDP_UNICAST_ERROR_1:
    z_free(ret);
    return NULL;
}

void *_z_listen_udp_unicast(void *arg, uint32_t tout) {
    struct addrinfo *laddr = (struct addrinfo *)arg;
    (void)laddr;

    // @TODO: To be implemented

    return NULL;
}

void _z_close_udp_unicast(void *sock_arg) {
    __z_net_socket *sock = (__z_net_socket *)sock_arg;
    if (sock == NULL) {
        return;
    }

    close(sock->_fd);
    z_free(sock);
}

size_t _z_read_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len) {
    __z_net_socket *sock = (__z_net_socket *)sock_arg;

    struct sockaddr_storage raddr;
    unsigned int addrlen = sizeof(struct sockaddr_storage);

    size_t rb = recvfrom(sock->_fd, ptr, len, 0, (struct sockaddr *)&raddr, &addrlen);

    return rb;
}

size_t _z_read_exact_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len) {
    size_t n = len;

    do {
        size_t rb = _z_read_udp_unicast(sock_arg, ptr, n);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_udp_unicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg) {
    __z_net_socket *sock = (__z_net_socket *)sock_arg;
    struct addrinfo *raddr = (struct addrinfo *)raddr_arg;

    return sendto(sock->_fd, ptr, len, 0, raddr->ai_addr, raddr->ai_addrlen);
}
#endif

#if Z_LINK_UDP_MULTICAST == 1
void *_z_open_udp_multicast(void *arg_1, void **arg_2, uint32_t tout, const char *iface) {
    __z_net_socket *ret = (__z_net_socket *)z_malloc(sizeof(__z_net_socket));
    struct addrinfo *raddr = (struct addrinfo *)arg_1;
    struct addrinfo *laddr = NULL;
    unsigned int addrlen = 0;

    struct sockaddr *lsockaddr = NULL;
    if (raddr->ai_family == AF_INET) {
        lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in));
        (void)memset(lsockaddr, 0, sizeof(struct sockaddr_in));
        addrlen = sizeof(struct sockaddr_in);

        struct sockaddr_in *c_laddr = (struct sockaddr_in *)lsockaddr;
        c_laddr->sin_family = AF_INET;
        c_laddr->sin_addr.s_addr = INADDR_ANY;
        c_laddr->sin_port = htons(INADDR_ANY);
    } else if (raddr->ai_family == AF_INET6) {
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

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_2;
    }

    z_time_t tv;
    tv.tv_sec = tout / (uint32_t)1000;
    tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_3;
    }

    if (bind(sock, lsockaddr, addrlen) < 0) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_3;
    }

    // Get the randomly assigned port used to discard loopback messages
    if (getsockname(sock, lsockaddr, &addrlen) < 0) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_3;
    }

    // Create laddr endpoint
    laddr = (struct addrinfo *)z_malloc(sizeof(struct addrinfo));
    laddr->ai_flags = 0;
    laddr->ai_family = raddr->ai_family;
    laddr->ai_socktype = raddr->ai_socktype;
    laddr->ai_protocol = raddr->ai_protocol;
    laddr->ai_addrlen = addrlen;
    laddr->ai_addr = lsockaddr;
    laddr->ai_canonname = NULL;
    laddr->ai_next = NULL;
    *arg_2 = laddr;

    ret->_fd = sock;
    return ret;

_Z_OPEN_UDP_MULTICAST_ERROR_3:
    close(sock);

_Z_OPEN_UDP_MULTICAST_ERROR_2:
    z_free(lsockaddr);

_Z_OPEN_UDP_MULTICAST_ERROR_1:
    z_free(ret);
    return NULL;
}

void *_z_listen_udp_multicast(void *arg, uint32_t tout, const char *iface) {
    __z_net_socket *ret = (__z_net_socket *)z_malloc(sizeof(__z_net_socket));
    struct addrinfo *raddr = (struct addrinfo *)arg;
    struct sockaddr *laddr = NULL;
    unsigned int addrlen = 0;

    if (raddr->ai_family == AF_INET) {
        laddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in));
        (void)memset(laddr, 0, sizeof(struct sockaddr_in));
        addrlen = sizeof(struct sockaddr_in);

        struct sockaddr_in *c_laddr = (struct sockaddr_in *)laddr;
        c_laddr->sin_family = AF_INET;
        c_laddr->sin_addr.s_addr = INADDR_ANY;
        c_laddr->sin_port = ((struct sockaddr_in *)raddr->ai_addr)->sin_port;
    } else if (raddr->ai_family == AF_INET6) {
        laddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in6));
        (void)memset(laddr, 0, sizeof(struct sockaddr_in6));
        addrlen = sizeof(struct sockaddr_in6);

        struct sockaddr_in6 *c_laddr = (struct sockaddr_in6 *)laddr;
        c_laddr->sin6_family = AF_INET6;
        c_laddr->sin6_addr = in6addr_any;
        c_laddr->sin6_port = htons(INADDR_ANY);
        c_laddr->sin6_port = ((struct sockaddr_in6 *)raddr->ai_addr)->sin6_port;
        //        c_laddr->sin6_scope_id; // Not needed to be defined
    } else {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_1;
    }

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_1;
    }

    int optflag = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&optflag, sizeof(optflag)) < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
    }

    z_time_t tv;
    tv.tv_sec = tout / (uint32_t)1000;
    tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
    }

    if (bind(sock, laddr, addrlen) < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
    }

    // FIXME: iface passed into the locator is being ignored
    //        default if used instead
    struct net_if *ifa = NULL;
    ifa = net_if_get_default();
    if (!ifa) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
    }

    // Join the multicast group
    if (raddr->ai_family == AF_INET) {
        struct net_if_mcast_addr *mcast = NULL;
        mcast = net_if_ipv4_maddr_add(ifa, &((struct sockaddr_in *)raddr->ai_addr)->sin_addr);
        if (!mcast) {
            goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
        }
        net_if_ipv4_maddr_join(mcast);
    } else if (raddr->ai_family == AF_INET6) {
        struct net_if_mcast_addr *mcast = NULL;
        mcast = net_if_ipv6_maddr_add(ifa, &((struct sockaddr_in6 *)raddr->ai_addr)->sin6_addr);
        if (!mcast) {
            goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
        }
        net_if_ipv6_maddr_join(mcast);
    } else {
        // Do nothing. It must never not enter here, as it was already checked above.
        // Required to be compliant with MISRA 15.7 rule
    }

    ret->_fd = sock;
    return ret;

_Z_LISTEN_UDP_MULTICAST_ERROR_2:
    close(sock);

_Z_LISTEN_UDP_MULTICAST_ERROR_1:
    z_free(ret);
    return ret;
}

void _z_close_udp_multicast(void *sockrecv_arg, void *socksend_arg, void *raddr_arg) {
    __z_net_socket *sockrecv = (__z_net_socket *)sockrecv_arg;
    __z_net_socket *socksend = (__z_net_socket *)socksend_arg;
    struct addrinfo *raddr = (struct addrinfo *)raddr_arg;

    // FIXME: iface passed into the locator is being ignored
    //        default if used instead
    struct net_if *ifa = NULL;
    ifa = net_if_get_default();
    if (!ifa) {
        goto _Z_CLOSE_UDP_MULTICAST_ERROR_1;
    }

    // Both sockrecv and socksend must be compared to NULL,
    //  because we dont know if the close is trigger by a normal close
    //  or some of the sockets failed during the opening/listening procedure.
    if (sockrecv != NULL) {
        if (raddr->ai_family == AF_INET) {
            struct net_if_mcast_addr *mcast = NULL;
            mcast = net_if_ipv4_maddr_add(ifa, &((struct sockaddr_in *)raddr->ai_addr)->sin_addr);
            if (!mcast) {
                goto _Z_CLOSE_UDP_MULTICAST_ERROR_1;
            }

            net_if_ipv4_maddr_leave(mcast);
            net_if_ipv4_maddr_rm(ifa, &((struct sockaddr_in *)raddr->ai_addr)->sin_addr);
        } else if (raddr->ai_family == AF_INET6) {
            struct net_if_mcast_addr *mcast = NULL;
            mcast = net_if_ipv6_maddr_add(ifa, &((struct sockaddr_in6 *)raddr->ai_addr)->sin6_addr);
            if (!mcast) {
                goto _Z_CLOSE_UDP_MULTICAST_ERROR_1;
            }

            net_if_ipv6_maddr_leave(mcast);
            net_if_ipv6_maddr_rm(ifa, &((struct sockaddr_in6 *)raddr->ai_addr)->sin6_addr);
        } else {
            // Do nothing. It must never not enter here.
            // Required to be compliant with MISRA 15.7 rule
        }

        close(sockrecv->_fd);
        z_free(sockrecv);
    }

    if (socksend != NULL) {
        close(socksend->_fd);
        z_free(socksend);
    }

_Z_CLOSE_UDP_MULTICAST_ERROR_1:
    if (sockrecv != NULL) {
        close(sockrecv->_fd);
        z_free(sockrecv);
    }

    if (socksend != NULL) {
        close(socksend->_fd);
        z_free(socksend);
    }
}

size_t _z_read_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *arg, _z_bytes_t *addr) {
    __z_net_socket *sock = (__z_net_socket *)sock_arg;
    struct addrinfo *laddr = (struct addrinfo *)arg;
    struct sockaddr_storage raddr;
    unsigned int raddrlen = sizeof(struct sockaddr_storage);

    ssize_t rb = 0;
    do {
        rb = recvfrom(sock->_fd, ptr, len, 0, (struct sockaddr *)&raddr, &raddrlen);

        if (rb < 0) {
            return rb;
        }

        if (laddr->ai_family == AF_INET) {
            struct sockaddr_in *a = ((struct sockaddr_in *)laddr->ai_addr);
            struct sockaddr_in *b = ((struct sockaddr_in *)&raddr);
            if (!((a->sin_port == b->sin_port) && (a->sin_addr.s_addr == b->sin_addr.s_addr))) {
                // If addr is not NULL, it means that the raddr was requested by the upper-layers
                if (addr != NULL) {
                    *addr = _z_bytes_make(sizeof(uint32_t) + sizeof(uint16_t));
                    (void)memcpy((uint8_t *)addr->start, &b->sin_addr.s_addr, sizeof(uint32_t));
                    (void)memcpy((uint8_t *)(addr->start + sizeof(uint32_t)), &b->sin_port, sizeof(uint16_t));
                }
                break;
            }
        } else if (laddr->ai_family == AF_INET6) {
            struct sockaddr_in6 *a = ((struct sockaddr_in6 *)laddr->ai_addr);
            struct sockaddr_in6 *b = ((struct sockaddr_in6 *)&raddr);
            if (!((a->sin6_port == b->sin6_port) &&
                  (memcmp(a->sin6_addr.s6_addr, b->sin6_addr.s6_addr, sizeof(uint32_t) * 4UL) == 0))) {
                // If addr is not NULL, it means that the raddr was requested by the upper-layers
                if (addr != NULL) {
                    *addr = _z_bytes_make((sizeof(uint32_t) * 4UL) + sizeof(uint16_t));
                    (void)memcpy((uint8_t *)addr->start, &b->sin6_addr.s6_addr, sizeof(uint32_t) * 4UL);
                    (void)memcpy((uint8_t *)(addr->start + (sizeof(uint32_t) * 4UL)), &b->sin6_port, sizeof(uint16_t));
                }
                break;
            }
        } else {
            continue;  // FIXME: support error report on invalid packet to the upper layer
        }
    } while (1);

    return rb;
}

size_t _z_read_exact_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *arg, _z_bytes_t *addr) {
    size_t n = len;

    do {
        size_t rb = _z_read_udp_multicast(sock_arg, ptr, n, arg, addr);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_udp_multicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg) {
    __z_net_socket *sock = (__z_net_socket *)sock_arg;
    struct addrinfo *raddr = (struct addrinfo *)raddr_arg;

    return sendto(sock->_fd, ptr, len, 0, raddr->ai_addr, raddr->ai_addrlen);
}
#endif

#if Z_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on Zephyr port of Zenoh-Pico"
#endif

#if Z_LINK_SERIAL == 1
#error "Serial not supported yet on Zephyr port of Zenoh-Pico"
#endif