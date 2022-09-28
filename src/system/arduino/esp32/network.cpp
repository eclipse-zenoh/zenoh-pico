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

#include <BluetoothSerial.h>

extern "C" {
#include <netdb.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/collections/bytes.h"
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
    struct addrinfo *self = static_cast<struct addrinfo *>(arg);

    freeaddrinfo(self);
}

void *_z_open_tcp(void *raddr_arg, uint32_t tout) {
    __z_net_socket *ret = new __z_net_socket();
    struct addrinfo *raddr = static_cast<struct addrinfo *>(raddr_arg);
    int flags = 1;

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0) {
        goto _Z_OPEN_TCP_ERROR_1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0) {
        goto _Z_OPEN_TCP_ERROR_2;
    }

#if LWIP_SO_LINGER == 1
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0) {
        goto _Z_OPEN_TCP_ERROR_2;
    }
#endif

    for (struct addrinfo *it = raddr; it != NULL; it = it->ai_next) {
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
    delete ret;
    return NULL;
}

void *_z_listen_tcp(void *laddr_arg) {
    struct addrinfo *laddr = static_cast<struct addrinfo *>(laddr_arg);
    (void)laddr;

    // @TODO: to be implemented

    return NULL;
}

void _z_close_tcp(void *sock_arg) {
    __z_net_socket *sock = static_cast<__z_net_socket *>(sock_arg);
    if (sock == NULL) {
        return;
    }

    shutdown(sock->_fd, SHUT_RDWR);
    close(sock->_fd);
    delete sock;
}

size_t _z_read_tcp(void *sock_arg, uint8_t *ptr, size_t len) {
    __z_net_socket *sock = static_cast<__z_net_socket *>(sock_arg);
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
    __z_net_socket *sock = static_cast<__z_net_socket *>(sock_arg);
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
    struct addrinfo *self = static_cast<struct addrinfo *>(arg);

    freeaddrinfo(self);
}
#endif

#if Z_LINK_UDP_UNICAST == 1
void *_z_open_udp_unicast(void *raddr_arg, uint32_t tout) {
    __z_net_socket *ret = new __z_net_socket();
    struct addrinfo *raddr = static_cast<struct addrinfo *>(raddr_arg);

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
    delete ret;
    return NULL;
}

void *_z_listen_udp_unicast(void *laddr_arg, unsigned long tout) {
    struct addrinfo *laddr = static_cast<struct addrinfo *>(laddr_arg);
    (void)laddr;

    // @TODO: To be implemented

    return NULL;
}

void _z_close_udp_unicast(void *sock_arg) {
    __z_net_socket *sock = static_cast<__z_net_socket *>(sock_arg);
    if (sock == NULL) {
        return;
    }

    close(sock->_fd);
    delete sock;
}

size_t _z_read_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len) {
    __z_net_socket *sock = static_cast<__z_net_socket *>(sock_arg);

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
    __z_net_socket *sock = static_cast<__z_net_socket *>(sock_arg);
    struct addrinfo *raddr = static_cast<struct addrinfo *>(raddr_arg);

    return sendto(sock->_fd, ptr, len, 0, raddr->ai_addr, raddr->ai_addrlen);
}
#endif

#if Z_LINK_UDP_MULTICAST == 1
void *_z_open_udp_multicast(void *raddr_arg, void **laddr_arg, uint32_t tout, const char *iface) {
    __z_net_socket *ret = new __z_net_socket();
    struct addrinfo *raddr = static_cast<struct addrinfo *>(raddr_arg);
    struct addrinfo *laddr = NULL;
    unsigned int addrlen = 0;
    int sock;

    struct sockaddr *lsockaddr = NULL;
    if (raddr->ai_family == AF_INET) {
        lsockaddr = new struct sockaddr();
        addrlen = sizeof(struct sockaddr_in);

        struct sockaddr_in *c_laddr = reinterpret_cast<struct sockaddr_in *>(lsockaddr);
        c_laddr->sin_family = AF_INET;
        c_laddr->sin_addr.s_addr = INADDR_ANY;
        c_laddr->sin_port = htons(INADDR_ANY);
    } else if (raddr->ai_family == AF_INET6) {
        lsockaddr = new struct sockaddr();
        addrlen = sizeof(struct sockaddr_in6);

        struct sockaddr_in6 *c_laddr = reinterpret_cast<struct sockaddr_in6 *>(lsockaddr);
        c_laddr->sin6_family = AF_INET6;
        c_laddr->sin6_addr = in6addr_any;
        c_laddr->sin6_port = htons(INADDR_ANY);
        //        c_laddr->sin6_scope_id; // Not needed to be defined
    } else {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_1;
    }

    sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
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

    if (getsockname(sock, lsockaddr, &addrlen) < -1) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_3;
    }

    if (lsockaddr->sa_family == AF_INET) {
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &reinterpret_cast<struct sockaddr_in *>(lsockaddr)->sin_addr,
                       sizeof(struct in_addr)) < 0) {
            goto _Z_OPEN_UDP_MULTICAST_ERROR_3;
        }
    } else if (lsockaddr->sa_family == AF_INET6) {
        int ifindex = 0;
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0) {
            goto _Z_OPEN_UDP_MULTICAST_ERROR_3;
        }
    }

    // Create laddr endpoint
    laddr = new struct addrinfo();
    laddr->ai_flags = 0;
    laddr->ai_family = raddr->ai_family;
    laddr->ai_socktype = raddr->ai_socktype;
    laddr->ai_protocol = raddr->ai_protocol;
    laddr->ai_addrlen = addrlen;
    laddr->ai_addr = lsockaddr;
    laddr->ai_canonname = NULL;
    laddr->ai_next = NULL;
    *laddr_arg = laddr;

    ret->_fd = sock;
    return ret;

_Z_OPEN_UDP_MULTICAST_ERROR_3:
    close(sock);

_Z_OPEN_UDP_MULTICAST_ERROR_2:
    delete lsockaddr;

_Z_OPEN_UDP_MULTICAST_ERROR_1:
    delete ret;
    return NULL;
}

void *_z_listen_udp_multicast(void *raddr_arg, uint32_t tout, const char *iface) {
    __z_net_socket *ret = new __z_net_socket();
    struct addrinfo *raddr = static_cast<struct addrinfo *>(raddr_arg);
    struct sockaddr *laddr = NULL;
    unsigned int addrlen = 0;
    int sock;
    int optflag;

    if (raddr->ai_family == AF_INET) {
        laddr = new struct sockaddr();
        addrlen = sizeof(struct sockaddr_in);

        struct sockaddr_in *c_laddr = reinterpret_cast<struct sockaddr_in *>(laddr);
        c_laddr->sin_family = AF_INET;
        c_laddr->sin_addr.s_addr = INADDR_ANY;
        c_laddr->sin_port = reinterpret_cast<struct sockaddr_in *>(raddr->ai_addr)->sin_port;
    } else if (raddr->ai_family == AF_INET6) {
        laddr = new struct sockaddr();
        addrlen = sizeof(struct sockaddr_in6);

        struct sockaddr_in6 *c_laddr = reinterpret_cast<struct sockaddr_in6 *>(laddr);
        c_laddr->sin6_family = AF_INET6;
        c_laddr->sin6_addr = in6addr_any;
        c_laddr->sin6_port = htons(INADDR_ANY);
        c_laddr->sin6_port = reinterpret_cast<struct sockaddr_in6 *>(raddr->ai_addr)->sin6_port;
        //        c_laddr->sin6_scope_id; // Not needed to be defined
    } else {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_1;
    }

    sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_1;
    }

    z_time_t tv;
    tv.tv_sec = tout / (uint32_t)1000;
    tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
    }

    optflag = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&optflag, sizeof(optflag)) < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
    }

    if (bind(sock, laddr, addrlen) < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
    }

    // Join the multicast group
    if (raddr->ai_family == AF_INET) {
        struct ip_mreq mreq;
        (void)memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = reinterpret_cast<struct sockaddr_in *>(raddr->ai_addr)->sin_addr.s_addr;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
        }
    } else if (raddr->ai_family == AF_INET6) {
        struct ipv6_mreq mreq;
        (void)memset(&mreq, 0, sizeof(mreq));
        (void)memcpy(&mreq.ipv6mr_multiaddr, &reinterpret_cast<struct sockaddr_in6 *>(raddr->ai_addr)->sin6_addr,
                     sizeof(struct in6_addr));
        mreq.ipv6mr_interface = 1;  // FIXME: 0 is supposed to be the default interface,
                                    //        but it fails on the setsockopt.
                                    //        1 seems to be a working value on the WiFi interface
                                    //        which is the one available by default in ESP32
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0) {
            goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
        }
    } else {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_2;
    }

    ret->_fd = sock;
    return ret;

_Z_LISTEN_UDP_MULTICAST_ERROR_2:
    close(sock);

_Z_LISTEN_UDP_MULTICAST_ERROR_1:
    delete ret;

    return NULL;
}

void _z_close_udp_multicast(void *sockrecv_arg, void *socksend_arg, void *raddr_arg) {
    __z_net_socket *sockrecv = static_cast<__z_net_socket *>(sockrecv_arg);
    __z_net_socket *socksend = static_cast<__z_net_socket *>(socksend_arg);
    struct addrinfo *raddr = static_cast<struct addrinfo *>(raddr_arg);

    // Both sockrecv and socksend must be compared to NULL,
    //  because we dont know if the close is trigger by a normal close
    //  or some of the sockets failed during the opening/listening procedure.
    if (sockrecv != NULL) {
        if (raddr->ai_family == AF_INET) {
            struct ip_mreq mreq;
            (void)memset(&mreq, 0, sizeof(mreq));
            mreq.imr_multiaddr.s_addr = reinterpret_cast<struct sockaddr_in *>(raddr->ai_addr)->sin_addr.s_addr;
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            setsockopt(sockrecv->_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
        } else if (raddr->ai_family == AF_INET6) {
            struct ipv6_mreq mreq;
            (void)memset(&mreq, 0, sizeof(mreq));
            (void)memcpy(&mreq.ipv6mr_multiaddr, &reinterpret_cast<struct sockaddr_in6 *>(raddr->ai_addr)->sin6_addr,
                         sizeof(struct in6_addr));
            mreq.ipv6mr_interface = 1;  // FIXME: 0 is supposed to be the default interface,
                                        //        but it fails on the setsockopt.
                                        //        1 seems to be a working value on the WiFi interface
                                        //        which is the one available by default in ESP32
            setsockopt(sockrecv->_fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &mreq, sizeof(mreq));
        }

        close(sockrecv->_fd);
        delete sockrecv;
    }

    if (socksend != NULL) {
        close(socksend->_fd);
        delete socksend;
    }
}

size_t _z_read_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *laddr_arg, _z_bytes_t *addr) {
    __z_net_socket *sock = static_cast<__z_net_socket *>(sock_arg);
    struct addrinfo *laddr = static_cast<struct addrinfo *>(laddr_arg);
    struct sockaddr_storage raddr;
    unsigned int raddrlen = sizeof(struct sockaddr_storage);

    size_t rb = 0;
    do {
        rb = recvfrom(sock->_fd, ptr, len, 0, (struct sockaddr *)&raddr, &raddrlen);

        if (rb == SIZE_MAX) {
            return rb;
        }

        if (laddr->ai_family == AF_INET) {
            struct sockaddr_in *a = reinterpret_cast<struct sockaddr_in *>(laddr->ai_addr);
            struct sockaddr_in *b = reinterpret_cast<struct sockaddr_in *>(&raddr);
            if (!((a->sin_port == b->sin_port) && (a->sin_addr.s_addr == b->sin_addr.s_addr))) {
                // If addr is not NULL, it means that the raddr was requested by the upper-layers
                if (addr != NULL) {
                    *addr = _z_bytes_make(sizeof(in_addr_t) + sizeof(in_port_t));
                    (void)memcpy(const_cast<uint8_t *>(addr->start), &b->sin_addr.s_addr, sizeof(in_addr_t));
                    (void)memcpy(const_cast<uint8_t *>(addr->start + sizeof(in_addr_t)), &b->sin_port,
                                 sizeof(in_port_t));
                }
                break;
            }
        } else if (laddr->ai_family == AF_INET6) {
            struct sockaddr_in6 *a = reinterpret_cast<struct sockaddr_in6 *>(laddr->ai_addr);
            struct sockaddr_in6 *b = reinterpret_cast<struct sockaddr_in6 *>(&raddr);
            if (!((a->sin6_port == b->sin6_port) &&
                  (memcmp(a->sin6_addr.s6_addr, b->sin6_addr.s6_addr, sizeof(struct in6_addr)) == 0))) {
                // If addr is not NULL, it means that the raddr was requested by the upper-layers
                if (addr != NULL) {
                    *addr = _z_bytes_make(sizeof(struct in6_addr) + sizeof(in_port_t));
                    (void)memcpy(const_cast<uint8_t *>(addr->start), &b->sin6_addr.s6_addr, sizeof(struct in6_addr));
                    (void)memcpy(const_cast<uint8_t *>(addr->start + sizeof(struct in6_addr)), &b->sin6_port,
                                 sizeof(in_port_t));
                }
                break;
            }
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
    __z_net_socket *sock = static_cast<__z_net_socket *>(sock_arg);
    struct addrinfo *raddr = static_cast<struct addrinfo *>(raddr_arg);

    return sendto(sock->_fd, ptr, len, 0, raddr->ai_addr, raddr->ai_addrlen);
}
#endif

#if Z_LINK_BLUETOOTH == 1
#include "zenoh-pico/system/link/bt.h"
/*------------------ Bluetooth sockets ------------------*/
void *_z_open_bt(char *gname, uint8_t mode, uint8_t profile) {
    switch (profile) {
        case _Z_BT_PROFILE_SPP: {
            BluetoothSerial *sbt = new BluetoothSerial();
            if (mode == _Z_BT_MODE_SLAVE) {
                sbt->begin(gname, false);
            } else if (mode == _Z_BT_MODE_MASTER) {
                sbt->begin(gname, true);
                uint8_t connected = sbt->connect(gname);
                if (!connected) {
                    while (!sbt->connected(10000)) {
                        __asm__("nop");
                    }
                }
            } else {
                return NULL;
            }

            return sbt;
        } break;

        case _Z_BT_PROFILE_UNSUPPORTED:
        default: {
            return NULL;
        }
    }
}

void *_z_listen_bt(char *gname, uint8_t mode, uint8_t profile) {
    switch (profile) {
        case _Z_BT_PROFILE_SPP: {
            BluetoothSerial *sbt = new BluetoothSerial();
            if (mode == _Z_BT_MODE_SLAVE) {
                sbt->begin(gname, false);
            } else if (mode == _Z_BT_MODE_MASTER) {
                sbt->begin(gname, true);
                uint8_t connected = sbt->connect(gname);
                if (!connected) {
                    while (!sbt->connected(10000)) {
                        __asm__("nop");
                    }
                }
            } else {
                return NULL;
            }

            return sbt;
        } break;

        case _Z_BT_PROFILE_UNSUPPORTED:
        default: {
            return NULL;
        }
    }
}

void _z_close_bt(void *sock_arg) {
    BluetoothSerial *sock = static_cast<BluetoothSerial *>(sock_arg);
    if (sock == NULL) {
        return;
    }

    sock->end();
    delete sock;
}

size_t _z_read_bt(void *sock_arg, uint8_t *ptr, size_t len) {
    BluetoothSerial *sock = static_cast<BluetoothSerial *>(sock_arg);
    for (int i = 0; i < len; i++) {
        int c = sock->read();
        if (c == -1) {
            delay(1);  // FIXME: without this, the read task is blocking the other tasks
            return i;
        }
        ptr[i] = c;
    }

    return len;
}

size_t _z_read_exact_bt(void *sock_arg, uint8_t *ptr, size_t len) {
    BluetoothSerial *sock = static_cast<BluetoothSerial *>(sock_arg);
    size_t n = len;

    do {
        size_t rb = _z_read_bt(sock, ptr, n);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_bt(void *sock_arg, const uint8_t *ptr, size_t len) {
    BluetoothSerial *sock = static_cast<BluetoothSerial *>(sock_arg);
    sock->write(ptr, len);

    return len;
}
#endif

#if Z_LINK_SERIAL == 1
#error "Serial not supported yet on Arduino port of Zenoh-Pico"
#endif

}  // extern "C"