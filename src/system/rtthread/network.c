//
// Copyright (c) 2024 ZettaScale Technology
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/result.h"

#if Z_FEATURE_LINK_TCP == 1
z_result_t _z_socket_set_non_blocking(const _z_sys_net_socket_t *sock) {
    int flags = fcntl(sock->_socket, F_GETFL, 0);
    if (flags == -1) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    if (fcntl(sock->_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    return _Z_RES_OK;
}

z_result_t _z_socket_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    struct sockaddr naddr;
    socklen_t nlen = sizeof(naddr);
    int con_socket = accept(sock_in->_socket, &naddr, &nlen);
    if (con_socket < 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    
    // Set socket options
    struct timeval tv;
    tv.tv_sec = Z_CONFIG_SOCKET_TIMEOUT / 1000;
    tv.tv_usec = (Z_CONFIG_SOCKET_TIMEOUT % 1000) * 1000;
    if (setsockopt(con_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    int flags = 1;
    if (setsockopt(con_socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0) {
        close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

#if Z_FEATURE_TCP_NODELAY == 1
    if (setsockopt(con_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) < 0) {
        close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
#endif

    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if (setsockopt(con_socket, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0) {
        close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    sock_out->_socket = con_socket;
    return _Z_RES_OK;
}

void _z_socket_close(_z_sys_net_socket_t *sock) {
    shutdown(sock->_socket, SHUT_RDWR);
    close(sock->_socket);
}

#if Z_FEATURE_MULTI_THREAD == 1
z_result_t _z_socket_wait_event(void *v_peers, _z_mutex_rec_t *mutex) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    
    // Create select mask
    _z_transport_peer_unicast_slist_t **peers = (_z_transport_peer_unicast_slist_t **)v_peers;
    _z_mutex_rec_lock(mutex);
    _z_transport_peer_unicast_slist_t *curr = *peers;
    int max_fd = 0;
    while (curr != NULL) {
        _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(curr);
        FD_SET(peer->_socket._socket, &read_fds);
        if (peer->_socket._socket > max_fd) {
            max_fd = peer->_socket._socket;
        }
        curr = _z_transport_peer_unicast_slist_next(curr);
    }
    _z_mutex_rec_unlock(mutex);
    
    // Wait for events
    struct timeval timeout;
    timeout.tv_sec = Z_CONFIG_SOCKET_TIMEOUT / 1000;
    timeout.tv_usec = (Z_CONFIG_SOCKET_TIMEOUT % 1000) * 1000;
    if (select(max_fd + 1, &read_fds, NULL, NULL, &timeout) <= 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);  // Error or no data ready
    }
    
    // Mark sockets that are pending
    _z_mutex_rec_lock(mutex);
    curr = *peers;
    while (curr != NULL) {
        _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(curr);
        if (FD_ISSET(peer->_socket._socket, &read_fds)) {
            peer->_pending = true;
        }
        curr = _z_transport_peer_unicast_slist_next(curr);
    }
    _z_mutex_rec_unlock(mutex);
    return _Z_RES_OK;
}
#else
z_result_t _z_socket_wait_event(void *peers, _z_mutex_rec_t *mutex) {
    _ZP_UNUSED(peers);
    _ZP_UNUSED(mutex);
    return _Z_RES_OK;
}
#endif

/*------------------ TCP sockets ------------------*/
z_result_t _z_create_endpoint_tcp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    struct addrinfo hints;
    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(s_address, s_port, &hints, &ep->_iptcp) < 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
        return ret;
    }
    ep->_iptcp->ai_addr->sa_family = ep->_iptcp->ai_family;

    return ret;
}

void _z_free_endpoint_tcp(_z_sys_net_endpoint_t *ep) { 
    freeaddrinfo(ep->_iptcp); 
}

z_result_t _z_open_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_socket = socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol);
    if (sock->_socket != -1) {
        struct timeval tv;
        tv.tv_sec = tout / 1000;
        tv.tv_usec = (tout % 1000) * 1000;
        if ((ret == _Z_RES_OK) && (setsockopt(sock->_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        int flags = 1;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

#if Z_FEATURE_TCP_NODELAY == 1
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
#endif

        struct linger ling;
        ling.l_onoff = 1;
        ling.l_linger = Z_TRANSPORT_LEASE / 1000;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_socket, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        struct addrinfo *it = NULL;
        for (it = rep._iptcp; it != NULL; it = it->ai_next) {
            if ((ret == _Z_RES_OK) && (connect(sock->_socket, it->ai_addr, it->ai_addrlen) < 0)) {
                if (it->ai_next == NULL) {
                    _Z_ERROR_LOG(_Z_ERR_GENERIC);
                    ret = _Z_ERR_GENERIC;
                    break;
                }
            } else {
                break;
            }
        }

        if (ret != _Z_RES_OK) {
            close(sock->_socket);
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

z_result_t _z_listen_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep) {
    z_result_t ret = _Z_RES_OK;
    (void)sock;
    (void)lep;

    int yes = 1;
    sock->_socket = socket(lep._iptcp->ai_family, lep._iptcp->ai_socktype, lep._iptcp->ai_protocol);
    if (sock->_socket != -1) {
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_socket, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        if ((ret == _Z_RES_OK) && (bind(sock->_socket, lep._iptcp->ai_addr, lep._iptcp->ai_addrlen) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        if ((ret == _Z_RES_OK) && (listen(sock->_socket, SOMAXCONN) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        if (ret != _Z_RES_OK) {
            close(sock->_socket);
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

size_t _z_read_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    ssize_t rb = recv(sock._socket, ptr, len, 0);
    if (rb < 0) {
        rb = SIZE_MAX;
    }

    return rb;
}

size_t _z_send_tcp(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    return send(sock._socket, ptr, len, 0);
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
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

size_t _z_send_exact_tcp(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    size_t n = 0;
    const uint8_t *pos = &ptr[0];

    do {
        size_t sb = _z_send_tcp(sock, pos, len - n);
        if (sb == SIZE_MAX) {
            n = sb;
            break;
        }

        n = n + sb;
        pos = _z_ptr_u8_offset(pos, sb);
    } while (n != len);

    return n;
}
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
z_result_t _z_create_endpoint_udp(_z_sys_net_endpoint_udp_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    struct addrinfo hints;
    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo *result = NULL;
    if (getaddrinfo(s_address, s_port, &hints, &result) < 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
        return ret;
    }

    (void)memcpy(&ep->_sockaddr, result->ai_addr, result->ai_addrlen);
    ep->_socklen = result->ai_addrlen;

    freeaddrinfo(result);

    return ret;
}

void _z_free_endpoint_udp(_z_sys_net_endpoint_udp_t *ep) {
    _ZP_UNUSED(ep);
}
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1
z_result_t _z_open_udp_unicast(_z_sys_net_socket_udp_t *sock, const _z_sys_net_endpoint_udp_t rep,
                               uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_socket = socket(rep._sockaddr.ss_family, SOCK_DGRAM, 0);
    if (sock->_socket != -1) {
        z_time_t tv;
        tv.tv_sec = tout / (uint32_t)1000;
        tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
        if (setsockopt(sock->_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        if (ret == _Z_RES_OK) {
            (void)memcpy(&sock->_addr, &rep._sockaddr, sizeof(struct sockaddr_in));
        }

        if (ret != _Z_RES_OK) {
            close(sock->_socket);
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

z_result_t _z_listen_udp_unicast(_z_sys_net_socket_udp_t *sock, const _z_sys_net_endpoint_udp_t lep,
                                 uint32_t tout) {
    z_result_t ret = _Z_RES_OK;
    (void)sock;
    (void)lep;
    (void)tout;

    sock->_socket = socket(lep._sockaddr.ss_family, SOCK_DGRAM, 0);
    if (sock->_socket != -1) {
        z_time_t tv;
        tv.tv_sec = tout / (uint32_t)1000;
        tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
        if (setsockopt(sock->_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        if ((ret == _Z_RES_OK) && (bind(sock->_socket, (struct sockaddr *)&lep._sockaddr, lep._socklen) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        if (ret != _Z_RES_OK) {
            close(sock->_socket);
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_close_udp_unicast(_z_sys_net_socket_udp_t *sock) { 
    close(sock->_socket); 
}

size_t _z_read_udp_unicast(const _z_sys_net_socket_udp_t sock, uint8_t *ptr, size_t len) {
    struct sockaddr_storage raddr;
    socklen_t raddrlen = sizeof(struct sockaddr_storage);

    ssize_t rb = recvfrom(sock._socket, ptr, len, 0, (struct sockaddr *)&raddr, &raddrlen);
    if (rb < 0) {
        rb = SIZE_MAX;
    }

    return rb;
}

size_t _z_read_exact_udp_unicast(const _z_sys_net_socket_udp_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_udp_unicast(sock, pos, len - n);
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

size_t _z_send_udp_unicast(const _z_sys_net_socket_udp_t sock, const uint8_t *ptr, size_t len) {
    return sendto(sock._socket, ptr, len, 0, (struct sockaddr *)&sock._addr, sizeof(struct sockaddr_in));
}

size_t _z_send_exact_udp_unicast(const _z_sys_net_socket_udp_t sock, const uint8_t *ptr, size_t len) {
    size_t n = 0;
    const uint8_t *pos = &ptr[0];

    do {
        size_t sb = _z_send_udp_unicast(sock, pos, len - n);
        if (sb == SIZE_MAX) {
            n = sb;
            break;
        }

        n = n + sb;
        pos = _z_ptr_u8_offset(pos, sb);
    } while (n != len);

    return n;
}
#endif

#if Z_FEATURE_LINK_UDP_MULTICAST == 1
unsigned int __get_ip_from_iface(const char *iface, int sa_family, void *inaddr) {
    _ZP_UNUSED(iface);
    _ZP_UNUSED(sa_family);
    _ZP_UNUSED(inaddr);
    return 0;  // Not implemented for RT-Thread
}

z_result_t _z_open_udp_multicast(_z_sys_net_socket_udp_t *sock, const _z_sys_net_endpoint_udp_t rep,
                                 _z_sys_net_endpoint_udp_t *lep, unsigned int tout, const char *iface) {
    z_result_t ret = _Z_RES_OK;

    struct sockaddr *lsockaddr = NULL;
    socklen_t lsocklen = 0;

    if (lep == NULL) {
        switch (rep._sockaddr.ss_family) {
            case AF_INET: {
                lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in));
                if (lsockaddr != NULL) {
                    (void)memset(lsockaddr, 0, sizeof(struct sockaddr_in));
                    lsockaddr->sa_family = AF_INET;
                    ((struct sockaddr_in *)lsockaddr)->sin_port =
                        ((struct sockaddr_in *)&rep._sockaddr)->sin_port;
                    lsocklen = sizeof(struct sockaddr_in);
                }
                break;
            }
            case AF_INET6: {
                lsockaddr = (struct sockaddr *)z_malloc(sizeof(struct sockaddr_in6));
                if (lsockaddr != NULL) {
                    (void)memset(lsockaddr, 0, sizeof(struct sockaddr_in6));
                    lsockaddr->sa_family = AF_INET6;
                    ((struct sockaddr_in6 *)lsockaddr)->sin6_port =
                        ((struct sockaddr_in6 *)&rep._sockaddr)->sin6_port;
                    lsocklen = sizeof(struct sockaddr_in6);
                }
                break;
            }
            default:
                break;
        }
    } else {
        lsockaddr = (struct sockaddr *)&lep->_sockaddr;
        lsocklen = lep->_socklen;
    }

    sock->_socket = socket(rep._sockaddr.ss_family, SOCK_DGRAM, 0);
    if (sock->_socket != -1) {
        z_time_t tv;
        tv.tv_sec = tout / (uint32_t)1000;
        tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
        if ((ret == _Z_RES_OK) && (setsockopt(sock->_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        int optflag = 1;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_socket, SOL_SOCKET, SO_REUSEADDR, (void *)&optflag, sizeof(optflag)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        // Join multicast group
        if (ret == _Z_RES_OK) {
            switch (rep._sockaddr.ss_family) {
                case AF_INET: {
                    struct ip_mreq mreq;
                    (void)memset(&mreq, 0, sizeof(mreq));
                    mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)&rep._sockaddr)->sin_addr.s_addr;
                    if (iface != NULL) {
                        mreq.imr_interface.s_addr = htonl(__get_ip_from_iface(iface, AF_INET, &mreq.imr_interface.s_addr));
                    } else {
                        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
                    }

                    if ((ret == _Z_RES_OK) &&
                        (setsockopt(sock->_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, sizeof(mreq)) < 0)) {
                        _Z_ERROR_LOG(_Z_ERR_GENERIC);
                        ret = _Z_ERR_GENERIC;
                    }
                    break;
                }
                case AF_INET6: {
                    struct ipv6_mreq mreq;
                    (void)memset(&mreq, 0, sizeof(mreq));
                    (void)memcpy(&mreq.ipv6mr_multiaddr, &((struct sockaddr_in6 *)&rep._sockaddr)->sin6_addr,
                                 sizeof(struct in6_addr));
                    // mreq.ipv6mr_interface = ifindex;
                    mreq.ipv6mr_interface = 0;  // Use default interface

                    if ((ret == _Z_RES_OK) &&
                        (setsockopt(sock->_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP, (void *)&mreq, sizeof(mreq)) < 0)) {
                        _Z_ERROR_LOG(_Z_ERR_GENERIC);
                        ret = _Z_ERR_GENERIC;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        if ((ret == _Z_RES_OK) && (bind(sock->_socket, lsockaddr, lsocklen) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        if (ret == _Z_RES_OK) {
            (void)memcpy(&sock->_addr, &rep._sockaddr, sizeof(struct sockaddr_in));
        }

        if (ret != _Z_RES_OK) {
            close(sock->_socket);
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    if (lep == NULL) {
        z_free(lsockaddr);
    }

    return ret;
}

z_result_t _z_listen_udp_multicast(_z_sys_net_socket_udp_t *sock, const _z_sys_net_endpoint_udp_t rep,
                                   uint32_t tout, const char *iface) {
    z_result_t ret = _Z_RES_OK;
    (void)sock;
    (void)rep;
    (void)tout;
    (void)iface;

    // The listening socket is the same as the one used for transmission
    ret = _z_open_udp_multicast(sock, rep, NULL, tout, iface);

    return ret;
}

void _z_close_udp_multicast(_z_sys_net_socket_udp_t *sock) { 
    close(sock->_socket); 
}

size_t _z_read_udp_multicast(const _z_sys_net_socket_udp_t sock, uint8_t *ptr, size_t len,
                             const _z_sys_net_endpoint_udp_t lep, _z_bytes_t *addr) {
    struct sockaddr_storage raddr;
    socklen_t raddrlen = sizeof(struct sockaddr_storage);

    ssize_t rb = recvfrom(sock._socket, ptr, len, 0, (struct sockaddr *)&raddr, &raddrlen);
    if (rb < 0) {
        rb = SIZE_MAX;
    }

    if ((rb != SIZE_MAX) && (addr != NULL)) {
        *addr = _z_bytes_make(raddrlen);
        (void)memcpy((uint8_t *)addr->start, &raddr, raddrlen);
    }

    return rb;
}

size_t _z_read_exact_udp_multicast(const _z_sys_net_socket_udp_t sock, uint8_t *ptr, size_t len,
                                   const _z_sys_net_endpoint_udp_t lep, _z_bytes_t *addr) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_udp_multicast(sock, pos, len - n, lep, addr);
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

size_t _z_send_udp_multicast(const _z_sys_net_socket_udp_t sock, const uint8_t *ptr, size_t len,
                             const _z_sys_net_endpoint_udp_t rep) {
    return sendto(sock._socket, ptr, len, 0, (struct sockaddr *)&rep._sockaddr, rep._socklen);
}

size_t _z_send_exact_udp_multicast(const _z_sys_net_socket_udp_t sock, const uint8_t *ptr, size_t len,
                                   const _z_sys_net_endpoint_udp_t rep) {
    size_t n = 0;
    const uint8_t *pos = &ptr[0];

    do {
        size_t sb = _z_send_udp_multicast(sock, pos, len - n, rep);
        if (sb == SIZE_MAX) {
            n = sb;
            break;
        }

        n = n + sb;
        pos = _z_ptr_u8_offset(pos, sb);
    } while (n != len);

    return n;
}
#endif

#if Z_FEATURE_LINK_SERIAL == 1
z_result_t _z_open_serial_from_pins(_z_sys_net_socket_serial_t *sock, uint32_t txpin, uint32_t rxpin,
                                    uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(txpin);
    _ZP_UNUSED(rxpin);
    _ZP_UNUSED(baudrate);
    return _Z_ERR_GENERIC;  // Not implemented for RT-Thread
}

z_result_t _z_open_serial_from_dev(_z_sys_net_socket_serial_t *sock, char *device, uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(device);
    _ZP_UNUSED(baudrate);
    return _Z_ERR_GENERIC;  // Not implemented for RT-Thread
}

z_result_t _z_listen_serial_from_pins(_z_sys_net_socket_serial_t *sock, uint32_t txpin, uint32_t rxpin,
                                      uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(txpin);
    _ZP_UNUSED(rxpin);
    _ZP_UNUSED(baudrate);
    return _Z_ERR_GENERIC;  // Not implemented for RT-Thread
}

z_result_t _z_listen_serial_from_dev(_z_sys_net_socket_serial_t *sock, char *device, uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(device);
    _ZP_UNUSED(baudrate);
    return _Z_ERR_GENERIC;  // Not implemented for RT-Thread
}

void _z_close_serial(_z_sys_net_socket_serial_t *sock) {
    _ZP_UNUSED(sock);
}

size_t _z_read_serial(const _z_sys_net_socket_serial_t sock, uint8_t *ptr, size_t len) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(ptr);
    _ZP_UNUSED(len);
    return SIZE_MAX;
}

size_t _z_read_exact_serial(const _z_sys_net_socket_serial_t sock, uint8_t *ptr, size_t len) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(ptr);
    _ZP_UNUSED(len);
    return SIZE_MAX;
}

size_t _z_send_serial(const _z_sys_net_socket_serial_t sock, const uint8_t *ptr, size_t len) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(ptr);
    _ZP_UNUSED(len);
    return SIZE_MAX;
}

size_t _z_send_exact_serial(const _z_sys_net_socket_serial_t sock, const uint8_t *ptr, size_t len) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(ptr);
    _ZP_UNUSED(len);
    return SIZE_MAX;
}
#endif