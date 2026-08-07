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

#include "zenoh-pico/transport/manager.h"

#include <stddef.h>

#include "zenoh-pico/link/link.h"
#include "zenoh-pico/runtime/runtime.h"
#include "zenoh-pico/session/interest.h"
#include "zenoh-pico/system/common/platform.h"
#include "zenoh-pico/transport/multicast/transport.h"
#include "zenoh-pico/transport/unicast/accept.h"
#include "zenoh-pico/transport/unicast/transport.h"
#include "zenoh-pico/utils/sleep.h"

#if Z_FEATURE_CONNECTIVITY == 1
static void _z_new_peer_dispatch_connected_event(_z_transport_unicast_t *ztu, const _z_transport_peer_unicast_t *peer) {
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

static z_result_t _z_new_transport_client(_z_transport_t *zt, const _z_string_t *locator, const _z_id_t *local_zid,
                                          const _z_config_t *session_cfg) {
    z_result_t ret = _Z_RES_OK;
    // Init link
    _z_link_t *zl = NULL;
    // Open link
    ret = _z_open_link(&zl, locator, session_cfg);
    if (ret != _Z_RES_OK) {
        return ret;
    }
    // Open transport
    switch (zl->_cap._transport) {
        // Unicast transport
        case Z_LINK_CAP_TRANSPORT_UNICAST: {
            _z_transport_unicast_establish_param_t tp_param;
            ret = _z_unicast_open_client(&tp_param, zl, local_zid);
            if (ret != _Z_RES_OK) {
                _z_link_free(&zl);
                return ret;
            }
            ret = _z_unicast_transport_create(zt, zl, &tp_param);
            // Fill peer list
            if (ret == _Z_RES_OK) {
                _z_link_peer_t link_peer = _z_link_peer_null();
                ret = _z_link_peer_from_link(zl, &link_peer);
                if (ret == _Z_RES_OK) {
                    ret = _z_transport_peer_unicast_add(&zt->_transport._unicast, &tp_param, &link_peer, NULL);
                }
            }
            break;
        }
        // Multicast transport
        case Z_LINK_CAP_TRANSPORT_RAWETH:
        case Z_LINK_CAP_TRANSPORT_MULTICAST: {
            _z_transport_multicast_establish_param_t tp_param = {0};
            ret = _z_multicast_open_client(&tp_param, zl, local_zid);
            if (ret != _Z_RES_OK) {
                _z_link_free(&zl);
                return ret;
            }
            ret = _z_multicast_transport_create(zt, zl, &tp_param);
            break;
        }
        default:
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            _z_link_free(&zl);
            ret = _Z_ERR_GENERIC;
            break;
    }
    return ret;
}

static z_result_t _z_new_transport_peer(_z_transport_t *zt, const _z_string_t *locator, const _z_id_t *local_zid,
                                        int peer_op, const _z_config_t *session_cfg, _z_runtime_t *runtime) {
    z_result_t ret = _Z_RES_OK;
#if Z_FEATURE_UNICAST_PEER != 1
    _ZP_UNUSED(runtime);
#endif
    // Init link
    _z_link_t *zl = NULL;
    // Listen link
    if (peer_op == _Z_PEER_OP_OPEN) {
        ret = _z_open_link(&zl, locator, session_cfg);
    } else {
        ret = _z_listen_link(&zl, locator, session_cfg);
    }
    if (ret != _Z_RES_OK) {
        return ret;
    }
    switch (zl->_cap._transport) {
        case Z_LINK_CAP_TRANSPORT_UNICAST: {
#if Z_FEATURE_UNICAST_PEER == 1
            if ((peer_op != _Z_PEER_OP_OPEN) && !_z_link_can_accept_peers(zl)) {
                _Z_ERROR_LOG(_Z_ERR_TRANSPORT_OPEN_FAILED);
                _z_link_free(&zl);
                return _Z_ERR_TRANSPORT_OPEN_FAILED;
            }
            _z_transport_unicast_establish_param_t tp_param = {0};
            ret = _z_unicast_open_peer(&tp_param, zl, local_zid, peer_op, NULL);
            if (ret != _Z_RES_OK) {
                _z_link_free(&zl);
                return ret;
            }
            ret = _z_unicast_transport_create(zt, zl, &tp_param);
            _z_link_peer_t link_peer = _z_link_peer_null();
            if (ret == _Z_RES_OK) {
                ret = _z_link_peer_from_link(zl, &link_peer);
                _Z_SET_IF_OK(ret, _z_link_peer_set_blocking(&link_peer, false));
            }
            if (ret == _Z_RES_OK) {
                if (peer_op == _Z_PEER_OP_OPEN) {
                    ret = _z_transport_peer_unicast_add(&zt->_transport._unicast, &tp_param, &link_peer, NULL);
                } else {
                    _z_link_peer_clear(&link_peer);
                    _z_fut_t f = _z_fut_null();
                    f._fut_arg = &zt->_transport._unicast;
                    f._fut_fn = _zp_unicast_accept_task_fn;
                    if (_z_fut_handle_is_null(_z_runtime_spawn(runtime, &f))) {
                        _Z_ERROR("Failed to spawn unicast accept task after transport creation.");
                        ret = _Z_ERR_FAILED_TO_SPAWN_TASK;
                    }
                }
            }
            if (ret != _Z_RES_OK) {
                _z_link_peer_clear(&link_peer);
            }
#else
            _ZP_UNUSED(runtime);
            _Z_ERROR_LOG(_Z_ERR_TRANSPORT_OPEN_FAILED);
            ret = _Z_ERR_TRANSPORT_OPEN_FAILED;
#endif
            break;
        }
        case Z_LINK_CAP_TRANSPORT_RAWETH:
        case Z_LINK_CAP_TRANSPORT_MULTICAST: {
            _z_transport_multicast_establish_param_t tp_param;
            ret = _z_multicast_open_peer(&tp_param, zl, local_zid);
            if (ret != _Z_RES_OK) {
                _z_link_free(&zl);
                return ret;
            }
            ret = _z_multicast_transport_create(zt, zl, &tp_param);
            break;
        }
        default:
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            _z_link_free(&zl);
            ret = _Z_ERR_GENERIC;
            break;
    }
    return ret;
}

z_result_t _z_new_transport(_z_transport_t *zt, const _z_id_t *bs, const _z_string_t *locator, z_whatami_t mode,
                            int peer_op, const _z_config_t *session_cfg, _z_runtime_t *runtime) {
    z_result_t ret;

    if (mode == Z_WHATAMI_CLIENT) {
        ret = _z_new_transport_client(zt, locator, bs, session_cfg);
    } else {
        ret = _z_new_transport_peer(zt, locator, bs, peer_op, session_cfg, runtime);
    }

    return ret;
}

z_result_t _z_new_peer(_z_transport_t *zt, const _z_id_t *session_id, const _z_string_t *locator,
                       const _z_config_t *session_cfg) {
    z_result_t ret = _Z_RES_OK;
    switch (zt->_type) {
        case _Z_TRANSPORT_UNICAST_TYPE: {
            _z_transport_peer_unicast_t *peer = NULL;
            ret = _z_transport_peer_unicast_open(&zt->_transport._unicast, session_id, locator, session_cfg, &peer);
            if (ret != _Z_RES_OK) {
                return ret;
            }
            if (peer != NULL) {
                (void)_z_interest_push_declarations_to_peer(
                    _z_transport_common_get_session(&zt->_transport._unicast._common), &peer->common);
            }
#if Z_FEATURE_CONNECTIVITY == 1
            if (peer != NULL) {
                _z_new_peer_dispatch_connected_event(&zt->_transport._unicast, peer);
            }
#endif
        } break;

        default:
            break;
    }
    return ret;
}

bool _z_transport_open_error_is_retryable(z_result_t ret) {
    switch (ret) {
        case _Z_ERR_TRANSPORT_OPEN_FAILED:
        case _Z_ERR_TRANSPORT_TX_FAILED:
        case _Z_ERR_TRANSPORT_RX_FAILED:
        case _Z_ERR_TRANSPORT_RX_DURATION_EXPIRED:
            return true;

        default:
            return false;
    }
}

#if Z_FEATURE_UNICAST_PEER == 1
/*
 * Attempt to add all peers currently marked as pending.
 *
 * Behaviour:
 * - Each locator is attempted once per call.
 * - On success, the locator is marked done.
 * - On non-retryable error, the locator is marked failed.
 * - On retryable error, the locator remains pending for future attempts.
 *
 * Return values:
 * - _Z_RES_OK:
 *     All pending peers were added, or there were no pending peers.
 *
 * - _Z_ERR_TRANSPORT_OPEN_PARTIAL_CONNECTIVITY:
 *     At least one peer was successfully added during this call, but
 *     retryable peers remain.
 *
 * - _Z_ERR_TRANSPORT_OPEN_FAILED:
 *     No peer was successfully added during this call, and retryable
 *     peers remain.
 *
 * - Any other error:
 *     A non-retryable error occurred; the returned value is the last
 *     such error encountered.
 */
static z_result_t _z_add_peers_impl(_z_transport_t *zt, const _z_id_t *session_id, _z_pending_peers_t *pending_peers,
                                    const _z_config_t *config) {
    if (!_z_pending_peers_has_pending(pending_peers)) {
        return _Z_RES_OK;
    }

    z_result_t last_non_retryable_ret = _Z_RES_OK;
    bool peer_added = false;

    size_t len = _z_pending_peer_svec_len(&pending_peers->_peers);
    for (size_t i = 0; i < len; i++) {
        _z_pending_peer_t *peer = _z_pending_peer_svec_get(&pending_peers->_peers, i);
        if (peer->_state != _Z_PENDING_PEER_STATE_PENDING) {
            continue;
        }

        _z_string_t *locator = &peer->_locator;
        z_result_t peer_ret = _z_new_peer(zt, session_id, locator, config);

        if (peer_ret == _Z_RES_OK) {
            peer->_state = _Z_PENDING_PEER_STATE_DONE;
            peer_added = true;
            _Z_DEBUG("Successfully added peer locator [%zu]: %.*s", i, (int)_z_string_len(locator),
                     _z_string_data(locator));
            continue;
        }

        _Z_DEBUG("Could not add peer locator [%zu]: %.*s (%d)", i, (int)_z_string_len(locator), _z_string_data(locator),
                 peer_ret);

        if (!_z_transport_open_error_is_retryable(peer_ret)) {
            peer->_state = _Z_PENDING_PEER_STATE_FAILED;
            last_non_retryable_ret = peer_ret;

            _Z_WARN("Peer locator [%zu] (%.*s) removed from pending set due to non-retryable error", i,
                    (int)_z_string_len(locator), _z_string_data(locator));
        }
    }

    if (last_non_retryable_ret != _Z_RES_OK) {
        return last_non_retryable_ret;
    }
    if (_z_pending_peers_has_pending(pending_peers)) {
        return peer_added ? _Z_ERR_TRANSPORT_OPEN_PARTIAL_CONNECTIVITY : _Z_ERR_TRANSPORT_OPEN_FAILED;
    }
    return _Z_RES_OK;
}

static _z_fut_fn_result_t _z_add_peers_task_ready(_z_pending_peers_t *pending_peers) {
    _z_pending_peers_clear(pending_peers);
    return _z_fut_fn_result_ready();
}

/*
 * Add peers for a transport according to the configured failure policy.
 *
 * This function operates only on unicast transports. For other transport
 * types it is a no-op and returns _Z_RES_OK.
 *
 * Behaviour:
 * - Performs at least one attempt to add pending peers.
 * - If all peers are added, returns _Z_RES_OK.
 *
 * Non-retryable errors:
 * - If exit_on_failure is true, returns immediately with the error.
 * - Otherwise, tolerates the error and continues if retryable peers remain.
 *
 * Retryable peers remaining:
 * - If timeout_ms == 0:
 *     - exit_on_failure == true:
 *         returns:
 *           _Z_ERR_TRANSPORT_OPEN_PARTIAL_CONNECTIVITY if any peer was added
 *           _Z_ERR_TRANSPORT_OPEN_FAILED otherwise
 *     - exit_on_failure == false:
 *         returns _Z_RES_OK
 *
 * - If timeout_ms != 0:
 *     - exit_on_failure == true:
 *         retries synchronously with backoff until:
 *           * all peers are added, or
 *           * timeout expires, or
 *           * a non-retryable error occurs
 *
 *     - exit_on_failure == false:
 *         returns _Z_RES_OK and leaves pending_peers populated for
 *         background retry handling.
 *
 * Timeout result (exit_on_failure == true):
 * - If at least one peer was added before timeout:
 *     returns _Z_ERR_TRANSPORT_OPEN_PARTIAL_CONNECTIVITY
 * - Otherwise:
 *     returns _Z_ERR_TRANSPORT_OPEN_FAILED
 */
z_result_t _z_add_peers(_z_transport_t *zt, const _z_id_t *session_id, _z_pending_peers_t *pending_peers,
                        const _z_config_t *session_cfg, bool exit_on_failure) {
    if (zt->_type != _Z_TRANSPORT_UNICAST_TYPE) {
        _z_pending_peers_clear(pending_peers);
        return _Z_RES_OK;
    }

    bool peer_added = false;

    while (true) {
        z_result_t ret = _z_add_peers_impl(zt, session_id, pending_peers, session_cfg);

        if (ret == _Z_RES_OK) {
            assert(!_z_pending_peers_has_pending(pending_peers));
            _z_pending_peers_clear(pending_peers);
            return _Z_RES_OK;
        }

        if (ret == _Z_ERR_TRANSPORT_OPEN_PARTIAL_CONNECTIVITY) {
            peer_added = true;
        } else if (ret != _Z_ERR_TRANSPORT_OPEN_FAILED) {
            // Non-retryable error
            if (exit_on_failure) {
                _z_pending_peers_clear(pending_peers);
                return ret;
            }

            if (!_z_pending_peers_has_pending(pending_peers)) {
                _z_pending_peers_clear(pending_peers);
                return _Z_RES_OK;
            }
        }

        // Retryable peers remain
        if (pending_peers->_timeout_ms == 0) {
            if (!exit_on_failure) {
                _z_pending_peers_clear(pending_peers);
                return _Z_RES_OK;
            }
            _z_pending_peers_clear(pending_peers);
            return peer_added ? _Z_ERR_TRANSPORT_OPEN_PARTIAL_CONNECTIVITY : _Z_ERR_TRANSPORT_OPEN_FAILED;
        }

        if (!exit_on_failure) {
            _z_pending_peers_move(&zt->_transport._unicast._pending_peers, pending_peers);
            return _Z_RES_OK;  // Let the background task handle retries until success or timeout
        }

        if (!_z_backoff_sleep(&pending_peers->_start, pending_peers->_timeout_ms, &pending_peers->_sleep_ms)) {
            _z_pending_peers_clear(pending_peers);
            return peer_added ? _Z_ERR_TRANSPORT_OPEN_PARTIAL_CONNECTIVITY : _Z_ERR_TRANSPORT_OPEN_FAILED;
        }
    }
}

/*
 * Continue peer addition after z_open() accepted partial connectivity.
 *
 * The task owns ztu->_pending_peers while pending locators remain. It clears
 * that state when the transport/session can no longer accept peers, when all
 * pending peers have been added, or when the configured timeout expires. During
 * transport reconnection it suspends so reconnection can re-establish the base
 * transport before peer addition resumes.
 */
_z_fut_fn_result_t _zp_add_peers_task_fn(void *ztu_arg, _z_executor_t *executor) {
    _ZP_UNUSED(executor);

    _z_transport_unicast_t *ztu = (_z_transport_unicast_t *)ztu_arg;
    _z_pending_peers_t *pending_peers = &ztu->_pending_peers;

    if (ztu->_common._state == _Z_TRANSPORT_STATE_CLOSED) {
        return _z_add_peers_task_ready(pending_peers);
    } else if (ztu->_common._state == _Z_TRANSPORT_STATE_RECONNECTING) {
        return _z_fut_fn_result_suspend();
    }

    if (!_z_pending_peers_has_pending(pending_peers)) {
        return _z_add_peers_task_ready(pending_peers);
    }

    _z_session_rc_t session_rc = _z_session_weak_upgrade_if_open(&ztu->_common._session);
    if (_Z_RC_IS_NULL(&session_rc)) {
        return _z_add_peers_task_ready(pending_peers);
    }

    (void)_z_add_peers_impl(&_Z_RC_IN_VAL(&session_rc)->_tp, &_Z_RC_IN_VAL(&session_rc)->_local_zid, pending_peers,
                            &_Z_RC_IN_VAL(&session_rc)->_config);

    _z_session_rc_drop(&session_rc);

    if (!_z_pending_peers_has_pending(pending_peers)) {
        return _z_add_peers_task_ready(pending_peers);
    }

    // Check timeout and reschedule if needed
    unsigned long delay_ms = pending_peers->_sleep_ms;

    if (pending_peers->_timeout_ms > 0) {
        unsigned long elapsed = z_clock_elapsed_ms(&pending_peers->_start);
        if (elapsed >= (unsigned long)pending_peers->_timeout_ms) {
            return _z_add_peers_task_ready(pending_peers);
        }

        unsigned long remaining_ms = (unsigned long)pending_peers->_timeout_ms - elapsed;
        if (delay_ms > remaining_ms) {
            delay_ms = remaining_ms;
        }
    }
    _z_backoff_advance(&pending_peers->_sleep_ms);
    return _z_fut_fn_result_wake_up_after(delay_ms);
}
#endif
