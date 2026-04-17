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

z_result_t _z_socket_set_blocking(const _z_sys_net_socket_t *sock, bool blocking) {
    TickType_t option_value = blocking ? pdMS_TO_TICKS(Z_CONFIG_SOCKET_TIMEOUT) : 0;
    BaseType_t result =
        FreeRTOS_setsockopt(sock->_socket, 0, FREERTOS_SO_RCVTIMEO, &option_value, sizeof(option_value));

    if (result != pdPASS) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    return _Z_RES_OK;
}

void _z_socket_close(_z_sys_net_socket_t *sock) { FreeRTOS_closesocket(sock->_socket); }

z_result_t _z_socket_wait_readable(_z_socket_wait_iter_t *iter, uint32_t timeout_ms) {
    z_result_t ret = _Z_RES_OK;

    _z_socket_wait_iter_reset(iter);
    if (!_z_socket_wait_iter_next(iter)) {
        return _Z_RES_OK;
    }

    SocketSet_t socketSet = FreeRTOS_CreateSocketSet();
    if (socketSet == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }

    do {
        const _z_sys_net_socket_t *sock = _z_socket_wait_iter_get_socket(iter);
        _z_socket_wait_iter_set_ready(iter, false);
        FreeRTOS_FD_SET(sock->_socket, socketSet, eSELECT_READ);
    } while (_z_socket_wait_iter_next(iter));

    BaseType_t result = FreeRTOS_select(socketSet, pdMS_TO_TICKS(timeout_ms));
    if (result != 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    _z_socket_wait_iter_reset(iter);
    while (_z_socket_wait_iter_next(iter)) {
        const _z_sys_net_socket_t *sock = _z_socket_wait_iter_get_socket(iter);
        _z_socket_wait_iter_set_ready(iter, FreeRTOS_FD_ISSET(sock->_socket, socketSet) != 0);
    }

    FreeRTOS_DeleteSocketSet(socketSet);
    return ret;
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
