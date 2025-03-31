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
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//   Błażej Sowa, <blazej@fictionlab.pl>

#include <stdlib.h>

#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/result.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

z_result_t _z_socket_set_non_blocking(const _z_sys_net_socket_t *sock) {
    TickType_t option_value = 0;  // Non-blocking mode
    BaseType_t result;

    result = FreeRTOS_setsockopt(sock->_socket, 0, FREERTOS_SO_RCVTIMEO, &option_value, sizeof(option_value));

    if (result != pdPASS) {
        return _Z_ERR_GENERIC;
    }
    return _Z_RES_OK;
}

z_result_t _z_socket_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    struct freertos_sockaddr naddr;
    socklen_t nlen = sizeof(naddr);
    int con_socket = FreeRTOS_accept(sock_in->_socket, &naddr, &nlen);
    if (con_socket < 0) {
        return _Z_ERR_GENERIC;
    }
    // Set socket options
    TickType_t receive_timeout = pdMS_TO_TICKS(Z_CONFIG_SOCKET_TIMEOUT);
    if (FreeRTOS_setsockopt(con_socket, 0, FREERTOS_SO_RCVTIMEO, &receive_timeout, 0) != 0) {
        return _Z_ERR_GENERIC;
    }
    // Note socket
    sock_out->_socket = con_socket;
    return _Z_RES_OK;
}

void _z_socket_close(_z_sys_net_socket_t *sock) { FreeRTOS_closesocket(sock->_socket); }

#if Z_FEATURE_MULTI_THREAD == 1
z_result_t _z_socket_wait_event(void *v_peers, _z_mutex_rec_t *mutex) {
    z_result_t ret = _Z_RES_OK;
    // Create a SocketSet to monitor multiple sockets
    SocketSet_t socketSet = FreeRTOS_CreateSocketSet();
    if (socketSet == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Add each socket to the socket set
    _z_transport_unicast_peer_list_t **peers = (_z_transport_unicast_peer_list_t **)v_peers;
    _z_mutex_rec_lock(mutex);
    _z_transport_unicast_peer_list_t *curr = *peers;
    while (curr != NULL) {
        _z_transport_unicast_peer_t *peer = _z_transport_unicast_peer_list_head(curr);
        FreeRTOS_FD_SET(peer->_socket._socket, socketSet, eSELECT_READ);
        curr = _z_transport_unicast_peer_list_tail(curr);
    }
    _z_mutex_rec_unlock(mutex);
    // Wait for an event on any of the sockets in the set, non-blocking
    BaseType_t result = FreeRTOS_select(socketSet, portMAX_DELAY);
    // Check for errors or events
    if (result != 0) {
        ret = _Z_ERR_GENERIC;
    }
    // Mark sockets that are pending
    _z_mutex_rec_lock(mutex);
    curr = *peers;
    while (curr != NULL) {
        _z_transport_unicast_peer_t *peer = _z_transport_unicast_peer_list_head(curr);
        if (FreeRTOS_FD_ISSET(peer->_socket._socket, socketSet)) {
            peer->_pending = true;
        }
        curr = _z_transport_unicast_peer_list_tail(curr);
    }
    _z_mutex_rec_unlock(mutex);
    // Clean up
    FreeRTOS_DeleteSocketSet(socketSet);
    return ret;
}
#else
z_result_t _z_socket_wait_event(void *peers, _z_mutex_rec_t *mutex) {
    _ZP_UNUSED(peers);
    _ZP_UNUSED(mutex);
    return _Z_RES_OK;
}
#endif

#if Z_FEATURE_LINK_TCP == 1
/*------------------ TCP sockets ------------------*/
z_result_t _z_create_endpoint_tcp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    if (FreeRTOS_getaddrinfo(s_address, NULL, NULL, &ep->_iptcp) < 0) {
        ret = _Z_ERR_GENERIC;
        return ret;
    }

    ep->_iptcp->ai_addr->sin_family = ep->_iptcp->ai_family;

    // Parse and check the validity of the port
    uint32_t port = strtoul(s_port, NULL, 10);
    if ((port > (uint32_t)0) && (port <= (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
        ep->_iptcp->ai_addr->sin_port = FreeRTOS_htons((uint16_t)port);
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_free_endpoint_tcp(_z_sys_net_endpoint_t *ep) { FreeRTOS_freeaddrinfo(ep->_iptcp); }

z_result_t _z_open_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_socket = FreeRTOS_socket(rep._iptcp->ai_family, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    if (sock->_socket != FREERTOS_INVALID_SOCKET) {
        TickType_t receive_timeout = pdMS_TO_TICKS(tout);

        if (FreeRTOS_setsockopt(sock->_socket, 0, FREERTOS_SO_RCVTIMEO, &receive_timeout, 0) != 0) {
            ret = _Z_ERR_GENERIC;
        } else if (FreeRTOS_connect(sock->_socket, rep._iptcp->ai_addr, rep._iptcp->ai_addrlen) < 0) {
            ret = _Z_ERR_GENERIC;
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

z_result_t _z_listen_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep) {
    z_result_t ret = _Z_RES_OK;
    sock->_socket = FreeRTOS_socket(lep._iptcp->ai_family, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    if (sock->_socket == FREERTOS_INVALID_SOCKET) {
        return _Z_ERR_GENERIC;
    }
    if (FreeRTOS_bind(sock->_socket, lep._iptcp->ai_addr, lep._iptcp->ai_addrlen) != 0) {
        FreeRTOS_closesocket(sock->_socket);
        return _Z_ERR_GENERIC;
    }
    if (FreeRTOS_listen(sock->_socket, Z_LISTEN_MAX_CONNECTION_NB) != 0) {
        FreeRTOS_closesocket(sock->_socket);
        return _Z_ERR_GENERIC;
    }
    return ret;
}

void _z_close_tcp(_z_sys_net_socket_t *sock) { FreeRTOS_closesocket(sock->_socket); }

size_t _z_read_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    BaseType_t rb = FreeRTOS_recv(sock._socket, ptr, len, 0);
    if (rb < 0) {
        return SIZE_MAX;
    }

    return rb;
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
    return FreeRTOS_send(sock._socket, ptr, len, 0);
}
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
z_result_t _z_create_endpoint_udp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    if (FreeRTOS_getaddrinfo(s_address, NULL, NULL, &ep->_iptcp) < 0) {
        ret = _Z_ERR_GENERIC;
        return ret;
    }

    ep->_iptcp->ai_addr->sin_family = ep->_iptcp->ai_family;

    // Parse and check the validity of the port
    uint32_t port = strtoul(s_port, NULL, 10);
    if ((port > (uint32_t)0) && (port <= (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
        ep->_iptcp->ai_addr->sin_port = FreeRTOS_htons((uint16_t)port);
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_free_endpoint_udp(_z_sys_net_endpoint_t *ep) { FreeRTOS_freeaddrinfo(ep->_iptcp); }
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1
z_result_t _z_open_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_socket = FreeRTOS_socket(rep._iptcp->ai_family, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);
    if (sock->_socket != FREERTOS_INVALID_SOCKET) {
        TickType_t receive_timeout = pdMS_TO_TICKS(tout);

        if (FreeRTOS_setsockopt(sock->_socket, 0, FREERTOS_SO_RCVTIMEO, &receive_timeout, 0) != 0) {
            ret = _Z_ERR_GENERIC;
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

z_result_t _z_listen_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;
    (void)sock;
    (void)rep;
    (void)tout;

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

void _z_close_udp_unicast(_z_sys_net_socket_t *sock) { FreeRTOS_closesocket(sock->_socket); }

size_t _z_read_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    struct freertos_sockaddr raddr;
    uint32_t addrlen = sizeof(struct freertos_sockaddr);

    int32_t rb = FreeRTOS_recvfrom(sock._socket, ptr, len, 0, &raddr, &addrlen);
    if (rb < 0) {
        return SIZE_MAX;
    }

    return rb;
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
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_udp_unicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                           const _z_sys_net_endpoint_t rep) {
    return FreeRTOS_sendto(sock._socket, ptr, len, 0, rep._iptcp->ai_addr, sizeof(struct freertos_sockaddr));
}
#endif

#if Z_FEATURE_LINK_UDP_MULTICAST == 1
#error "UDP Multicast not supported yet on FreeRTOS-Plus-TCP port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on FreeRTOS-Plus-TCP port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_SERIAL == 1
#error "Serial not supported yet on FreeRTOS-Plus-TCP port of Zenoh-Pico"
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported yet on FreeRTOS-Plus-TCP port of Zenoh-Pico"
#endif
