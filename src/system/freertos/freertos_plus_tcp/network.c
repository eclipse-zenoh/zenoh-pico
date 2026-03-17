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

#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/utils/mutex.h"
#include "zenoh-pico/utils/result.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

z_result_t _z_socket_set_blocking(const _z_sys_net_socket_t *sock, bool blocking) {
    TickType_t option_value = blocking ? pdMS_TO_TICKS(Z_CONFIG_SOCKET_TIMEOUT) : 0;
    BaseType_t result;

    result = FreeRTOS_setsockopt(sock->_socket, 0, FREERTOS_SO_RCVTIMEO, &option_value, sizeof(option_value));
    if (result != pdPASS) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    return _Z_RES_OK;
}

z_result_t _z_socket_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    struct freertos_sockaddr naddr;
    socklen_t nlen = sizeof(naddr);
    Socket_t con_socket = FREERTOS_INVALID_SOCKET;

    sock_out->_socket = FREERTOS_INVALID_SOCKET;
    con_socket = FreeRTOS_accept(sock_in->_socket, &naddr, &nlen);
    if (con_socket == FREERTOS_INVALID_SOCKET) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    TickType_t receive_timeout = pdMS_TO_TICKS(Z_CONFIG_SOCKET_TIMEOUT);
    if (FreeRTOS_setsockopt(con_socket, 0, FREERTOS_SO_RCVTIMEO, &receive_timeout, 0) != 0) {
        FreeRTOS_closesocket(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    sock_out->_socket = con_socket;
    return _Z_RES_OK;
}

void _z_socket_close(_z_sys_net_socket_t *sock) { FreeRTOS_closesocket(sock->_socket); }

#if Z_FEATURE_MULTI_THREAD == 1
z_result_t _z_socket_wait_event(void *v_peers, _z_mutex_rec_t *mutex) {
    z_result_t ret = _Z_RES_OK;
    SocketSet_t socketSet = FreeRTOS_CreateSocketSet();
    if (socketSet == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }

    _z_transport_peer_unicast_slist_t **peers = (_z_transport_peer_unicast_slist_t **)v_peers;
    _z_mutex_rec_mt_lock(mutex);
    _z_transport_peer_unicast_slist_t *curr = *peers;
    while (curr != NULL) {
        _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(curr);
        FreeRTOS_FD_SET(peer->_socket._socket, socketSet, eSELECT_READ);
        curr = _z_transport_peer_unicast_slist_next(curr);
    }
    _z_mutex_rec_mt_unlock(mutex);

    BaseType_t result = FreeRTOS_select(socketSet, portMAX_DELAY);
    if (result != 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    _z_mutex_rec_mt_lock(mutex);
    curr = *peers;
    while (curr != NULL) {
        _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(curr);
        if (FreeRTOS_FD_ISSET(peer->_socket._socket, socketSet)) {
            peer->_pending = true;
        }
        curr = _z_transport_peer_unicast_slist_next(curr);
    }
    _z_mutex_rec_mt_unlock(mutex);

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
