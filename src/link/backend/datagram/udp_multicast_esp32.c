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
#include "zenoh-pico/link/backend/udp_multicast.h"

#if defined(ZP_PLATFORM_SOCKET_ESP32) && (Z_FEATURE_LINK_UDP_MULTICAST == 1)

#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static unsigned int _z_esp32_udp_multicast_ipv6_outbound_ifindex(void) {
    // TODO: route this through platform/socket support once esp32 multicast iface selection is explicit.
    return 0U;
}

static unsigned int _z_esp32_udp_multicast_ipv6_membership_ifindex(void) {
    // TODO: route this through platform/socket support once esp32 multicast iface selection is explicit.
    return 1U;
}

static z_result_t _z_esp32_udp_multicast_make_local_addr(int family, in_port_t port, struct sockaddr **lsockaddr,
                                                         socklen_t *addrlen) {
    z_result_t ret = _Z_RES_OK;

    *lsockaddr = NULL;
    *addrlen = 0;
    if (family == AF_INET) {
        *lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in));
        if (*lsockaddr != NULL) {
            (void)memset(*lsockaddr, 0, sizeof(struct sockaddr_in));
            *addrlen = sizeof(struct sockaddr_in);

            struct sockaddr_in *c_laddr = (struct sockaddr_in *)*lsockaddr;
            c_laddr->sin_family = AF_INET;
            c_laddr->sin_addr.s_addr = INADDR_ANY;
            c_laddr->sin_port = port;
        } else {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
    } else if (family == AF_INET6) {
        *lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in6));
        if (*lsockaddr != NULL) {
            (void)memset(*lsockaddr, 0, sizeof(struct sockaddr_in6));
            *addrlen = sizeof(struct sockaddr_in6);

            struct sockaddr_in6 *c_laddr = (struct sockaddr_in6 *)*lsockaddr;
            c_laddr->sin6_family = AF_INET6;
            c_laddr->sin6_addr = in6addr_any;
            c_laddr->sin6_port = port;
        } else {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static z_result_t _z_esp32_udp_multicast_configure_outbound_iface(int fd, const struct sockaddr *lsockaddr) {
    z_result_t ret = _Z_RES_OK;

    if (lsockaddr->sa_family == AF_INET) {
        if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &((const struct sockaddr_in *)lsockaddr)->sin_addr,
                       sizeof(struct in_addr)) < 0) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
    } else if (lsockaddr->sa_family == AF_INET6) {
        unsigned int ifindex = _z_esp32_udp_multicast_ipv6_outbound_ifindex();
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static z_result_t _z_esp32_udp_multicast_apply_membership(int fd, int family, int option, const void *addr_bytes) {
    z_result_t ret = _Z_RES_OK;

    if (family == AF_INET) {
        struct ip_mreq mreq;
        (void)memset(&mreq, 0, sizeof(mreq));
        // flawfinder: ignore
        (void)memcpy(&mreq.imr_multiaddr, addr_bytes, sizeof(struct in_addr));
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(fd, IPPROTO_IP, option, &mreq, sizeof(mreq)) < 0) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
    } else if (family == AF_INET6) {
        struct ipv6_mreq mreq;
        (void)memset(&mreq, 0, sizeof(mreq));
        // flawfinder: ignore
        (void)memcpy(&mreq.ipv6mr_multiaddr, addr_bytes, sizeof(struct in6_addr));
        mreq.ipv6mr_interface = _z_esp32_udp_multicast_ipv6_membership_ifindex();
        if (setsockopt(fd, IPPROTO_IPV6, option, &mreq, sizeof(mreq)) < 0) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static z_result_t _z_esp32_udp_multicast_apply_primary_membership(int fd, const _z_sys_net_endpoint_t rep, int option) {
    if (rep._iptcp->ai_family == AF_INET) {
        return _z_esp32_udp_multicast_apply_membership(fd, rep._iptcp->ai_family, option,
                                                       &((const struct sockaddr_in *)rep._iptcp->ai_addr)->sin_addr);
    }
    if (rep._iptcp->ai_family == AF_INET6) {
        return _z_esp32_udp_multicast_apply_membership(fd, rep._iptcp->ai_family, option,
                                                       &((const struct sockaddr_in6 *)rep._iptcp->ai_addr)->sin6_addr);
    }

    _Z_ERROR_LOG(_Z_ERR_GENERIC);
    return _Z_ERR_GENERIC;
}

z_result_t _z_open_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                                 uint32_t tout, const char *iface) {
    _ZP_UNUSED(iface);
    z_result_t ret = _Z_RES_OK;

    struct sockaddr *lsockaddr = NULL;
    socklen_t addrlen = 0;
    ret = _z_esp32_udp_multicast_make_local_addr(rep._iptcp->ai_family, 0, &lsockaddr, &addrlen);
    if ((ret == _Z_RES_OK) && (addrlen != 0U)) {
        sock->_fd = socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol);
        if (sock->_fd != -1) {
            z_time_t tv;
            tv.tv_sec = tout / (uint32_t)1000;
            tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
            if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if ((ret == _Z_RES_OK) && (bind(sock->_fd, lsockaddr, addrlen) < 0)) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if ((ret == _Z_RES_OK) && (getsockname(sock->_fd, lsockaddr, &addrlen) == -1)) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if (ret == _Z_RES_OK) {
                ret = _z_esp32_udp_multicast_configure_outbound_iface(sock->_fd, lsockaddr);
            }

            if (ret != _Z_RES_OK) {
                close(sock->_fd);
                sock->_fd = -1;
                z_free(lsockaddr);
                return ret;
            }

            struct addrinfo *laddr = (struct addrinfo *)z_malloc(sizeof(struct addrinfo));
            if (laddr == NULL) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                close(sock->_fd);
                sock->_fd = -1;
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
    } else if (ret == _Z_RES_OK) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

z_result_t _z_listen_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                                   const char *iface, const char *join) {
    _ZP_UNUSED(iface);
    (void)join;
    z_result_t ret = _Z_RES_OK;

    struct sockaddr *lsockaddr = NULL;
    socklen_t addrlen = 0;
    if (rep._iptcp->ai_family == AF_INET) {
        ret = _z_esp32_udp_multicast_make_local_addr(
            rep._iptcp->ai_family, ((struct sockaddr_in *)rep._iptcp->ai_addr)->sin_port, &lsockaddr, &addrlen);
    } else if (rep._iptcp->ai_family == AF_INET6) {
        ret = _z_esp32_udp_multicast_make_local_addr(
            rep._iptcp->ai_family, ((struct sockaddr_in6 *)rep._iptcp->ai_addr)->sin6_port, &lsockaddr, &addrlen);
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    if ((ret == _Z_RES_OK) && (addrlen != 0U)) {
        sock->_fd = socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol);
        if (sock->_fd != -1) {
            z_time_t tv;
            tv.tv_sec = tout / (uint32_t)1000;
            tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
            if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            int optflag = 1;
            if ((ret == _Z_RES_OK) &&
                (setsockopt(sock->_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optflag, sizeof(optflag)) < 0)) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if ((ret == _Z_RES_OK) && (bind(sock->_fd, lsockaddr, addrlen) < 0)) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
            }

            if (ret == _Z_RES_OK) {
                if (rep._iptcp->ai_family == AF_INET) {
                    ret = _z_esp32_udp_multicast_apply_primary_membership(sock->_fd, rep, IP_ADD_MEMBERSHIP);
                } else if (rep._iptcp->ai_family == AF_INET6) {
                    ret = _z_esp32_udp_multicast_apply_primary_membership(sock->_fd, rep, IPV6_JOIN_GROUP);
                } else {
                    _Z_ERROR_LOG(_Z_ERR_GENERIC);
                    ret = _Z_ERR_GENERIC;
                }
            }

            if (ret != _Z_RES_OK) {
                close(sock->_fd);
                sock->_fd = -1;
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
    if (sockrecv->_fd >= 0) {
        if (rep._iptcp->ai_family == AF_INET) {
            (void)_z_esp32_udp_multicast_apply_primary_membership(sockrecv->_fd, rep, IP_DROP_MEMBERSHIP);
        } else if (rep._iptcp->ai_family == AF_INET6) {
            (void)_z_esp32_udp_multicast_apply_primary_membership(sockrecv->_fd, rep, IPV6_LEAVE_GROUP);
        }
    }

    if (sockrecv->_fd >= 0) {
        close(sockrecv->_fd);
        sockrecv->_fd = -1;
    }
    if (socksend->_fd >= 0) {
        close(socksend->_fd);
        socksend->_fd = -1;
    }
}

size_t _z_read_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_slice_t *addr) {
    struct sockaddr_storage raddr;
    socklen_t raddrlen = sizeof(struct sockaddr_storage);

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
                if (addr != NULL) {
                    addr->len = sizeof(in_addr_t) + sizeof(in_port_t);
                    // flawfinder: ignore
                    (void)memcpy((uint8_t *)addr->start, &b->sin_addr.s_addr, sizeof(in_addr_t));
                    // flawfinder: ignore
                    (void)memcpy((uint8_t *)(addr->start + sizeof(in_addr_t)), &b->sin_port, sizeof(in_port_t));
                }
                break;
            }
        } else if (lep._iptcp->ai_family == AF_INET6) {
            struct sockaddr_in6 *a = ((struct sockaddr_in6 *)lep._iptcp->ai_addr);
            struct sockaddr_in6 *b = ((struct sockaddr_in6 *)&raddr);
            if ((a->sin6_port != b->sin6_port) ||
                (memcmp(&a->sin6_addr, &b->sin6_addr, sizeof(struct in6_addr)) != 0)) {
                if (addr != NULL) {
                    addr->len = sizeof(struct in6_addr) + sizeof(in_port_t);
                    // flawfinder: ignore
                    (void)memcpy((uint8_t *)addr->start, &b->sin6_addr.s6_addr, sizeof(struct in6_addr));
                    // flawfinder: ignore
                    (void)memcpy((uint8_t *)(addr->start + sizeof(struct in6_addr)), &b->sin6_port, sizeof(in_port_t));
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
    return sendto(sock._fd, ptr, len, 0, rep._iptcp->ai_addr, rep._iptcp->ai_addrlen);
}

const _z_udp_multicast_ops_t _z_udp_multicast_esp32_ops = {
    .endpoint_init_from_address = _z_udp_multicast_default_endpoint_init_from_address,
    .endpoint_clear = _z_udp_multicast_default_endpoint_clear,
    .open = _z_open_udp_multicast,
    .listen = _z_listen_udp_multicast,
    .close = _z_close_udp_multicast,
    .read_exact = _z_read_exact_udp_multicast,
    .read = _z_read_udp_multicast,
    .write = _z_send_udp_multicast,
};

#endif
