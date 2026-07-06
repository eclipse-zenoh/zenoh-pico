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

#include "zenoh-pico/link/transport/socket.h"
#if Z_FEATURE_LINK_TLS == 1
#include "zenoh-pico/link/transport/tls_stream.h"
#endif
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/unicast/connectivity.h"
#include "zenoh-pico/transport/utils.h"

#if Z_FEATURE_CONNECTIVITY == 1
static z_result_t _z_transport_make_endpoint(const _z_string_t *protocol, const char *address, _z_string_t *out) {
    _z_locator_t locator;

    *out = _z_string_null();
    if (address == NULL || address[0] == '\0') {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    _z_locator_init(&locator);
    if (protocol != NULL && _z_string_check(protocol)) {
        locator._protocol = _z_string_alias(*protocol);
    } else {
        locator._protocol = _z_string_alias_str("tcp");
    }
    locator._address = _z_string_alias_str(address);

    *out = _z_locator_to_string(&locator);
    _z_locator_clear(&locator);
    if (!_z_string_check(out)) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    return _Z_RES_OK;
}
#endif

void _z_transport_peer_common_clear(_z_transport_peer_common_t *src) {
#if Z_FEATURE_CONNECTIVITY == 1
    _z_string_clear(&src->_link_src);
    _z_string_clear(&src->_link_dst);
#endif
#if Z_FEATURE_FRAGMENTATION == 1
    _z_wbuf_clear(&src->_dbuf_reliable);
    _z_wbuf_clear(&src->_dbuf_best_effort);
#endif
    src->_remote_zid = _z_id_empty();
}

#if Z_FEATURE_CONNECTIVITY == 1

void _z_connectivity_peer_event_data_alias_from_common(_z_connectivity_peer_event_data_t *dst,
                                                       const _z_transport_peer_common_t *src) {
    *dst = (_z_connectivity_peer_event_data_t){
        ._remote_zid = src->_remote_zid,
        ._remote_whatami = src->_remote_whatami,
        ._link_src = _z_string_view_from_string(&src->_link_src),
        ._link_dst = _z_string_view_from_string(&src->_link_dst),
    };
}
#endif

void _z_transport_peer_multicast_clear(_z_transport_peer_multicast_t *src) {
    _z_transport_peer_common_clear(&src->common);
}

void _z_transport_peer_unicast_clear(_z_transport_peer_unicast_t *src) {
    _z_zbuf_clear(&src->flow_buff);
    if (src->_owns_socket) {
#if Z_FEATURE_LINK_TLS == 1
        _z_close_tls_socket(&src->_socket);
#endif
        _z_socket_close(&src->_socket);
    }
    _z_transport_peer_common_clear(&src->common);
}

z_result_t _z_transport_peer_unicast_add(_z_transport_unicast_t *ztu, _z_transport_unicast_establish_param_t *param,
                                         _z_sys_net_socket_t socket, bool owns_socket, bool notify_connectivity) {
    // Fill peer data
    _z_transport_peer_unicast_t peer;
    peer.flow_state = _Z_FLOW_STATE_INACTIVE;
    peer.flow_curr_size = 0;
    peer.flow_buff = _z_zbuf_null();
    peer._pending = false;
    peer._socket = socket;
    peer._owns_socket = owns_socket;
    _z_zint_t initial_sn_rx = _z_sn_decrement(ztu->_common._sn_res, param->_initial_sn_rx);
    peer._sn_rx_reliable = initial_sn_rx;
    peer._sn_rx_best_effort = initial_sn_rx;

    peer.common._remote_zid = param->_remote_zid;
    peer.common._remote_whatami = param->_remote_whatami;
    peer.common._received = true;
#if Z_FEATURE_CONNECTIVITY == 1
    peer.common._link_src = _z_string_null();
    peer.common._link_dst = _z_string_null();
#endif
#if Z_FEATURE_FRAGMENTATION == 1
    peer.common._patch = param->_patch < _Z_CURRENT_PATCH ? param->_patch : _Z_CURRENT_PATCH;
    peer.common._state_reliable = _Z_DBUF_STATE_NULL;
    peer.common._state_best_effort = _Z_DBUF_STATE_NULL;
    peer.common._dbuf_reliable = _z_wbuf_null();
    peer.common._dbuf_best_effort = _z_wbuf_null();
#endif
    _z_transport_peer_mutex_lock(&ztu->_common);
    _z_transport_peer_unicast_hset_iter_t iter = _z_transport_peer_unicast_hset_insert(&ztu->_peers, &peer);
    if (iter == _z_transport_peer_unicast_hset_end(&ztu->_peers)) {
        _z_transport_peer_mutex_unlock(&ztu->_common);
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }

#if Z_FEATURE_CONNECTIVITY == 1
    _z_transport_peer_unicast_t *new_peer = _z_transport_peer_unicast_hset_at(&ztu->_peers, iter);
    if (ztu->_common._link != NULL) {
        char local_addr[160] = {0};
        char remote_addr[160] = {0};
        if (_z_socket_get_endpoints(&new_peer->_socket, local_addr, sizeof(local_addr), remote_addr,
                                    sizeof(remote_addr)) == _Z_RES_OK) {
            (void)_z_transport_make_endpoint(&ztu->_common._link->_endpoint._locator._protocol, local_addr,
                                             &new_peer->common._link_src);
            (void)_z_transport_make_endpoint(&ztu->_common._link->_endpoint._locator._protocol, remote_addr,
                                             &new_peer->common._link_dst);
        }
    }
#endif
    _z_transport_peer_mutex_unlock(&ztu->_common);

    if (notify_connectivity) {
        _zp_unicast_report_connected_event(ztu, iter);
    }

    return _Z_RES_OK;
}
