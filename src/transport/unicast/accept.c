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

#include "zenoh-pico/session/liveliness.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/system/common/platform.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/unicast/lease.h"
#include "zenoh-pico/transport/unicast/transport.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_MULTI_THREAD == 1 && Z_FEATURE_UNICAST_TRANSPORT == 1
static void *_zp_unicast_accept_task(void *ctx) {
    _z_transport_unicast_t *ztu = (_z_transport_unicast_t *)ctx;
    _z_sys_net_socket_t listen_socket = *_z_link_get_socket(&ztu->_common._link);
    _z_sys_net_socket_t con_socket = {0};

    while (true) {
        if (_z_transport_unicast_peer_list_len(ztu->_peers) < Z_LISTEN_MAX_CONNECTION_NB) {
            // Accept connection
            if (_z_socket_accept(&listen_socket, &con_socket) != _Z_RES_OK) {
                _Z_INFO("Connection accept failed");
                continue;
            }
            _z_transport_unicast_establish_param_t param = {0};
            // Start handshake
            z_result_t ret = _z_unicast_handshake_listen(
                &param, &ztu->_common._link, &_Z_RC_IN_VAL(ztu->_common._session)->_local_zid, Z_WHATAMI_PEER);
            if (ret != _Z_RES_OK) {
                _Z_INFO("Connection accept handshake failed with error %d", ret);
                _z_socket_close(&con_socket);
                continue;
            }
            // Set socket as non blocking (FIXME: activate when read tasks reworked)
            if (_z_socket_set_non_blocking(&con_socket) != _Z_RES_OK) {
                _Z_INFO("Failed to set socket non blocking");
                _z_socket_close(&con_socket);
                continue;
            }
            // Add peer
            _z_transport_unicast_peer_add(ztu, &param, con_socket);
        } else {
            // Wait for connections to drop
            z_sleep_s(1);
        }
    }
    return NULL;
}

z_result_t _zp_unicast_start_accept_task(_z_transport_unicast_t *ztu) {
    // Init memory
    _z_task_t task;
    // Init task
    if (_z_task_init(&task, NULL, _zp_unicast_accept_task, ztu) != _Z_RES_OK) {
        return _Z_ERR_SYSTEM_TASK_FAILED;
    }
    // Detach the thread
    _z_task_detach(&task);
    return _Z_RES_OK;
}

#else

z_result_t _zp_unicast_start_accept_task(_z_transport_unicast_t *ztu) {
    _ZP_UNUSED(ztu);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif
