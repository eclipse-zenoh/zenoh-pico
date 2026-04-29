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
#include <stdlib.h>

#include "zenoh-pico/link/transport/socket.h"
#if Z_FEATURE_LINK_TLS == 1
#include "zenoh-pico/link/transport/tls_stream.h"
#endif
#include "zenoh-pico/link/link.h"
#include "zenoh-pico/runtime/runtime.h"
#include "zenoh-pico/system/common/platform.h"
#include "zenoh-pico/transport/multicast/transport.h"
#include "zenoh-pico/transport/unicast/accept.h"
#include "zenoh-pico/transport/unicast/transport.h"
#include "zenoh-pico/utils/sleep.h"

static z_result_t _z_new_transport_client(_z_transport_t *zt, const _z_string_t *locator, const _z_id_t *local_zid,
                                          const _z_config_t *session_cfg) {
    z_result_t ret = _Z_RES_OK;
    // Init link
    _z_link_t *zl = (_z_link_t *)z_malloc(sizeof(_z_link_t));
    if (zl == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    memset(zl, 0, sizeof(_z_link_t));
    // Open link
    ret = _z_open_link(zl, locator, session_cfg);
    if (ret != _Z_RES_OK) {
        z_free(zl);
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
                ret = _z_transport_peer_unicast_add(&zt->_transport._unicast, &tp_param, *_z_link_get_socket(zl), false,
                                                    NULL);
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
#if Z_FEATURE_LINK_TCP != 1 && Z_FEATURE_LINK_TLS != 1
    _ZP_UNUSED(runtime);
#endif
    // Init link
    _z_link_t *zl = (_z_link_t *)z_malloc(sizeof(_z_link_t));
    if (zl == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    memset(zl, 0, sizeof(_z_link_t));
    // Listen link
    if (peer_op == _Z_PEER_OP_OPEN) {
        ret = _z_open_link(zl, locator, session_cfg);
    } else {
        ret = _z_listen_link(zl, locator, session_cfg);
    }
    if (ret != _Z_RES_OK) {
        z_free(zl);
        return ret;
    }
    switch (zl->_cap._transport) {
        case Z_LINK_CAP_TRANSPORT_UNICAST: {
#if Z_FEATURE_UNICAST_PEER == 1
            _z_transport_unicast_establish_param_t tp_param = {0};
            ret = _z_unicast_open_peer(&tp_param, zl, local_zid, peer_op, NULL);
            if (ret != _Z_RES_OK) {
                _z_link_free(&zl);
                return ret;
            }
            ret = _z_unicast_transport_create(zt, zl, &tp_param);
            _Z_SET_IF_OK(ret, _z_socket_set_blocking(_z_link_get_socket(zl), false));
            if (ret == _Z_RES_OK) {
                if (peer_op == _Z_PEER_OP_OPEN) {
                    ret = _z_transport_peer_unicast_add(&zt->_transport._unicast, &tp_param, *_z_link_get_socket(zl),
                                                        false, NULL);
                } else {
#if Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_TLS == 1
                    _z_fut_t f = _z_fut_null();
                    f._fut_arg = &zt->_transport._unicast;
                    f._fut_fn = _zp_unicast_accept_task_fn;
                    if (_z_fut_handle_is_null(_z_runtime_spawn(runtime, &f))) {
                        _Z_ERROR("Failed to spawn unicast accept task after transport creation.");
                        ret = _Z_ERR_FAILED_TO_SPAWN_TASK;
                    }
#else
                    _Z_ERROR_LOG(_Z_ERR_TRANSPORT_OPEN_FAILED);
                    ret = _Z_ERR_TRANSPORT_OPEN_FAILED;
#endif
                }
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
            _z_sys_net_socket_t socket = {0};
            ret = _z_open_socket(locator, session_cfg, &socket);
            if (ret == _Z_ERR_GENERIC) {
                ret = _Z_ERR_TRANSPORT_OPEN_FAILED;
            }
            _Z_RETURN_IF_ERR(ret);
            _z_transport_unicast_establish_param_t tp_param = {0};
            ret = _z_unicast_open_peer(&tp_param, zt->_transport._unicast._common._link, session_id, _Z_PEER_OP_OPEN,
                                       &socket);
            if (ret != _Z_RES_OK) {
#if Z_FEATURE_LINK_TLS == 1
                _z_close_tls_socket(&socket);
#endif
                _z_socket_close(&socket);
                return ret;
            }
            ret = _z_socket_set_blocking(&socket, false);
            if (ret != _Z_RES_OK) {
#if Z_FEATURE_LINK_TLS == 1
                _z_close_tls_socket(&socket);
#endif
                _z_socket_close(&socket);
                return ret;
            }
            ret = _z_transport_peer_unicast_add(&zt->_transport._unicast, &tp_param, socket, true, NULL);
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
            return true;

        default:
            return false;
    }
}

#if Z_FEATURE_UNICAST_PEER == 1
/*
 * Attempt to add all peers currently marked in pending_peers->_retry_mask.
 *
 * Each bit in pending_peers->_retry_mask corresponds to an index in
 * pending_peers->_locators.
 *
 * Behaviour:
 * - Each locator is attempted once per call.
 * - On success, the corresponding bit is cleared.
 * - On non-retryable error, the corresponding bit is cleared.
 * - On retryable error, the bit remains set for future attempts.
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
    size_t len = _z_string_svec_len(&pending_peers->_locators);

    if ((len == 0) || (pending_peers->_retry_mask == 0u)) {
        return _Z_RES_OK;
    }

    z_result_t last_non_retryable_ret = _Z_RES_OK;
    bool peer_added = false;

    for (size_t i = 0; i < len; i++) {
        uint64_t bit = UINT64_C(1) << i;
        if ((pending_peers->_retry_mask & bit) == 0u) {
            continue;
        }

        _z_string_t *locator = _z_string_svec_get(&pending_peers->_locators, i);
        z_result_t peer_ret = _z_new_peer(zt, session_id, locator, config);

        if (peer_ret == _Z_RES_OK) {
            pending_peers->_retry_mask &= ~bit;
            peer_added = true;
            _Z_DEBUG("Successfully added peer locator [%zu]: %.*s", i, (int)_z_string_len(locator),
                     _z_string_data(locator));
            continue;
        }

        _Z_DEBUG("Could not add peer locator [%zu]: %.*s (%d)", i, (int)_z_string_len(locator), _z_string_data(locator),
                 peer_ret);

        if (!_z_transport_open_error_is_retryable(peer_ret)) {
            // Non-retryable peer error: remove from the pending set
            pending_peers->_retry_mask &= ~bit;
            last_non_retryable_ret = peer_ret;

            _Z_WARN("Peer locator [%zu] (%.*s) removed from pending set due to non-retryable error", i,
                    (int)_z_string_len(locator), _z_string_data(locator));
        }
    }

    if (last_non_retryable_ret != _Z_RES_OK) {
        return last_non_retryable_ret;
    }
    if (pending_peers->_retry_mask != 0u) {
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
            assert(pending_peers->_retry_mask == 0u);
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

            if (pending_peers->_retry_mask == 0u) {
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
 * The task owns ztu->_pending_peers while its retry mask is non-zero. It clears
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

    if (pending_peers->_retry_mask == 0u) {
        return _z_add_peers_task_ready(pending_peers);
    }

    _z_session_rc_t session_rc = _z_session_weak_upgrade_if_open(&ztu->_common._session);
    if (_Z_RC_IS_NULL(&session_rc)) {
        return _z_add_peers_task_ready(pending_peers);
    }

    (void)_z_add_peers_impl(&_Z_RC_IN_VAL(&session_rc)->_tp, &_Z_RC_IN_VAL(&session_rc)->_local_zid, pending_peers,
                            &_Z_RC_IN_VAL(&session_rc)->_config);

    _z_session_rc_drop(&session_rc);

    if (pending_peers->_retry_mask == 0u) {
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
