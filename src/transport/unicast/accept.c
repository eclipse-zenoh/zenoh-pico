//
// Copyright (c) 2025 ZettaScale Technology
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

#include "zenoh-pico/link/transport/socket.h"
#if Z_FEATURE_LINK_TLS == 1
#include "zenoh-pico/link/transport/tls_stream.h"
#endif
#include "zenoh-pico/runtime/runtime.h"
#include "zenoh-pico/session/interest.h"
#include "zenoh-pico/session/liveliness.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/system/common/platform.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/unicast/lease.h"
#include "zenoh-pico/transport/unicast/transport.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_UNICAST_TRANSPORT == 1 && Z_FEATURE_UNICAST_PEER == 1 && \
    (Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_TLS == 1)

_z_fut_fn_result_t _zp_unicast_accept_task_fn(void *ctx, _z_executor_t *executor) {
    _ZP_UNUSED(executor);
    _z_transport_unicast_t *ztu = (_z_transport_unicast_t *)ctx;
    const _z_sys_net_socket_t *socket_ptr = _z_link_get_socket(ztu->_common._link);
    if (socket_ptr == NULL) {
        _Z_ERROR_LOG(_Z_ERR_INVALID);
        return _z_fut_fn_result_ready();
    }

    _z_sys_net_socket_t listen_socket = *socket_ptr;
    _z_sys_net_socket_t con_socket = {0};
    z_result_t ret = _z_tcp_accept(&listen_socket, &con_socket);
    if (ret != _Z_RES_OK) {
        if (ret == _Z_ERR_INVALID) {
            _Z_INFO("Accept socket was closed");
            return _z_fut_fn_result_ready();
        }
        return _z_fut_fn_result_wake_up_after(1000);
    }

    if (_z_transport_peer_unicast_hset_size(&ztu->_peers) >= Z_MAX_NUM_PEERS) {
        _Z_INFO("Refusing connection as max connections currently reached");
#if Z_FEATURE_LINK_TLS == 1
        _z_close_tls_socket(&con_socket);
#endif
        _z_socket_close(&con_socket);
        return _z_fut_fn_result_wake_up_after(1000);
    }

    ret = _z_socket_set_blocking(&con_socket, true);
    if (ret != _Z_RES_OK) {
        _Z_INFO("Failed to set socket blocking with error %d", ret);
#if Z_FEATURE_LINK_TLS == 1
        _z_close_tls_socket(&con_socket);
#endif
        _z_socket_close(&con_socket);
        return _z_fut_fn_result_continue();
    }

#if Z_FEATURE_LINK_TLS == 1
    if (ztu->_common._link->_type == _Z_LINK_TYPE_TLS) {
        ret = _z_tls_accept(&con_socket, &listen_socket);
        if (ret != _Z_RES_OK) {
            _Z_INFO("TLS handshake failed with error %d", ret);
            _z_close_tls_socket(&con_socket);
            _z_socket_close(&con_socket);
            return _z_fut_fn_result_continue();
        }
    }
#endif

    _z_transport_unicast_establish_param_t param = {0};
    ret = _z_unicast_handshake_listen(&param, ztu->_common._link,
                                      &_z_transport_common_get_session(&ztu->_common)->_local_zid, Z_WHATAMI_PEER,
                                      &con_socket);
    if (ret != _Z_RES_OK) {
        _Z_INFO("Connection accept handshake failed with error %d", ret);
#if Z_FEATURE_LINK_TLS == 1
        _z_close_tls_socket(&con_socket);
#endif
        _z_socket_close(&con_socket);
        return _z_fut_fn_result_continue();
    }

    if (_z_socket_set_blocking(&con_socket, false) != _Z_RES_OK) {
        _Z_INFO("Failed to set socket non blocking");
#if Z_FEATURE_LINK_TLS == 1
        _z_close_tls_socket(&con_socket);
#endif
        _z_socket_close(&con_socket);
        return _z_fut_fn_result_continue();
    }

    ret = _z_transport_peer_unicast_add(ztu, &param, con_socket, true, true);
    if (ret != _Z_RES_OK) {
#if Z_FEATURE_LINK_TLS == 1
        _z_close_tls_socket(&con_socket);
#endif
        _z_socket_close(&con_socket);
        return _z_fut_fn_result_continue();
    }

    return _z_fut_fn_result_continue();
}
#endif
