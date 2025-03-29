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
#include <assert.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"
#include <fcntl.h>
#include <termios.h>
#include "zenoh-pico/system/link/serial.h"
#include "zenoh-pico/utils/checksum.h"
#include "zenoh-pico/utils/encoding.h"

#if Z_FEATURE_LINK_TCP == 1

/*------------------ TCP sockets ------------------*/
z_result_t _z_create_endpoint_tcp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    struct addrinfo hints;
    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(s_address, s_port, &hints, &ep->_iptcp) < 0) {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_free_endpoint_tcp(_z_sys_net_endpoint_t *ep) { freeaddrinfo(ep->_iptcp); }

/*------------------ TCP sockets ------------------*/
z_result_t _z_open_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_fd = socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol);
    if (sock->_fd != -1) {
        z_time_t tv;
        tv.tv_sec = tout / (uint32_t)1000;
        tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
        if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }

        int flags = 1;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }
#if Z_FEATURE_TCP_NODELAY == 1
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }
#endif
        struct linger ling;
        ling.l_onoff = 1;
        ling.l_linger = Z_TRANSPORT_LEASE / 1000;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }

#if defined(ZENOH_MACOS) || defined(ZENOH_BSD)
        setsockopt(sock->_fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)0, sizeof(int));
#endif

        struct addrinfo *it = NULL;
        for (it = rep._iptcp; it != NULL; it = it->ai_next) {
            if ((ret == _Z_RES_OK) && (connect(sock->_fd, it->ai_addr, it->ai_addrlen) < 0)) {
                if (it->ai_next == NULL) {
                    ret = _Z_ERR_GENERIC;
                    break;
                }
            } else {
                break;
            }
        }

        if (ret != _Z_RES_OK) {
            close(sock->_fd);
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

z_result_t _z_listen_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep) {
    z_result_t ret = _Z_RES_OK;
    // Open socket
    sock->_fd = socket(lep._iptcp->ai_family, lep._iptcp->ai_socktype, lep._iptcp->ai_protocol);
    if (sock->_fd == -1) {
        return _Z_ERR_GENERIC;
    }
    // Set options
    int value = true;
    if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0)) {
        ret = _Z_ERR_GENERIC;
    }
    int flags = 1;
    if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0)) {
        ret = _Z_ERR_GENERIC;
    }
#if Z_FEATURE_TCP_NODELAY == 1
    if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) < 0)) {
        ret = _Z_ERR_GENERIC;
    }
#endif
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if ((ret == _Z_RES_OK) &&
        (setsockopt(sock->_fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)) {
        ret = _Z_ERR_GENERIC;
    }
#if defined(ZENOH_MACOS) || defined(ZENOH_BSD)
    setsockopt(sock->_fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)0, sizeof(int));
#endif
    if (ret != _Z_RES_OK) {
        close(sock->_fd);
        return ret;
    }
    struct addrinfo *it = NULL;
    for (it = lep._iptcp; it != NULL; it = it->ai_next) {
        if (bind(sock->_fd, it->ai_addr, it->ai_addrlen) < 0) {
            if (it->ai_next == NULL) {
                ret = _Z_ERR_GENERIC;
                break;
            }
        }
        if (listen(sock->_fd, 1) < 0) {
            if (it->ai_next == NULL) {
                ret = _Z_ERR_GENERIC;
                break;
            }
        }
        struct sockaddr naddr;
        unsigned int nlen = sizeof(naddr);
        int con_socket = accept(sock->_fd, &naddr, &nlen);
        if (con_socket < 0) {
            if (it->ai_next == NULL) {
                ret = _Z_ERR_GENERIC;
                break;
            }
        } else {
            sock->_fd = con_socket;
            break;
        }
    }
    if (ret != _Z_RES_OK) {
        close(sock->_fd);
    }
    return ret;
}

void _z_close_tcp(_z_sys_net_socket_t *sock) {
    shutdown(sock->_fd, SHUT_RDWR);
    close(sock->_fd);
}

size_t _z_read_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    ssize_t rb = recv(sock._fd, ptr, len, 0);
    if (rb < (ssize_t)0) {
        return SIZE_MAX;
    }

    return (size_t)rb;
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
        pos = _z_ptr_u8_offset(pos, (ptrdiff_t)n);
    } while (n != len);

    return n;
}

size_t _z_send_tcp(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
#if defined(ZENOH_LINUX)
    return (size_t)send(sock._fd, ptr, len, MSG_NOSIGNAL);
#else
    return send(sock._fd, ptr, len, 0);
#endif
}
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
z_result_t _z_create_endpoint_udp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    struct addrinfo hints;
    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(s_address, s_port, &hints, &ep->_iptcp) < 0) {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_free_endpoint_udp(_z_sys_net_endpoint_t *ep) { freeaddrinfo(ep->_iptcp); }
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1
z_result_t _z_open_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_fd = socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol);
    if (sock->_fd != -1) {
        z_time_t tv;
        tv.tv_sec = tout / (uint32_t)1000;
        tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
        if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }

        if (ret != _Z_RES_OK) {
            close(sock->_fd);
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

z_result_t _z_listen_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep, uint32_t tout) {
    (void)sock;
    (void)lep;
    (void)tout;
    z_result_t ret = _Z_RES_OK;

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

void _z_close_udp_unicast(_z_sys_net_socket_t *sock) { close(sock->_fd); }

size_t _z_read_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    struct sockaddr_storage raddr;
    unsigned int addrlen = sizeof(struct sockaddr_storage);

    ssize_t rb = recvfrom(sock._fd, ptr, len, 0, (struct sockaddr *)&raddr, &addrlen);
    if (rb < (ssize_t)0) {
        return SIZE_MAX;
    }
    return (size_t)rb;
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
        pos = _z_ptr_u8_offset(pos, (ptrdiff_t)n);
    } while (n != len);

    return n;
}

size_t _z_send_udp_unicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                           const _z_sys_net_endpoint_t rep) {
    return (size_t)sendto(sock._fd, ptr, len, 0, rep._iptcp->ai_addr, rep._iptcp->ai_addrlen);
}
#endif

#if Z_FEATURE_LINK_UDP_MULTICAST == 1
unsigned int __get_ip_from_iface(const char *iface, int sa_family, struct sockaddr **lsockaddr) {
    unsigned int addrlen = 0U;

    struct ifaddrs *l_ifaddr = NULL;
    if (getifaddrs(&l_ifaddr) != -1) {
        struct ifaddrs *tmp = NULL;
        for (tmp = l_ifaddr; tmp != NULL; tmp = tmp->ifa_next) {
            if (_z_str_eq(tmp->ifa_name, iface) == true) {
                if (tmp->ifa_addr->sa_family == sa_family) {
                    if (tmp->ifa_addr->sa_family == AF_INET) {
                        *lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in));
                        if (lsockaddr != NULL) {
                            (void)memset(*lsockaddr, 0, sizeof(struct sockaddr_in));
                            (void)memcpy(*lsockaddr, tmp->ifa_addr, sizeof(struct sockaddr_in));
                            addrlen = sizeof(struct sockaddr_in);
                        }
                    } else if (tmp->ifa_addr->sa_family == AF_INET6) {
                        *lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in6));
                        if (lsockaddr != NULL) {
                            (void)memset(*lsockaddr, 0, sizeof(struct sockaddr_in6));
                            (void)memcpy(*lsockaddr, tmp->ifa_addr, sizeof(struct sockaddr_in6));
                            addrlen = sizeof(struct sockaddr_in6);
                        }
                    } else {
                        continue;
                    }

                    break;
                }
            }
        }
        freeifaddrs(l_ifaddr);
    }

    return addrlen;
}

z_result_t _z_open_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                                 uint32_t tout, const char *iface) {
    z_result_t ret = _Z_RES_OK;

    struct sockaddr *lsockaddr = NULL;
    unsigned int addrlen = __get_ip_from_iface(iface, rep._iptcp->ai_family, &lsockaddr);
    if (addrlen != 0U) {
        sock->_fd = socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol);
        if (sock->_fd != -1) {
            z_time_t tv;
            tv.tv_sec = tout / (uint32_t)1000;
            tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
            if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

            if ((ret == _Z_RES_OK) && (bind(sock->_fd, lsockaddr, addrlen) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

            // Get the randomly assigned port used to discard loopback messages
            if ((ret == _Z_RES_OK) && (getsockname(sock->_fd, lsockaddr, &addrlen) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

#ifndef UNIX_NO_MULTICAST_IF
            if (lsockaddr->sa_family == AF_INET) {
                if ((ret == _Z_RES_OK) &&
                    (setsockopt(sock->_fd, IPPROTO_IP, IP_MULTICAST_IF, &((struct sockaddr_in *)lsockaddr)->sin_addr,
                                sizeof(struct in_addr)) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else if (lsockaddr->sa_family == AF_INET6) {
                int ifindex = (int)if_nametoindex(iface);
                if ((ret == _Z_RES_OK) &&
                    (setsockopt(sock->_fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                ret = _Z_ERR_GENERIC;
            }
#endif

            // Create lep endpoint
            if (ret == _Z_RES_OK) {
                struct addrinfo *laddr = (struct addrinfo *)z_malloc(sizeof(struct addrinfo));
                if (laddr != NULL) {
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
                    ret = _Z_ERR_GENERIC;
                }
            }

            if (ret != _Z_RES_OK) {
                close(sock->_fd);
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

z_result_t _z_listen_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                                   const char *iface, const char *join) {
    z_result_t ret = _Z_RES_OK;

    struct sockaddr *lsockaddr = NULL;
    unsigned int addrlen = __get_ip_from_iface(iface, rep._iptcp->ai_family, &lsockaddr);
    if (addrlen != 0U) {
        sock->_fd = socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol);
        if (sock->_fd != -1) {
            z_time_t tv;
            tv.tv_sec = tout / (uint32_t)1000;
            tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
            if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

            int value = true;
            if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0)) {
                ret = _Z_ERR_GENERIC;
            }
#ifdef SO_REUSEPORT
            if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value)) < 0)) {
                ret = _Z_ERR_GENERIC;
            }
#endif

            if (rep._iptcp->ai_family == AF_INET) {
                struct sockaddr_in address;
                (void)memset(&address, 0, sizeof(address));
                address.sin_family = (sa_family_t)rep._iptcp->ai_family;
                address.sin_port = ((struct sockaddr_in *)rep._iptcp->ai_addr)->sin_port;
                inet_pton(address.sin_family, "0.0.0.0", &address.sin_addr);
                if ((ret == _Z_RES_OK) && (bind(sock->_fd, (struct sockaddr *)&address, sizeof(address)) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else if (rep._iptcp->ai_family == AF_INET6) {
                struct sockaddr_in6 address;
                (void)memset(&address, 0, sizeof(address));
                address.sin6_family = (sa_family_t)rep._iptcp->ai_family;
                address.sin6_port = ((struct sockaddr_in6 *)rep._iptcp->ai_addr)->sin6_port;
                inet_pton(address.sin6_family, "::", &address.sin6_addr);
                if ((ret == _Z_RES_OK) && (bind(sock->_fd, (struct sockaddr *)&address, sizeof(address)) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                ret = _Z_ERR_GENERIC;
            }

            // Join the multicast group
            if (rep._iptcp->ai_family == AF_INET) {
                struct ip_mreq mreq;
                (void)memset(&mreq, 0, sizeof(mreq));
                mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)rep._iptcp->ai_addr)->sin_addr.s_addr;
                mreq.imr_interface.s_addr = ((struct sockaddr_in *)lsockaddr)->sin_addr.s_addr;
                if ((ret == _Z_RES_OK) &&
                    (setsockopt(sock->_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else if (rep._iptcp->ai_family == AF_INET6) {
                struct ipv6_mreq mreq;
                (void)memset(&mreq, 0, sizeof(mreq));
                (void)memcpy(&mreq.ipv6mr_multiaddr, &((struct sockaddr_in6 *)rep._iptcp->ai_addr)->sin6_addr,
                             sizeof(struct in6_addr));
                mreq.ipv6mr_interface = if_nametoindex(iface);
                if ((ret == _Z_RES_OK) &&
                    (setsockopt(sock->_fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                ret = _Z_ERR_GENERIC;
            }
            // Join any additional multicast group
            if (join != NULL) {
                char *joins = _z_str_clone(join);
                for (char *ip = strsep(&joins, "|"); ip != NULL; ip = strsep(&joins, "|")) {
                    if (rep._iptcp->ai_family == AF_INET) {
                        struct ip_mreq mreq;
                        (void)memset(&mreq, 0, sizeof(mreq));
                        inet_pton(rep._iptcp->ai_family, ip, &mreq.imr_multiaddr);
                        mreq.imr_interface.s_addr = ((struct sockaddr_in *)lsockaddr)->sin_addr.s_addr;
                        if ((ret == _Z_RES_OK) &&
                            (setsockopt(sock->_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)) {
                            ret = _Z_ERR_GENERIC;
                        }
                    } else if (rep._iptcp->ai_family == AF_INET6) {
                        struct ipv6_mreq mreq;
                        (void)memset(&mreq, 0, sizeof(mreq));
                        inet_pton(rep._iptcp->ai_family, ip, &mreq.ipv6mr_multiaddr);
                        mreq.ipv6mr_interface = if_nametoindex(iface);
                        if ((ret == _Z_RES_OK) &&
                            (setsockopt(sock->_fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0)) {
                            ret = _Z_ERR_GENERIC;
                        }
                    }
                }
                z_free(joins);
            }

            if (ret != _Z_RES_OK) {
                close(sock->_fd);
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
                            const _z_sys_net_endpoint_t rep, const _z_sys_net_endpoint_t lep) {
    if (rep._iptcp->ai_family == AF_INET) {
        struct ip_mreq mreq;
        (void)memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)rep._iptcp->ai_addr)->sin_addr.s_addr;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        setsockopt(sockrecv->_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    } else if (rep._iptcp->ai_family == AF_INET6) {
        struct ipv6_mreq mreq;
        (void)memset(&mreq, 0, sizeof(mreq));
        (void)memcpy(&mreq.ipv6mr_multiaddr, &((struct sockaddr_in6 *)rep._iptcp->ai_addr)->sin6_addr,
                     sizeof(struct in6_addr));
        // mreq.ipv6mr_interface = ifindex;
        setsockopt(sockrecv->_fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &mreq, sizeof(mreq));
    } else {
        // Do nothing. It must never not enter here.
        // Required to be compliant with MISRA 15.7 rule
    }
#if defined(ZENOH_LINUX)
    if (lep._iptcp != NULL) {
        z_free(lep._iptcp->ai_addr);
    }
#else
    _ZP_UNUSED(lep);
#endif
    close(sockrecv->_fd);
    close(socksend->_fd);
}

size_t _z_read_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_slice_t *addr) {
    struct sockaddr_storage raddr;
    unsigned int replen = sizeof(struct sockaddr_storage);

    ssize_t rb = 0;
    do {
        rb = recvfrom(sock._fd, ptr, len, 0, (struct sockaddr *)&raddr, &replen);
        if (rb < (ssize_t)0) {
            return SIZE_MAX;
        }

        if (lep._iptcp->ai_family == AF_INET) {
            struct sockaddr_in *a = ((struct sockaddr_in *)lep._iptcp->ai_addr);
            struct sockaddr_in *b = ((struct sockaddr_in *)&raddr);
            if (!((a->sin_port == b->sin_port) && (a->sin_addr.s_addr == b->sin_addr.s_addr))) {
                // If addr is not NULL, it means that the rep was requested by the upper-layers
                if (addr != NULL) {
                    assert(addr->len >= sizeof(in_addr_t) + sizeof(in_port_t));
                    addr->len = sizeof(in_addr_t) + sizeof(in_port_t);
                    (void)memcpy((uint8_t *)addr->start, &b->sin_addr.s_addr, sizeof(in_addr_t));
                    (void)memcpy((uint8_t *)(addr->start + sizeof(in_addr_t)), &b->sin_port, sizeof(in_port_t));
                }
                break;
            }
        } else if (lep._iptcp->ai_family == AF_INET6) {
            struct sockaddr_in6 *a = ((struct sockaddr_in6 *)lep._iptcp->ai_addr);
            struct sockaddr_in6 *b = ((struct sockaddr_in6 *)&raddr);
            if (!((a->sin6_port == b->sin6_port) &&
                  (memcmp(a->sin6_addr.s6_addr, b->sin6_addr.s6_addr, sizeof(struct in6_addr)) == 0))) {
                // If addr is not NULL, it means that the rep was requested by the upper-layers
                if (addr != NULL) {
                    assert(addr->len >= sizeof(struct in6_addr) + sizeof(in_port_t));
                    addr->len = sizeof(struct in6_addr) + sizeof(in_port_t);
                    (void)memcpy((uint8_t *)addr->start, &b->sin6_addr.s6_addr, sizeof(struct in6_addr));
                    (void)memcpy((uint8_t *)(addr->start + sizeof(struct in6_addr)), &b->sin6_port, sizeof(in_port_t));
                }
                break;
            }
        } else {
            continue;  // FIXME: support error report on invalid packet to the upper layer
        }
    } while (1);

    return (size_t)rb;
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
        pos = _z_ptr_u8_offset(pos, (ptrdiff_t)n);
    } while (n != len);

    return n;
}

size_t _z_send_udp_multicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                             const _z_sys_net_endpoint_t rep) {
    return (size_t)sendto(sock._fd, ptr, len, 0, rep._iptcp->ai_addr, rep._iptcp->ai_addrlen);
}

#endif

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on Unix port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_SERIAL == 1
int8_t _z_open_serial_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    int8_t ret = _Z_RES_OK;
    (void)(sock);
    (void)(txpin);
    (void)(rxpin);
    (void)(baudrate);

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

int8_t _z_open_serial_from_dev(_z_sys_net_socket_t *sock, char *dev, uint32_t baudrate) {
    int8_t ret = _Z_RES_OK;

    char fname[64];
    struct termios tty;
    speed_t speed;

    // Open serial device
    snprintf(fname, sizeof(fname), "/dev/%s", dev);
    sock->_serial = open(fname, O_RDWR);
    if (sock->_serial < 0) {
        _z_close_serial(sock);
        return _Z_ERR_GENERIC;
    }

    // Acquire non-blocking exclusive lock
    if(flock(sock->_serial , LOCK_EX | LOCK_NB) == -1) {
        _z_close_serial(sock);
        return _Z_ERR_GENERIC;
    }

    // Setup device parameters
    speed_t baud[] = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 500000, 576000, 921600,
                      1000000, 1152000, 1500000, 2000000, 2500000, 3000000, 3500000, 4000000};
    int _baud[] = {B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600,
                   B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000};
    speed = B0;
    for (size_t i = 0; i < _ZP_ARRAY_SIZE(baud); i++) {
        if (baud[i] == baudrate) {
            speed = _baud[i];
            break;
        }
    }
    if (speed == B0) {
        _z_close_serial(sock);
        return _Z_ERR_GENERIC;
    }
    if (tcgetattr(sock->_serial, &tty) != 0) {
        _z_close_serial(sock);
        return _Z_ERR_GENERIC;
    }
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);
    tty.c_cflag &= ~CSIZE;          // Clear all the size bits
    tty.c_cflag |= CS8;             // 8 bits per byte
    tty.c_cflag &= ~PARENB;         // Clear parity bit, disabling parity
    tty.c_cflag &= ~CSTOPB;         // Clear stop field, only one stop bit used in communication
    tty.c_cflag &= ~CRTSCTS;        // Disable RTS/CTS hardware flow control
    tty.c_cflag |= CREAD | CLOCAL;  // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    tty.c_lflag &= ~ICANON;         // Disable canonical mode
    tty.c_lflag &= ~ECHO;           // Disable echo
    tty.c_lflag &= ~ECHOE; 			// Disable erasure
    tty.c_lflag &= ~ECHONL; 		// Disable new-line echo    
    tty.c_lflag &= ~ISIG;           // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON|IXOFF|IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
    tty.c_oflag &= ~OPOST;          // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR;          // Prevent conversion of newline to carriage return/line feed
	tty.c_cc[VTIME] = 10;			// Wait for up to 1s (10 deciseconds), returning as soon as any data is received
 	tty.c_cc[VMIN] = 0;    			//

    if (tcsetattr(sock->_serial, TCSANOW, &tty) != 0) {
        _z_close_serial(sock);
        return _Z_ERR_GENERIC;
    }

    // Flush serial port input buffer
    usleep(20000);
    tcflush(sock->_serial, TCIFLUSH);

    // Allocate buffers
    sock->r_before_cobs = (uint8_t *)z_malloc(_Z_SERIAL_MAX_COBS_BUF_SIZE);
    sock->r_after_cobs  = (uint8_t *)z_malloc(_Z_SERIAL_MFS_SIZE);
    if (sock->r_before_cobs == NULL || sock->r_after_cobs == NULL) {
        _z_close_serial(sock);
        return _Z_ERR_GENERIC;
    }
    sock->w_before_cobs = (uint8_t *)z_malloc(_Z_SERIAL_MFS_SIZE);
    sock->w_after_cobs  = (uint8_t *)z_malloc(_Z_SERIAL_MAX_COBS_BUF_SIZE);
    if (sock->w_before_cobs == NULL || sock->w_after_cobs == NULL) {
        _z_close_serial(sock);
        return _Z_ERR_GENERIC;
    }

    return ret;
}

int8_t _z_listen_serial_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    int8_t ret = _Z_RES_OK;
    (void)(sock);
    (void)(txpin);
    (void)(rxpin);
    (void)(baudrate);

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

int8_t _z_listen_serial_from_dev(_z_sys_net_socket_t *sock, char *dev, uint32_t baudrate) {
    int8_t ret = _Z_RES_OK;

    ret = _z_open_serial_from_dev(sock, dev, baudrate);

    return ret;
}

void _z_close_serial(_z_sys_net_socket_t *sock) {
    if (sock->r_before_cobs != NULL) {
        z_free(sock->r_before_cobs);
    }
    if (sock->r_after_cobs != NULL) {
        z_free(sock->r_after_cobs);
    }
    if (sock->w_before_cobs != NULL) {
        z_free(sock->w_before_cobs);
    }
    if (sock->w_after_cobs != NULL) {
        z_free(sock->w_after_cobs);
    }
    if (sock->_serial >= 0) {
        close(sock->_serial);
    }
}

size_t _z_read_serial_internal(const _z_sys_net_socket_t sock, uint8_t *header, uint8_t *ptr, size_t len) {
    int8_t ret = _Z_RES_OK;
    (void)(header);
    (void)(len);

    size_t rb = 0;
    while (rb < _Z_SERIAL_MAX_COBS_BUF_SIZE) {

    	size_t r = 0;
        r = read(sock._serial, &sock.r_before_cobs[rb], 1);

        if (r == 0) {
            _Z_DEBUG("Timeout reading from serial");
            if (rb == 0) {
                ret = _Z_ERR_GENERIC;
            }
            break;
        } else if (r == 1) {
            rb = rb + (size_t)1;
            if (sock.r_before_cobs[rb - 1] == (uint8_t)0x00) {
                break;
            }
        } else {
            _Z_ERROR("Error reading from serial");
            ret = _Z_ERR_GENERIC;
        }
    }
    uint16_t payload_len = 0;

    if (ret == _Z_RES_OK) {
        _Z_DEBUG("Read %zu bytes from serial", rb);
        size_t trb = _z_cobs_decode(sock.r_before_cobs, rb, sock.r_after_cobs);
        _Z_DEBUG("Decoded %zu bytes from serial", trb);
        size_t i = 0;
        for (i = 0; i < sizeof(payload_len); i++) {
            payload_len |= (sock.r_after_cobs[i] << ((uint8_t)i * (uint8_t)8));
        }
        _Z_DEBUG("payload_len = %u <= %X %X", payload_len, sock.r_after_cobs[1], sock.r_after_cobs[0]);

        if (trb == (size_t)(payload_len + (uint16_t)6)) {
            (void)memcpy(ptr, &sock.r_after_cobs[i], payload_len);
            i = i + (size_t)payload_len;

            uint32_t crc = 0;
            for (uint8_t j = 0; j < sizeof(crc); j++) {
                crc |= (uint32_t)(sock.r_after_cobs[i] << (j * (uint8_t)8));
                i = i + (size_t)1;
            }

            uint32_t c_crc = _z_crc32(ptr, payload_len);
            if (c_crc != crc) {
                _Z_ERROR("CRC mismatch: %d != %d ", c_crc, crc);
                ret = _Z_ERR_GENERIC;
            }
        } else {
            _Z_ERROR("length mismatch => %zd <> %d ", trb, payload_len + (uint16_t)6);
            ret = _Z_ERR_GENERIC;
        }
    }

    rb = payload_len;
    if (ret != _Z_RES_OK) {
        rb = SIZE_MAX;
    }
    _Z_DEBUG("return _z_read_serial() = %zu ", rb);
    return rb;
}

size_t _z_send_serial_internal(const _z_sys_net_socket_t sock, uint8_t header, const uint8_t *ptr, size_t len) {
    int8_t ret = _Z_RES_OK;
    (void)(header);
    (void)(len);

    size_t i = 0;
    for (i = 0; i < sizeof(uint16_t); ++i) {
        sock.w_before_cobs[i] = (len >> (i * (size_t)8)) & (size_t)0XFF;
    }

    (void)memcpy(&sock.w_before_cobs[i], ptr, len);
    i = i + len;

    uint32_t crc = _z_crc32(ptr, len);
    for (uint8_t j = 0; j < sizeof(crc); j++) {
        sock.w_before_cobs[i] = (crc >> (j * (uint8_t)8)) & (uint32_t)0XFF;
        i = i + (size_t)1;
    }

    size_t twb = _z_cobs_encode(sock.w_before_cobs, i, sock.w_after_cobs);
    sock.w_after_cobs[twb] = 0x00;  // Manually add the COBS delimiter
    size_t wb = 0;
    wb = write(sock._serial, sock.w_after_cobs, twb + (size_t)1);

    if (wb != (twb + (size_t)1)) {
        ret = _Z_ERR_GENERIC;
    }

    size_t sb = len;
    if (ret != _Z_RES_OK) {
        sb = SIZE_MAX;
    }

    return sb;
}
#endif
