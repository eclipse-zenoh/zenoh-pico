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
#include <HardwareSerial.h>

extern "C" {
#include <netdb.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/link/bt.h"
#include "zenoh-pico/system/link/serial.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/checksum.h"
#include "zenoh-pico/utils/encoding.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_FEATURE_LINK_TCP == 1
/*------------------ TCP sockets ------------------*/
int8_t _z_create_endpoint_tcp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    int8_t ret = _Z_RES_OK;

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

int8_t _z_open_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    int8_t ret = _Z_RES_OK;

    sock->_fd = socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol);
    if (sock->_fd != -1) {
        int optflag = 1;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&optflag, sizeof(optflag)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }

#if LWIP_SO_LINGER == 1
        struct linger ling;
        ling.l_onoff = 1;
        ling.l_linger = Z_TRANSPORT_LEASE / 1000;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }
#endif

        for (struct addrinfo *it = rep._iptcp; it != NULL; it = it->ai_next) {
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

int8_t _z_listen_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep) {
    int8_t ret = _Z_RES_OK;
    (void)sock;
    (void)lep;

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

void _z_close_tcp(_z_sys_net_socket_t *sock) {
    shutdown(sock->_fd, SHUT_RDWR);
    close(sock->_fd);
}

size_t _z_read_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    ssize_t rb = recv(sock._fd, ptr, len, 0);
    if (rb < (ssize_t)0) {
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
    return send(sock._fd, ptr, len, 0);
}
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
int8_t _z_create_endpoint_udp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    int8_t ret = _Z_RES_OK;

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
int8_t _z_open_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    int8_t ret = _Z_RES_OK;

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

int8_t _z_listen_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep, uint32_t tout) {
    int8_t ret = _Z_RES_OK;
    (void)sock;
    (void)lep;
    (void)tout;

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
    return sendto(sock._fd, ptr, len, 0, rep._iptcp->ai_addr, rep._iptcp->ai_addrlen);
}
#endif

#if Z_FEATURE_LINK_UDP_MULTICAST == 1
int8_t _z_open_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                             uint32_t tout, const char *iface) {
    int8_t ret = _Z_RES_OK;

    struct sockaddr *lsockaddr = NULL;
    unsigned int addrlen = 0;
    if (rep._iptcp->ai_family == AF_INET) {
        lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in));
        if (lsockaddr != NULL) {
            (void)memset(lsockaddr, 0, sizeof(struct sockaddr_in));
            addrlen = sizeof(struct sockaddr_in);

            struct sockaddr_in *c_laddr = (struct sockaddr_in *)lsockaddr;
            c_laddr->sin_family = AF_INET;
            c_laddr->sin_addr.s_addr = INADDR_ANY;
            c_laddr->sin_port = htons(INADDR_ANY);
        } else {
            ret = _Z_ERR_GENERIC;
        }
    } else if (rep._iptcp->ai_family == AF_INET6) {
        lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in6));
        if (lsockaddr != NULL) {
            (void)memset(lsockaddr, 0, sizeof(struct sockaddr_in6));
            addrlen = sizeof(struct sockaddr_in6);

            struct sockaddr_in6 *c_laddr = (struct sockaddr_in6 *)lsockaddr;
            c_laddr->sin6_family = AF_INET6;
            c_laddr->sin6_addr = in6addr_any;
            c_laddr->sin6_port = htons(INADDR_ANY);
            //        c_laddr->sin6_scope_id; // Not needed to be defined
        } else {
            ret = _Z_ERR_GENERIC;
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

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

            if ((ret == _Z_RES_OK) && (getsockname(sock->_fd, lsockaddr, &addrlen) == -1)) {
                ret = _Z_ERR_GENERIC;
            }

            if (lsockaddr->sa_family == AF_INET) {
                if ((ret == _Z_RES_OK) &&
                    (setsockopt(sock->_fd, IPPROTO_IP, IP_MULTICAST_IF, &((struct sockaddr_in *)lsockaddr)->sin_addr,
                                sizeof(struct in_addr)) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else if (lsockaddr->sa_family == AF_INET6) {
                int ifindex = 0;
                if ((ret == _Z_RES_OK) &&
                    (setsockopt(sock->_fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                ret = _Z_ERR_GENERIC;
            }

            // Create laddr endpoint
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

int8_t _z_listen_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                               const char *iface, const char *join) {
    (void)join;
    int8_t ret = _Z_RES_OK;

    struct sockaddr *lsockaddr = NULL;
    unsigned int addrlen = 0;
    if (rep._iptcp->ai_family == AF_INET) {
        lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in));
        if (lsockaddr != NULL) {
            (void)memset(lsockaddr, 0, sizeof(struct sockaddr_in));
            addrlen = sizeof(struct sockaddr_in);

            struct sockaddr_in *c_laddr = (struct sockaddr_in *)lsockaddr;
            c_laddr->sin_family = AF_INET;
            c_laddr->sin_addr.s_addr = INADDR_ANY;
            c_laddr->sin_port = ((struct sockaddr_in *)rep._iptcp->ai_addr)->sin_port;
        } else {
            ret = _Z_ERR_GENERIC;
        }
    } else if (rep._iptcp->ai_family == AF_INET6) {
        lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in6));
        if (lsockaddr != NULL) {
            (void)memset(lsockaddr, 0, sizeof(struct sockaddr_in6));
            addrlen = sizeof(struct sockaddr_in6);

            struct sockaddr_in6 *c_laddr = (struct sockaddr_in6 *)lsockaddr;
            c_laddr->sin6_family = AF_INET6;
            c_laddr->sin6_addr = in6addr_any;
            c_laddr->sin6_port = htons(INADDR_ANY);
            c_laddr->sin6_port = ((struct sockaddr_in6 *)rep._iptcp->ai_addr)->sin6_port;
            //        c_laddr->sin6_scope_id; // Not needed to be defined
        } else {
            ret = _Z_ERR_GENERIC;
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    if (addrlen != 0U) {
        sock->_fd = socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol);
        if (sock->_fd != -1) {
            z_time_t tv;
            tv.tv_sec = tout / (uint32_t)1000;
            tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
            if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

            int optflag = 1;
            if ((ret == _Z_RES_OK) &&
                (setsockopt(sock->_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optflag, sizeof(optflag)) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

            if ((ret == _Z_RES_OK) && (bind(sock->_fd, lsockaddr, addrlen) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

            // Join the multicast group
            if (rep._iptcp->ai_family == AF_INET) {
                struct ip_mreq mreq;
                (void)memset(&mreq, 0, sizeof(mreq));
                mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)rep._iptcp->ai_addr)->sin_addr.s_addr;
                mreq.imr_interface.s_addr = htonl(INADDR_ANY);
                if ((ret == _Z_RES_OK) &&
                    (setsockopt(sock->_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else if (rep._iptcp->ai_family == AF_INET6) {
                struct ipv6_mreq mreq;
                (void)memset(&mreq, 0, sizeof(mreq));
                (void)memcpy(&mreq.ipv6mr_multiaddr, &((struct sockaddr_in6 *)rep._iptcp->ai_addr)->sin6_addr,
                             sizeof(struct in6_addr));
                mreq.ipv6mr_interface = 1;  // FIXME: 0 is supposed to be the default interface,
                                            //        but it fails on the setsockopt.
                                            //        1 seems to be a working value on the WiFi interface
                                            //        which is the one available by default in ESP32
                if ((ret == _Z_RES_OK) &&
                    (setsockopt(sock->_fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0)) {
                    ret = _Z_ERR_GENERIC;
                }
            } else {
                ret = _Z_ERR_GENERIC;
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
    _ZP_UNUSED(lep);
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
        mreq.ipv6mr_interface = 1;  // FIXME: 0 is supposed to be the default interface,
                                    //        but it fails on the setsockopt.
                                    //        1 seems to be a working value on the WiFi interface
                                    //        which is the one available by default in ESP32
        setsockopt(sockrecv->_fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &mreq, sizeof(mreq));
    } else {
        // Do nothing. It must never not enter here.
        // Required to be compliant with MISRA 15.7 rule
    }

    close(sockrecv->_fd);
    close(socksend->_fd);
}

size_t _z_read_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_slice_t *addr) {
    struct sockaddr_storage raddr;
    unsigned int raddrlen = sizeof(struct sockaddr_storage);

    ssize_t rb = 0;
    do {
        rb = recvfrom(sock._fd, ptr, len, 0, (struct sockaddr *)&raddr, &raddrlen);
        if (rb < (ssize_t)0) {
            rb = SIZE_MAX;
            break;
        }

        if (lep._iptcp->ai_family == AF_INET) {
            struct sockaddr_in *a = ((struct sockaddr_in *)lep._iptcp->ai_addr);
            struct sockaddr_in *b = ((struct sockaddr_in *)&raddr);
            if (!((a->sin_port == b->sin_port) && (a->sin_addr.s_addr == b->sin_addr.s_addr))) {
                // If addr is not NULL, it means that the raddr was requested by the upper-layers
                if (addr != NULL) {
                    *addr = _z_slice_make(sizeof(in_addr_t) + sizeof(in_port_t));
                    (void)memcpy((uint8_t *)addr->start, &b->sin_addr.s_addr, sizeof(in_addr_t));
                    (void)memcpy((uint8_t *)(addr->start + sizeof(in_addr_t)), &b->sin_port, sizeof(in_port_t));
                }
                break;
            }
        } else if (lep._iptcp->ai_family == AF_INET6) {
            struct sockaddr_in6 *a = ((struct sockaddr_in6 *)lep._iptcp->ai_addr);
            struct sockaddr_in6 *b = ((struct sockaddr_in6 *)&raddr);
            if ((a->sin6_port != b->sin6_port) ||
                (memcmp(&a->sin6_addr, &b->sin6_addr, sizeof(struct in6_addr)) != 0)) {
                // If addr is not NULL, it means that the raddr was requested by the upper-layers
                if (addr != NULL) {
                    *addr = _z_slice_make(sizeof(struct in6_addr) + sizeof(in_port_t));
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
    return sendto(sock._fd, ptr, len, 0, rep._iptcp->ai_addr, rep._iptcp->ai_addrlen);
}
#endif

#if Z_FEATURE_LINK_BLUETOOTH == 1
/*------------------ Bluetooth sockets ------------------*/
int8_t _z_open_bt(_z_sys_net_socket_t *sock, const char *gname, uint8_t mode, uint8_t profile, uint32_t tout) {
    int8_t ret = _Z_RES_OK;

    if (profile == _Z_BT_PROFILE_SPP) {
        sock->_bts = new BluetoothSerial();
        if (mode == _Z_BT_MODE_SLAVE) {
            sock->_bts->begin(gname, false);
        } else if (mode == _Z_BT_MODE_MASTER) {
            sock->_bts->begin(gname, true);
            uint8_t connected = sock->_bts->connect(gname);
            if (!connected) {
                while (!sock->_bts->connected(tout)) {
                    ZP_ASM_NOP;
                }
            }
        } else {
            delete sock->_bts;
            ret = _Z_ERR_GENERIC;
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

int8_t _z_listen_bt(_z_sys_net_socket_t *sock, const char *gname, uint8_t mode, uint8_t profile, uint32_t tout) {
    int8_t ret = _Z_RES_OK;

    if (profile == _Z_BT_PROFILE_SPP) {
        sock->_bts = new BluetoothSerial();
        if (mode == _Z_BT_MODE_SLAVE) {
            sock->_bts->begin(gname, false);
        } else if (mode == _Z_BT_MODE_MASTER) {
            sock->_bts->begin(gname, true);
            uint8_t connected = sock->_bts->connect(gname);
            if (!connected) {
                while (!sock->_bts->connected(tout)) {
                    ZP_ASM_NOP;
                }
            }
        } else {
            delete sock->_bts;
            ret = _Z_ERR_GENERIC;
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_close_bt(_z_sys_net_socket_t *sock) {
    sock->_bts->end();
    delete sock->_bts;
}

size_t _z_read_bt(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t i = 0;
    for (i = 0; i < len; i++) {
        int c = sock._bts->read();
        if (c == -1) {
            delay(1);  // FIXME: without this, the read task is blocking the other tasks
            break;
        }
        ptr[i] = c;
    }

    return i;
}

size_t _z_read_exact_bt(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_bt(sock, ptr, len - n);
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_bt(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    sock._bts->write(ptr, len);
    return len;
}
#endif

#if Z_FEATURE_LINK_SERIAL == 1
/*------------------ Serial sockets ------------------*/
int8_t _z_open_serial_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    int8_t ret = _Z_RES_OK;

    uint8_t uart = 255;
    if (rxpin == 3 && txpin == 1) {
        uart = 0;
    } else if (rxpin == 9 && txpin == 10) {
        uart = 1;
    } else if (rxpin == 16 && txpin == 17) {
        uart = 2;
    } else {
        ret = _Z_ERR_GENERIC;
    }

    if (ret == _Z_RES_OK) {
        // WARNING: Glitch on the very 1st byte is common when the serial input line is allowed to float.
        //          To minimize this issue the RxPin is set to INPUT_PULLUP, and set the TxPin is driven to HIGH. This
        //          will keep the pins high and upon initialization of the serial port using serial.begin() the line
        //          does not float (drops to low).
        pinMode(rxpin, INPUT_PULLUP);
        pinMode(txpin, OUTPUT);
        digitalWrite(txpin, HIGH);

        sock->_serial = new HardwareSerial(uart);
        if (sock->_serial != NULL) {
            sock->_serial->begin(baudrate);
            sock->_serial->flush();
        } else {
            ret = _Z_ERR_GENERIC;
        }
    }

    return ret;
}

int8_t _z_open_serial_from_dev(_z_sys_net_socket_t *sock, char *dev, uint32_t baudrate) {
    int8_t ret = _Z_RES_OK;

    uint8_t uart = 255;
    uint32_t rxpin = 0;
    uint32_t txpin = 0;
    if (strcmp(dev, "UART_0") == 0) {
        uart = 0;
        rxpin = 3;
        txpin = 1;
    } else if (strcmp(dev, "UART_1") == 0) {
        uart = 1;
        rxpin = 9;
        txpin = 10;
    } else if (strcmp(dev, "UART_2") == 0) {
        uart = 2;
        rxpin = 16;
        txpin = 17;
    } else {
        ret = _Z_ERR_GENERIC;
    }

    if (ret == _Z_RES_OK) {
        // WARNING: Glitch on the very 1st byte is common when the serial input line is allowed to float.
        //          To minimize this issue the RxPin is set to INPUT_PULLUP, and set the TxPin is driven to HIGH. This
        //          will keep the pins high and upon initialization of the serial port using serial.begin() the line
        //          does not float (drops to low).
        pinMode(rxpin, INPUT_PULLUP);
        pinMode(txpin, OUTPUT);
        digitalWrite(txpin, HIGH);

        sock->_serial = new HardwareSerial(uart);
        if (sock->_serial != NULL) {
            sock->_serial->begin(baudrate);
            sock->_serial->flush();
        } else {
            ret = _Z_ERR_GENERIC;
        }
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
    (void)(sock);
    (void)(dev);
    (void)(baudrate);

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

void _z_close_serial(_z_sys_net_socket_t *sock) {
    sock->_serial->end();
    delete sock->_serial;
}

size_t _z_read_serial(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    int8_t ret = _Z_RES_OK;

    uint8_t *before_cobs = new uint8_t[_Z_SERIAL_MAX_COBS_BUF_SIZE]();
    size_t rb = 0;
    for (size_t i = 0; i < _Z_SERIAL_MAX_COBS_BUF_SIZE; i++) {
        while (sock._serial->available() < 1) {
            z_sleep_ms(1);  // FIXME: Yield by sleeping.
        }
        before_cobs[i] = sock._serial->read();
        rb = rb + (size_t)1;
        if (before_cobs[i] == (uint8_t)0x00) {
            break;
        }
    }

    uint8_t *after_cobs = new uint8_t[_Z_SERIAL_MFS_SIZE]();
    size_t trb = _z_cobs_decode(before_cobs, rb, after_cobs);

    size_t i = 0;
    uint16_t payload_len = 0;
    for (i = 0; i < sizeof(payload_len); i++) {
        payload_len |= (after_cobs[i] << ((uint8_t)i * (uint8_t)8));
    }

    if (trb == (size_t)(payload_len + (uint16_t)6)) {
        (void)memcpy(ptr, &after_cobs[i], payload_len);
        i = i + (size_t)payload_len;

        uint32_t crc = 0;
        for (uint8_t j = 0; j < sizeof(crc); j++) {
            crc |= (uint32_t)(after_cobs[i] << (j * (uint8_t)8));
            i = i + (size_t)1;
        }

        uint32_t c_crc = _z_crc32(ptr, payload_len);
        if (c_crc != crc) {
            ret = _Z_ERR_GENERIC;
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    delete before_cobs;
    delete after_cobs;

    rb = payload_len;
    if (ret != _Z_RES_OK) {
        rb = SIZE_MAX;
    }

    return rb;
}

size_t _z_read_exact_serial(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = len;
    size_t rb = 0;

    do {
        rb = _z_read_serial(sock, ptr, n);
        if (rb == SIZE_MAX) return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _z_send_serial(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    int8_t ret = _Z_RES_OK;

    uint8_t *before_cobs = (uint8_t *)z_malloc(_Z_SERIAL_MFS_SIZE);
    size_t i = 0;
    for (i = 0; i < sizeof(uint16_t); ++i) {
        before_cobs[i] = (len >> (i * (size_t)8)) & (size_t)0XFF;
    }

    (void)memcpy(&before_cobs[i], ptr, len);
    i = i + len;

    uint32_t crc = _z_crc32(ptr, len);
    for (uint8_t j = 0; j < sizeof(crc); j++) {
        before_cobs[i] = (crc >> (j * (uint8_t)8)) & (uint32_t)0XFF;
        i = i + (size_t)1;
    }

    uint8_t *after_cobs = (uint8_t *)z_malloc(_Z_SERIAL_MAX_COBS_BUF_SIZE);
    ssize_t twb = _z_cobs_encode(before_cobs, i, after_cobs);
    after_cobs[twb] = 0x00;  // Manually add the COBS delimiter
    ssize_t wb = sock._serial->write(after_cobs, twb + (ssize_t)1);
    if (wb != (twb + (ssize_t)1)) {
        ret = _Z_ERR_GENERIC;
    }

    z_free(before_cobs);
    z_free(after_cobs);

    size_t sb = len;
    if (ret != _Z_RES_OK) {
        sb = SIZE_MAX;
    }

    return sb;
}
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported yet on ESP32 port of Zenoh-Pico"
#endif

}  // extern "C"
