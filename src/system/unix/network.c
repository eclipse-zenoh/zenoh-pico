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

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"

typedef struct {
    int _fd;
} __z_net_socket;

#if Z_LINK_TCP == 1

/*------------------ TCP sockets ------------------*/
void *_z_create_endpoint_tcp(const char *s_addr, const char *port) {
    struct addrinfo hints;
    struct addrinfo *addr = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(s_addr, port, &hints, &addr) < 0) return NULL;

    return addr;
}

void _z_free_endpoint_tcp(void *arg) {
    struct addrinfo *self = (struct addrinfo *)arg;

    freeaddrinfo(self);
}

/*------------------ TCP sockets ------------------*/
void *_z_open_tcp(void *arg, uint32_t tout) {
    __z_net_socket *ret = (__z_net_socket *)z_malloc(sizeof(__z_net_socket));
    struct addrinfo *raddr = (struct addrinfo *)arg;

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0) goto _Z_OPEN_TCP_ERROR_1;

    z_time_t tv;
    tv.tv_sec = tout / 1000;
    tv.tv_usec = (tout % 1000) * 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) goto _Z_OPEN_TCP_ERROR_2;

    int flags = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0) goto _Z_OPEN_TCP_ERROR_2;

    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0) goto _Z_OPEN_TCP_ERROR_2;

#if defined(ZENOH_MACOS) || defined(ZENOH_BSD)
    setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void *)0, sizeof(int));
#endif

    struct addrinfo *it = NULL;
    for (it = raddr; it != NULL; it = it->ai_next) {
        if (connect(sock, it->ai_addr, it->ai_addrlen) < 0) {
            if (it->ai_next == NULL) goto _Z_OPEN_TCP_ERROR_2;
        } else
            break;
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
    if (sock == NULL) return;

    shutdown(sock->_fd, SHUT_RDWR);
    close(sock->_fd);
    z_free(sock);
}

size_t _z_read_tcp(void *sock_arg, uint8_t *ptr, size_t len) {
    __z_net_socket *sock = (__z_net_socket *)sock_arg;
    ssize_t rb = recv(sock->_fd, ptr, len, 0);
    if (rb < 0) return SIZE_MAX;

    return rb;
}

size_t _z_read_exact_tcp(void *sock_arg, uint8_t *ptr, size_t len) {
    size_t n = len;
    size_t rb = 0;

    do {
        rb = _z_read_tcp(sock_arg, ptr, n);
        if (rb == SIZE_MAX) return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _z_send_tcp(void *sock_arg, const uint8_t *ptr, size_t len) {
    __z_net_socket *sock = (__z_net_socket *)sock_arg;
#if defined(ZENOH_LINUX)
    return send(sock->_fd, ptr, len, MSG_NOSIGNAL);
#else
    return send(sock->_fd, ptr, len, 0);
#endif
}
#endif

#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
void *_z_create_endpoint_udp(const char *s_addr, const char *port) {
    struct addrinfo hints;
    struct addrinfo *addr = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(s_addr, port, &hints, &addr) < 0) return NULL;

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
    if (sock < 0) goto _Z_OPEN_UDP_UNICAST_ERROR_1;

    z_time_t tv;
    tv.tv_sec = tout / 1000;
    tv.tv_usec = (tout % 1000) * 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) goto _Z_OPEN_UDP_UNICAST_ERROR_2;

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
    (void)tout;

    // @TODO: To be implemented

    return NULL;
}

void _z_close_udp_unicast(void *sock_arg) {
    __z_net_socket *sock = (__z_net_socket *)sock_arg;
    if (sock == NULL) return;

    close(sock->_fd);
    z_free(sock);
}

size_t _z_read_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len) {
    __z_net_socket *sock = (__z_net_socket *)sock_arg;

    struct sockaddr_storage raddr;
    unsigned int addrlen = sizeof(struct sockaddr_storage);

    ssize_t rb = recvfrom(sock->_fd, ptr, len, 0, (struct sockaddr *)&raddr, &addrlen);

    if (rb < 0) return SIZE_MAX;

    return rb;
}

size_t _z_read_exact_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len) {
    size_t n = len;
    size_t rb = 0;

    do {
        rb = _z_read_udp_unicast(sock_arg, ptr, n);
        if (rb == SIZE_MAX) return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _z_send_udp_unicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg) {
    __z_net_socket *sock = (__z_net_socket *)sock_arg;
    struct addrinfo *raddr = (struct addrinfo *)raddr_arg;

    return sendto(sock->_fd, ptr, len, 0, raddr->ai_addr, raddr->ai_addrlen);
}
#endif

#if Z_LINK_UDP_MULTICAST == 1
unsigned int __get_ip_from_iface(const char *iface, int sa_family, struct sockaddr **lsockaddr) {
    unsigned int addrlen = 0;
    struct ifaddrs *l_ifaddr = NULL;
    if (getifaddrs(&l_ifaddr) < 0) {
        return 0;
    }

    struct ifaddrs *tmp = NULL;
    for (tmp = l_ifaddr; tmp != NULL; tmp = tmp->ifa_next) {
        if (_z_str_eq(tmp->ifa_name, iface)) {
            if (tmp->ifa_addr->sa_family == sa_family) {
                if (tmp->ifa_addr->sa_family == AF_INET) {
                    *lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in));
                    memset(*lsockaddr, 0, sizeof(struct sockaddr_in));
                    memcpy(*lsockaddr, tmp->ifa_addr, sizeof(struct sockaddr_in));
                    addrlen = sizeof(struct sockaddr_in);
                } else if (tmp->ifa_addr->sa_family == AF_INET6) {
                    *lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in6));
                    memset(*lsockaddr, 0, sizeof(struct sockaddr_in6));
                    memcpy(*lsockaddr, tmp->ifa_addr, sizeof(struct sockaddr_in6));
                    addrlen = sizeof(struct sockaddr_in6);
                } else {
                    continue;
                }

                break;
            }
        }
    }
    freeifaddrs(l_ifaddr);

    return addrlen;
}

void *_z_open_udp_multicast(void *arg_1, void **arg_2, uint32_t tout, const char *iface) {
    __z_net_socket *ret = (__z_net_socket *)z_malloc(sizeof(__z_net_socket));
    struct addrinfo *raddr = (struct addrinfo *)arg_1;
    struct addrinfo *laddr = NULL;

    struct sockaddr *lsockaddr = NULL;
    unsigned int addrlen = __get_ip_from_iface(iface, raddr->ai_family, &lsockaddr);
    if (lsockaddr == NULL) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_1;
    }

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0) goto _Z_OPEN_UDP_MULTICAST_ERROR_2;

    z_time_t tv;
    tv.tv_sec = tout / 1000;
    tv.tv_usec = (tout % 1000) * 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) goto _Z_OPEN_UDP_MULTICAST_ERROR_3;

    if (bind(sock, lsockaddr, addrlen) < 0) goto _Z_OPEN_UDP_MULTICAST_ERROR_3;

    // Get the randomly assigned port used to discard loopback messages
    if (getsockname(sock, lsockaddr, &addrlen) < 0) goto _Z_OPEN_UDP_MULTICAST_ERROR_3;

    if (lsockaddr->sa_family == AF_INET) {
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &((struct sockaddr_in *)lsockaddr)->sin_addr,
                       sizeof(struct in_addr)) < 0)
            goto _Z_OPEN_UDP_MULTICAST_ERROR_3;
    } else if (lsockaddr->sa_family == AF_INET6) {
        int ifindex = if_nametoindex(iface);
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0)
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

    // This is leaking 16 bytes according to valgrind, but it is a know problem on some libc6 implementations:
    //    https://lists.debian.org/debian-glibc/2016/03/msg00241.html
    // To avoid a fix to break zenoh-pico, we are let it leak for the moment.
    //#if defined(ZENOH_LINUX)
    //    z_free(lsockaddr);
    //#endif

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

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0) goto _Z_LISTEN_UDP_MULTICAST_ERROR_1;

    z_time_t tv;
    tv.tv_sec = tout / 1000;
    tv.tv_usec = (tout % 1000) * 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;

    int optflag = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&optflag, sizeof(optflag)) < 0)
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;

#if defined(ZENOH_MACOS) || defined(ZENOH_BSD)
    if (bind(sock, raddr->ai_addr, raddr->ai_addrlen) < 0) goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
#elif defined(ZENOH_LINUX)
    if (raddr->ai_family == AF_INET) {
        struct sockaddr_in address = {AF_INET, ((struct sockaddr_in *)raddr->ai_addr)->sin_port, {0}, {0}};
        if (bind(sock, (struct sockaddr *)&address, sizeof address) < 0) goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
    } else if (raddr->ai_family == AF_INET6) {
        struct sockaddr_in6 address = {AF_INET6, ((struct sockaddr_in6 *)raddr->ai_addr)->sin6_port, 0, {{{0}}}, 0};
        if (bind(sock, (struct sockaddr *)&address, sizeof address) < 0) goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
    } else
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
#else
    goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
#endif

    struct sockaddr *lsockaddr = NULL;
    __get_ip_from_iface(iface, raddr->ai_family, &lsockaddr);
    if (lsockaddr == NULL) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
    }

    // Join the multicast group
    if (raddr->ai_family == AF_INET) {
        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)raddr->ai_addr)->sin_addr.s_addr;
        mreq.imr_interface.s_addr = ((struct sockaddr_in *)lsockaddr)->sin_addr.s_addr;
        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            goto _Z_LISTEN_UDP_MULTICAST_ERROR_3;
        }
    } else if (raddr->ai_family == AF_INET6) {
        struct ipv6_mreq mreq;
        memset(&mreq, 0, sizeof(mreq));
        memcpy(&mreq.ipv6mr_multiaddr, &((struct sockaddr_in6 *)raddr->ai_addr)->sin6_addr, sizeof(struct in6_addr));
        mreq.ipv6mr_interface = if_nametoindex(iface);
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0) {
            goto _Z_LISTEN_UDP_MULTICAST_ERROR_3;
        }
    } else {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_3;
    }

    z_free(lsockaddr);

    ret->_fd = sock;
    return ret;

_Z_LISTEN_UDP_MULTICAST_ERROR_3:
    z_free(lsockaddr);

_Z_LISTEN_UDP_MULTICAST_ERROR_2:
    close(sock);

_Z_LISTEN_UDP_MULTICAST_ERROR_1:
    z_free(ret);
    return NULL;
}

void _z_close_udp_multicast(void *sockrecv_arg, void *socksend_arg, void *raddr_arg) {
    __z_net_socket *sockrecv = (__z_net_socket *)sockrecv_arg;
    __z_net_socket *socksend = (__z_net_socket *)socksend_arg;
    struct addrinfo *raddr = (struct addrinfo *)raddr_arg;

    // Both sockrecv and socksend must be compared to NULL,
    //  because we dont know if the close is trigger by a normal close
    //  or some of the sockets failed during the opening/listening procedure.
    if (sockrecv != NULL) {
        if (raddr->ai_family == AF_INET) {
            struct ip_mreq mreq;
            memset(&mreq, 0, sizeof(mreq));
            mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)raddr->ai_addr)->sin_addr.s_addr;
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            setsockopt(sockrecv->_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
        } else if (raddr->ai_family == AF_INET6) {
            struct ipv6_mreq mreq;
            memset(&mreq, 0, sizeof(mreq));
            memcpy(&mreq.ipv6mr_multiaddr, &((struct sockaddr_in6 *)raddr->ai_addr)->sin6_addr,
                   sizeof(struct in6_addr));
            // mreq.ipv6mr_interface = ifindex;
            setsockopt(sockrecv->_fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &mreq, sizeof(mreq));
        }

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

        if (rb < 0) return SIZE_MAX;

        if (laddr->ai_family == AF_INET) {
            struct sockaddr_in *a = ((struct sockaddr_in *)laddr->ai_addr);
            struct sockaddr_in *b = ((struct sockaddr_in *)&raddr);
            if (!(a->sin_port == b->sin_port && a->sin_addr.s_addr == b->sin_addr.s_addr)) {
                // If addr is not NULL, it means that the raddr was requested by the upper-layers
                if (addr != NULL) {
                    *addr = _z_bytes_make(sizeof(in_addr_t) + sizeof(in_port_t));
                    memcpy((void *)addr->start, &b->sin_addr.s_addr, sizeof(in_addr_t));
                    memcpy((void *)(addr->start + sizeof(in_addr_t)), &b->sin_port, sizeof(in_port_t));
                }
                break;
            }
        } else if (laddr->ai_family == AF_INET6) {
            struct sockaddr_in6 *a = ((struct sockaddr_in6 *)laddr->ai_addr);
            struct sockaddr_in6 *b = ((struct sockaddr_in6 *)&raddr);
            if (!(a->sin6_port == b->sin6_port &&
                  memcmp(a->sin6_addr.s6_addr, b->sin6_addr.s6_addr, sizeof(struct in6_addr)) == 0)) {
                // If addr is not NULL, it means that the raddr was requested by the upper-layers
                if (addr != NULL) {
                    *addr = _z_bytes_make(sizeof(struct in6_addr) + sizeof(in_port_t));
                    memcpy((void *)addr->start, &b->sin6_addr.s6_addr, sizeof(struct in6_addr));
                    memcpy((void *)(addr->start + sizeof(struct in6_addr)), &b->sin6_port, sizeof(in_port_t));
                }
                break;
            }
        }
    } while (1);

    return rb;
}

size_t _z_read_exact_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *arg, _z_bytes_t *addr) {
    size_t n = len;
    size_t rb = 0;

    do {
        rb = _z_read_udp_multicast(sock_arg, ptr, n, arg, addr);
        if (rb == SIZE_MAX) return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _z_send_udp_multicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg) {
    __z_net_socket *sock = (__z_net_socket *)sock_arg;
    struct addrinfo *raddr = (struct addrinfo *)raddr_arg;

    return sendto(sock->_fd, ptr, len, 0, raddr->ai_addr, raddr->ai_addrlen);
}

#endif

#if Z_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on Unix port of Zenoh-Pico"
#endif

#if Z_LINK_SERIAL == 1
#error "Serial not supported yet on Unix port of Zenoh-Pico"
#endif
