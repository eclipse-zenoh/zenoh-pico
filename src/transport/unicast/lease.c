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

#include "zenoh-pico/transport/unicast/lease.h"

#include "zenoh-pico/runtime/runtime.h"
#include "zenoh-pico/session/interest.h"
#include "zenoh-pico/session/liveliness.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/system/common/platform.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/unicast/connectivity.h"
#include "zenoh-pico/transport/unicast/transport.h"
#include "zenoh-pico/transport/unicast/tx.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_UNICAST_TRANSPORT == 1

z_result_t _zp_unicast_send_keep_alive(_z_transport_unicast_t *ztu) {
    z_result_t ret = _Z_RES_OK;

    _z_transport_message_t t_msg = _z_t_msg_make_keep_alive();
    ret = _z_transport_unicast_send_t_msg(ztu, &t_msg, NULL);

    return ret;
}

_z_fut_fn_result_t _zp_unicast_failed_result(_z_transport_unicast_t *ztu, _z_executor_t *executor) {
    _z_session_t *zs = _z_transport_common_get_session(&ztu->_common);
    _zp_unicast_report_disconnected_event(ztu, _z_transport_peer_unicast_hset_begin(&ztu->_peers));
    _z_unicast_transport_close(ztu, _Z_CLOSE_EXPIRED);
    _z_session_transport_mutex_lock(zs);
#if Z_FEATURE_AUTO_RECONNECT == 1
    // Store weak session to reuse for reconnection.
    _z_session_weak_t weak_session_clone = _z_session_weak_clone(&ztu->_common._session);
#endif
    _z_transport_clear(&zs->_tp);
    _z_session_transport_mutex_unlock(zs);

#if Z_FEATURE_AUTO_RECONNECT == 1
    ztu->_common._state = _Z_TRANSPORT_STATE_RECONNECTING;
    ztu->_common._session = weak_session_clone;
    _z_fut_t f = _z_fut_null();
    f._fut_arg = &ztu->_common;
    f._fut_fn = _z_client_reopen_task_fn;
    f._destroy_fn = _z_client_reopen_task_drop;
    if (_z_fut_handle_is_null(_z_executor_spawn(executor, &f))) {
        _Z_ERROR("Failed to spawn client reopen task after transport failure.");
        ztu->_common._state = _Z_TRANSPORT_STATE_CLOSED;
        _z_session_weak_drop(&ztu->_common._session);
        return _z_fut_fn_result_ready();
    } else {
        return _z_fut_fn_result_suspend();
    }
#else
    return _z_fut_fn_result_ready();
#endif
}

_z_fut_fn_result_t _zp_unicast_lease_task_fn(void *ztu_arg, _z_executor_t *executor) {
    _z_transport_unicast_t *ztu = (_z_transport_unicast_t *)ztu_arg;
    if (ztu->_common._state == _Z_TRANSPORT_STATE_CLOSED) {
        return _z_fut_fn_result_ready();
    } else if (ztu->_common._state == _Z_TRANSPORT_STATE_RECONNECTING) {
        return _z_fut_fn_result_suspend();
    }

    z_whatami_t mode = _z_transport_common_get_session(&ztu->_common)->_mode;

    if (mode == Z_WHATAMI_CLIENT) {
        _z_transport_peer_unicast_t *curr_peer = _z_transport_unicast_get_first_peer(ztu);
        assert(curr_peer != NULL);
        if (curr_peer->common._received) {
            // Reset the lease parameters
            curr_peer->common._received = false;
        } else {
            // THIS LOG STRING USED IN TEST, change with caution
            _Z_INFO("Closing session because it has expired after %zums", ztu->_common._lease);
            return _zp_unicast_failed_result(ztu, executor);
        }
    }
// TODO: Should we have a task per peer ?
#if Z_FEATURE_UNICAST_PEER == 1
    if (mode == Z_WHATAMI_PEER) {
        _z_peer_mask_bitset_t expired_peers = _z_peer_mask_bitset_new();
        for (_z_transport_peer_unicast_hset_iter_t iter = _z_transport_peer_unicast_hset_begin(&ztu->_peers);
             iter != _z_transport_peer_unicast_hset_end(&ztu->_peers);
             iter = _z_transport_peer_unicast_hset_iter_next(&ztu->_peers, iter)) {
            _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_hset_at(&ztu->_peers, iter);
            if (!peer->common._received) {
                _z_peer_mask_bitset_set_at(&expired_peers, (size_t)iter, true);
                _zp_unicast_report_disconnected_event(ztu, iter);
            }
            peer->common._received = false;
        }
        _zp_unicast_remove_peers_by_mask(ztu, expired_peers);
    }
#endif
    return _z_fut_fn_result_wake_up_after((unsigned long)ztu->_common._lease);
}

_z_fut_fn_result_t _zp_unicast_keep_alive_task_fn(void *ztu_arg, _z_executor_t *executor) {
    _z_transport_unicast_t *ztu = (_z_transport_unicast_t *)ztu_arg;
    if (ztu->_common._state == _Z_TRANSPORT_STATE_CLOSED) {
        return _z_fut_fn_result_ready();
    } else if (ztu->_common._state == _Z_TRANSPORT_STATE_RECONNECTING) {
        return _z_fut_fn_result_suspend();
    }

    z_whatami_t mode = _z_transport_common_get_session(&ztu->_common)->_mode;
    if (mode == Z_WHATAMI_CLIENT) {
        if (!ztu->_common._transmitted) {
            if (_zp_unicast_send_keep_alive(ztu) < 0) {
                // THIS LOG STRING USED IN TEST, change with caution
                _Z_INFO("Send keep alive failed.");
                return _zp_unicast_failed_result(ztu, executor);
            }
        }
        ztu->_common._transmitted = false;
    }
// TODO: Should we have a task per peer ?
#if Z_FEATURE_UNICAST_PEER == 1
    if (mode == Z_WHATAMI_PEER) {
        if (!ztu->_common._transmitted) {
            _Z_DEBUG("Sending keep alive");
            // Send keep alive to all peers
            _z_transport_message_t t_msg = _z_t_msg_make_keep_alive();
            if (!_z_transport_peer_unicast_hset_is_empty(&ztu->_peers)) {
                if (_z_transport_unicast_send_t_msg(ztu, &t_msg, NULL) != _Z_RES_OK) {
                    _Z_INFO("Send keep alive failed.");
                    // TODO: report failed peers and close them ?
                }
            }
        }
        ztu->_common._transmitted = false;
    }
#endif
    return _z_fut_fn_result_wake_up_after((unsigned long)ztu->_common._lease / Z_TRANSPORT_LEASE_EXPIRE_FACTOR);
}
#endif  // Z_FEATURE_UNICAST_TRANSPORT == 1
