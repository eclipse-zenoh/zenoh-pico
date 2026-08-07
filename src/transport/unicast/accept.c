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

#if Z_FEATURE_UNICAST_TRANSPORT == 1 && Z_FEATURE_UNICAST_PEER == 1
#if Z_FEATURE_CONNECTIVITY == 1
static void _zp_unicast_dispatch_connected_event(_z_transport_unicast_t *ztu, const _z_transport_peer_unicast_t *peer) {
    if (ztu == NULL || peer == NULL) {
        return;
    }

    _z_connectivity_peer_event_data_t connected_peer = {0};
    uint16_t mtu = 0;
    bool is_streamed = false;
    bool is_reliable = false;
    bool has_event_data = false;

    _z_transport_peer_mutex_lock(&ztu->_common);
    _z_transport_peer_unicast_slist_t *it = ztu->_peers;
    while (it != NULL) {
        _z_transport_peer_unicast_t *current_peer = _z_transport_peer_unicast_slist_value(it);
        if (current_peer == peer) {
            _z_transport_get_link_properties(&ztu->_common, &mtu, &is_streamed, &is_reliable);
            _z_connectivity_peer_event_data_copy_from_common(&connected_peer, &current_peer->common);
            has_event_data = true;
            break;
        }
        it = _z_transport_peer_unicast_slist_next(it);
    }
    _z_transport_peer_mutex_unlock(&ztu->_common);

    if (has_event_data) {
        _z_connectivity_peer_connected(_z_transport_common_get_session(&ztu->_common), &connected_peer, false, mtu,
                                       is_streamed, is_reliable);
        _z_connectivity_peer_event_data_clear(&connected_peer);
    }
}
#endif

_z_fut_fn_result_t _zp_unicast_accept_task_fn(void *ctx, _z_executor_t *executor) {
    _ZP_UNUSED(executor);
    _z_transport_unicast_t *ztu = (_z_transport_unicast_t *)ctx;

    _z_link_peer_t link_peer = _z_link_peer_null();
    z_result_t ret = _z_link_accept_peer(ztu->_common._link, &link_peer);
    if (ret != _Z_RES_OK) {
        if (ret == _Z_ERR_INVALID) {
            _Z_INFO("Accept link was closed");
            return _z_fut_fn_result_ready();
        }
        return _z_fut_fn_result_wake_up_after(1000);
    }

    if (_z_transport_peer_unicast_slist_len(ztu->_peers) >= Z_LISTEN_MAX_CONNECTION_NB) {
        _Z_INFO("Refusing connection as max connections currently reached");
        _z_link_peer_clear(&link_peer);
        return _z_fut_fn_result_wake_up_after(1000);
    }

    ret = _z_link_accept_peer_complete(ztu->_common._link, &link_peer);
    if (ret != _Z_RES_OK) {
        _Z_INFO("Connection accept completion failed with error %d", ret);
        _z_link_peer_clear(&link_peer);
        return _z_fut_fn_result_continue();
    }

    _z_transport_unicast_establish_param_t param = {0};
    ret = _z_unicast_handshake_listen(&param, ztu->_common._link,
                                      &_z_transport_common_get_session(&ztu->_common)->_local_zid, Z_WHATAMI_PEER,
                                      &link_peer);
    if (ret != _Z_RES_OK) {
        _Z_INFO("Connection accept handshake failed with error %d", ret);
        _z_link_peer_clear(&link_peer);
        return _z_fut_fn_result_continue();
    }

    ret = _z_link_peer_set_blocking(&link_peer, false);
    if (ret != _Z_RES_OK) {
        _Z_INFO("Failed to set peer non blocking with error %d", ret);
        _z_link_peer_clear(&link_peer);
        return _z_fut_fn_result_continue();
    }

    _z_transport_peer_unicast_t *new_peer = NULL;
    ret = _z_transport_peer_unicast_add(ztu, &param, &link_peer, &new_peer);
    if (ret != _Z_RES_OK) {
        return _z_fut_fn_result_continue();
    }

    if (new_peer != NULL) {
        (void)_z_interest_push_declarations_to_peer(_z_transport_common_get_session(&ztu->_common), (void *)new_peer);
#if Z_FEATURE_CONNECTIVITY == 1
        _zp_unicast_dispatch_connected_event(ztu, new_peer);
#endif
    }

    return _z_fut_fn_result_continue();
}
#endif
