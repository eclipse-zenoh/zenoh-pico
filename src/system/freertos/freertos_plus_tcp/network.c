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

#include "zenoh-pico/link/transport/socket.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

static uintptr_t _z_socket_id_impl(const _z_sys_net_socket_t *sock) { return (uintptr_t)sock->_socket; }

z_result_t _z_socket_set_blocking(const _z_sys_net_socket_t *sock, bool blocking) {
    TickType_t option_value = blocking ? pdMS_TO_TICKS(Z_CONFIG_SOCKET_TIMEOUT) : 0;
    BaseType_t result =
        FreeRTOS_setsockopt(sock->_socket, 0, FREERTOS_SO_RCVTIMEO, &option_value, sizeof(option_value));

    if (result != pdPASS) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    return _Z_RES_OK;
}

z_result_t _z_socket_set_non_blocking(const _z_sys_net_socket_t *sock) { return _z_socket_set_blocking(sock, false); }

void _z_socket_close(_z_sys_net_socket_t *sock) { FreeRTOS_closesocket(sock->_socket); }

static z_result_t _z_socket_wait_readable_impl(const _z_sys_net_socket_t *sockets, size_t count, uint8_t *ready,
                                               uint32_t timeout_ms) {
    z_result_t ret = _Z_RES_OK;
    size_t i = 0;
    SocketSet_t socketSet = FreeRTOS_CreateSocketSet();
    if (socketSet == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }

    for (i = 0; i < count; i++) {
        ready[i] = 0;
        FreeRTOS_FD_SET(sockets[i]._socket, socketSet, eSELECT_READ);
    }

    BaseType_t result = FreeRTOS_select(socketSet, pdMS_TO_TICKS(timeout_ms));
    if (result != 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    for (i = 0; i < count; i++) {
        if (FreeRTOS_FD_ISSET(sockets[i]._socket, socketSet)) {
            ready[i] = 1;
        }
    }

    FreeRTOS_DeleteSocketSet(socketSet);
    return ret;
}

uintptr_t _z_socket_id(const _z_sys_net_socket_t *sock) { return _z_socket_id_impl(sock); }

z_result_t _z_socket_wait_readable(const _z_sys_net_socket_t *sockets, size_t count, uint8_t *ready,
                                   uint32_t timeout_ms) {
    return _z_socket_wait_readable_impl(sockets, count, ready, timeout_ms);
}

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
