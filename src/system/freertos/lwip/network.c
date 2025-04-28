//
// Copyright (c) 2022 ZettaScale Technology
// Copyright (c) 2023 Fictionlab sp. z o.o.
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   João Mário Lago, <joaolago@bluerobotics.com>
//

#include <stdlib.h>

#include "FreeRTOS.h"
#include "lwip/api.h"
#include "lwip/dns.h"
#include "lwip/ip4_addr.h"
#include "lwip/netdb.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/sockets.h"
#include "lwip/udp.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/result.h"

#if Z_FEATURE_LINK_TCP == 1
z_result_t _z_socket_set_non_blocking(const _z_sys_net_socket_t *sock) {
    int flags = lwip_fcntl(sock->_socket, F_GETFL, 0);
    if (flags == -1) {
        return _Z_ERR_GENERIC;
    }
    if (lwip_fcntl(sock->_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        return _Z_ERR_GENERIC;
    }
    return _Z_RES_OK;
}

z_result_t _z_socket_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    struct sockaddr naddr;
    socklen_t nlen = sizeof(naddr);
    int con_socket = lwip_accept(sock_in->_socket, &naddr, &nlen);
    if (con_socket < 0) {
        return _Z_ERR_GENERIC;
    }
    // Set socket options
    z_time_t tv;
    tv.tv_sec = Z_CONFIG_SOCKET_TIMEOUT / (uint32_t)1000;
    tv.tv_usec = (Z_CONFIG_SOCKET_TIMEOUT % (uint32_t)1000) * (uint32_t)1000;
    if (setsockopt(con_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        return _Z_ERR_GENERIC;
    }

    int flags = 1;
    if (setsockopt(con_socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0) {
        return _Z_ERR_GENERIC;
    }
#if Z_FEATURE_TCP_NODELAY == 1
    if (setsockopt(con_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) < 0) {
        return _Z_ERR_GENERIC;
    }
#endif
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if (setsockopt(con_socket, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0) {
        return _Z_ERR_GENERIC;
    }
    // Note socket
    sock_out->_socket = con_socket;
    return _Z_RES_OK;
}

void _z_socket_close(_z_sys_net_socket_t *sock) {
    shutdown(sock->_socket, SHUT_RDWR);
    lwip_close(sock->_socket);
}

#if Z_FEATURE_MULTI_THREAD == 1
z_result_t _z_socket_wait_event(void *v_peers, _z_mutex_rec_t *mutex) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    // Create select mask
    _z_transport_unicast_peer_list_t **peers = (_z_transport_unicast_peer_list_t **)v_peers;
    _z_mutex_rec_lock(mutex);
    _z_transport_unicast_peer_list_t *curr = *peers;
    int max_fd = 0;
    while (curr != NULL) {
        _z_transport_unicast_peer_t *peer = _z_transport_unicast_peer_list_head(curr);
        FD_SET(peer->_socket._socket, &read_fds);
        if (peer->_socket._socket > max_fd) {
            max_fd = peer->_socket._socket;
        }
        curr = _z_transport_unicast_peer_list_tail(curr);
    }
    _z_mutex_rec_unlock(mutex);
    // Wait for events
    struct timeval timeout;
    timeout.tv_sec = Z_CONFIG_SOCKET_TIMEOUT / 1000;
    timeout.tv_usec = (Z_CONFIG_SOCKET_TIMEOUT % 1000) * 1000;
    if (lwip_select(max_fd + 1, &read_fds, NULL, NULL, &timeout) <= 0) {
        return _Z_ERR_GENERIC;  // Error or no data ready
    }
    // Mark sockets that are pending
    _z_mutex_rec_lock(mutex);
    curr = *peers;
    while (curr != NULL) {
        _z_transport_unicast_peer_t *peer = _z_transport_unicast_peer_list_head(curr);
        if (FD_ISSET(peer->_socket._socket, &read_fds)) {
            peer->_pending = true;
        }
        curr = _z_transport_unicast_peer_list_tail(curr);
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
        ret = _Z_ERR_GENERIC;
        return ret;
    }
    ep->_iptcp->ai_addr->sa_family = ep->_iptcp->ai_family;

    return ret;
}

void _z_free_endpoint_tcp(_z_sys_net_endpoint_t *ep) { freeaddrinfo(ep->_iptcp); }

z_result_t _z_open_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_socket = socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol);
    if (sock->_socket != -1) {
        z_time_t tv;
        tv.tv_sec = tout / (uint32_t)1000;
        tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
        if ((ret == _Z_RES_OK) && (setsockopt(sock->_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }

        int flags = 1;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }
#if Z_FEATURE_TCP_NODELAY == 1
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }
#endif
        struct linger ling;
        ling.l_onoff = 1;
        ling.l_linger = Z_TRANSPORT_LEASE / 1000;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_socket, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }

        struct addrinfo *it = NULL;
        for (it = rep._iptcp; it != NULL; it = it->ai_next) {
            if ((ret == _Z_RES_OK) && (connect(sock->_socket, it->ai_addr, it->ai_addrlen) < 0)) {
                if (it->ai_next == NULL) {
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
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

z_result_t _z_listen_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep) {
    z_result_t ret = _Z_RES_OK;

    sock->_socket = socket(lep._iptcp->ai_family, lep._iptcp->ai_socktype, lep._iptcp->ai_protocol);
    if (sock->_socket == -1) {
        return _Z_ERR_GENERIC;
    }
    int flags = 1;
    if ((ret == _Z_RES_OK) &&
        (setsockopt(sock->_socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0)) {
        ret = _Z_ERR_GENERIC;
    }
#if Z_FEATURE_TCP_NODELAY == 1
    if ((ret == _Z_RES_OK) &&
        (setsockopt(sock->_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) < 0)) {
        ret = _Z_ERR_GENERIC;
    }
#endif
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if ((ret == _Z_RES_OK) &&
        (setsockopt(sock->_socket, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)) {
        ret = _Z_ERR_GENERIC;
    }
    struct addrinfo *it = NULL;
    if (ret == _Z_RES_OK) {
        for (it = lep._iptcp; it != NULL; it = it->ai_next) {
            if (bind(sock->_socket, it->ai_addr, it->ai_addrlen) < 0) {
                ret = _Z_ERR_GENERIC;
                break;
            }
            if (listen(sock->_socket, Z_LISTEN_MAX_CONNECTION_NB) < 0) {
                ret = _Z_ERR_GENERIC;
                break;
            }
        }
    }
    if (ret != _Z_RES_OK) {
        close(sock->_socket);
    }
    return ret;
}

void _z_close_tcp(_z_sys_net_socket_t *sock) {
    shutdown(sock->_socket, SHUT_RDWR);
    close(sock->_socket);
}

size_t _z_read_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    ssize_t rb = recv(sock._socket, ptr, len, 0);
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
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_tcp(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    return send(sock._socket, ptr, len, 0);
}
#else
z_result_t _z_socket_set_non_blocking(const _z_sys_net_socket_t *sock) {
    _ZP_UNUSED(sock);
    _Z_ERROR("Function not yet supported on this system");
    return _Z_ERR_GENERIC;
}

z_result_t _z_socket_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    _ZP_UNUSED(sock_in);
    _ZP_UNUSED(sock_out);
    _Z_ERROR("Function not yet supported on this system");
    return _Z_ERR_GENERIC;
}

void _z_socket_close(_z_sys_net_socket_t *sock) { _ZP_UNUSED(sock); }

z_result_t _z_socket_wait_event(void *peers, _z_mutex_rec_t *mutex) {
    _ZP_UNUSED(peers);
    _ZP_UNUSED(mutex);
    _Z_ERROR("Function not yet supported on this system");
    return _Z_ERR_GENERIC;
}
#endif  // Z_FEATURE_LINK_TCP == 1

#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
z_result_t _z_create_endpoint_udp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    struct addrinfo hints;
    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(s_address, s_port, &hints, &ep->_iptcp) < 0) {
        ret = _Z_ERR_GENERIC;
        return ret;
    }
    ep->_iptcp->ai_addr->sa_family = ep->_iptcp->ai_family;

    return ret;
}

void _z_free_endpoint_udp(_z_sys_net_endpoint_t *ep) { freeaddrinfo(ep->_iptcp); }
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1
z_result_t _z_open_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_socket = socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol);
    if (sock->_socket != -1) {
        z_time_t tv;
        tv.tv_sec = tout / (uint32_t)1000;
        tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
        if ((ret == _Z_RES_OK) && (setsockopt(sock->_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            ret = _Z_ERR_GENERIC;
        }

        if (ret != _Z_RES_OK) {
            close(sock->_socket);
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

void _z_close_udp_unicast(_z_sys_net_socket_t *sock) { close(sock->_socket); }

size_t _z_read_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    struct sockaddr_storage raddr;
    long unsigned int addrlen = sizeof(struct sockaddr_storage);

    ssize_t rb = recvfrom(sock._socket, ptr, len, 0, (struct sockaddr *)&raddr, &addrlen);
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
        if ((rb == SIZE_MAX) || (rb == 0)) {
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
    return (size_t)sendto(sock._socket, ptr, len, 0, rep._iptcp->ai_addr, rep._iptcp->ai_addrlen);
}
#endif

#if Z_FEATURE_LINK_UDP_MULTICAST == 1
long unsigned int __get_ip_from_iface(const char *iface, int sa_family, struct sockaddr **lsockaddr) {
    unsigned int addrlen = 0U;

    struct netif *netif = netif_find(iface);
    if (netif != NULL && netif_is_up(netif)) {
        struct sockaddr_in *lsockaddr_in = z_malloc(sizeof(struct sockaddr_in));
        if (lsockaddr != NULL) {
            (void)memset(lsockaddr_in, 0, sizeof(struct sockaddr_in));
            const ip4_addr_t *ip4_addr = netif_ip4_addr(netif);
            inet_addr_from_ip4addr(&lsockaddr_in->sin_addr, ip_2_ip4(ip4_addr));
            lsockaddr_in->sin_family = AF_INET;
            lsockaddr_in->sin_port = htons(0);

            addrlen = sizeof(struct sockaddr_in);
            *lsockaddr = (struct sockaddr *)lsockaddr_in;
        }
    }

    return addrlen;
}

z_result_t _z_open_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                                 uint32_t tout, const char *iface) {
    z_result_t ret = _Z_RES_OK;

    struct sockaddr *lsockaddr = NULL;
    long unsigned int addrlen = __get_ip_from_iface(iface, rep._iptcp->ai_family, &lsockaddr);
    if (addrlen != 0U) {
        sock->_socket = socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol);
        if (sock->_socket != -1) {
            z_time_t tv;
            tv.tv_sec = tout / (uint32_t)1000;
            tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
            if ((ret == _Z_RES_OK) &&
                (setsockopt(sock->_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

            if ((ret == _Z_RES_OK) && (bind(sock->_socket, lsockaddr, addrlen) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

            // Get the randomly assigned port used to discard loopback messages
            if ((ret == _Z_RES_OK) && (getsockname(sock->_socket, lsockaddr, &addrlen) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

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
                close(sock->_socket);
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
        sock->_socket = socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol);
        if (sock->_socket != -1) {
            z_time_t tv;
            tv.tv_sec = tout / (uint32_t)1000;
            tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
            if ((ret == _Z_RES_OK) &&
                (setsockopt(sock->_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
                ret = _Z_ERR_GENERIC;
            }

            if (rep._iptcp->ai_family == AF_INET) {
                struct sockaddr_in address;
                (void)memset(&address, 0, sizeof(address));
                address.sin_family = (sa_family_t)rep._iptcp->ai_family;
                address.sin_port = ((struct sockaddr_in *)rep._iptcp->ai_addr)->sin_port;
                inet_pton(address.sin_family, "0.0.0.0", &address.sin_addr);
                if ((ret == _Z_RES_OK) && (bind(sock->_socket, (struct sockaddr *)&address, sizeof(address)) < 0)) {
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
                    (setsockopt(sock->_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)) {
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
                            (setsockopt(sock->_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)) {
                            ret = _Z_ERR_GENERIC;
                        }
                    }
                }
                z_free(joins);
            }

            if (ret != _Z_RES_OK) {
                close(sock->_socket);
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
        setsockopt(sockrecv->_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    } else {
        // Do nothing. It must never not enter here.
        // Required to be compliant with MISRA 15.7 rule
    }
    _ZP_UNUSED(lep);

    close(sockrecv->_socket);
    close(socksend->_socket);
}

size_t _z_read_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_slice_t *addr) {
    struct sockaddr_storage raddr;
    long unsigned int replen = sizeof(struct sockaddr_storage);

    ssize_t rb = 0;
    do {
        rb = recvfrom(sock._socket, ptr, len, 0, (struct sockaddr *)&raddr, &replen);
        if (rb < (ssize_t)0) {
            return SIZE_MAX;
        }

        if (lep._iptcp->ai_family == AF_INET) {
            struct sockaddr_in *a = ((struct sockaddr_in *)lep._iptcp->ai_addr);
            struct sockaddr_in *b = ((struct sockaddr_in *)&raddr);
            if (!((a->sin_port == b->sin_port) && (a->sin_addr.s_addr == b->sin_addr.s_addr))) {
                // If addr is not NULL, it means that the rep was requested by the upper-layers
                if (addr != NULL) {
                    *addr = _z_slice_make(sizeof(in_addr_t) + sizeof(in_port_t));
                    (void)memcpy((uint8_t *)addr->start, &b->sin_addr.s_addr, sizeof(in_addr_t));
                    (void)memcpy((uint8_t *)(addr->start + sizeof(in_addr_t)), &b->sin_port, sizeof(in_port_t));
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
        if ((rb == SIZE_MAX) || (rb == 0)) {
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
    return (size_t)sendto(sock._socket, ptr, len, 0, rep._iptcp->ai_addr, rep._iptcp->ai_addrlen);
}

#endif

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on FreeRTOS + LWIP port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_SERIAL == 1
#error "Serial not supported yet on FreeRTOS + LWIP port of Zenoh-Pico"
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported yet on FreeRTOS + LWIP port of Zenoh-Pico"
#endif
