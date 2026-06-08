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
//   Błażej Sowa, <blazej@fictionlab.pl>

#include "zenoh-pico/net/session.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/declarations.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/runtime/runtime.h"
#include "zenoh-pico/session/interest.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/common/lease.h"
#include "zenoh-pico/transport/common/read.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/transport/multicast.h"
#include "zenoh-pico/transport/multicast/lease.h"
#include "zenoh-pico/transport/multicast/read.h"
#include "zenoh-pico/transport/raweth/read.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/unicast.h"
#include "zenoh-pico/transport/unicast/lease.h"
#include "zenoh-pico/transport/unicast/read.h"
#include "zenoh-pico/utils/config.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"
#include "zenoh-pico/utils/sleep.h"
#include "zenoh-pico/utils/string.h"
#include "zenoh-pico/utils/uuid.h"

#if Z_FEATURE_SCOUTING == 1
static z_result_t _z_locators_by_scout(const _z_config_t *config, const _z_id_t *zid, _z_string_svec_t *locators) {
    z_result_t ret = _Z_RES_OK;

    char *opt_as_str = _z_config_get(config, Z_CONFIG_SCOUTING_WHAT_KEY);
    if (opt_as_str == NULL) {
        opt_as_str = (char *)Z_CONFIG_SCOUTING_WHAT_DEFAULT;
    }
    z_what_t what = strtol(opt_as_str, NULL, 10);

    opt_as_str = _z_config_get(config, Z_CONFIG_MULTICAST_LOCATOR_KEY);
    if (opt_as_str == NULL) {
        opt_as_str = (char *)Z_CONFIG_MULTICAST_LOCATOR_DEFAULT;
    }
    _z_string_t mcast_locator = _z_string_alias_str(opt_as_str);

    opt_as_str = _z_config_get(config, Z_CONFIG_SCOUTING_TIMEOUT_KEY);
    if (opt_as_str == NULL) {
        opt_as_str = (char *)Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT;
    }
    uint32_t timeout = (uint32_t)strtoul(opt_as_str, NULL, 10);

    // Scout and return upon the first result
    _z_hello_slist_t *hellos = _z_scout_inner(what, *zid, &mcast_locator, timeout, true);
    if (hellos != NULL) {
        _z_hello_t *hello = _z_hello_slist_value(hellos);
        _z_string_svec_copy(locators, &hello->_locators, true);
    }
    _z_hello_slist_free(&hellos);
    return ret;
}
#else
static z_result_t _z_locators_by_scout(const _z_config_t *config, const _z_id_t *zid, _z_string_svec_t *locators) {
    _ZP_UNUSED(config);
    _ZP_UNUSED(zid);
    _ZP_UNUSED(locators);
    _Z_ERROR("Cannot scout as Z_FEATURE_SCOUTING was deactivated");
    _Z_ERROR_RETURN(_Z_ERR_SCOUT_NO_RESULTS);
}
#endif

static z_result_t _z_locators_by_config(_z_config_t *config, _z_string_svec_t *listen_locators,
                                        _z_string_svec_t *connect_locators) {
    _Z_RETURN_IF_ERR(_z_config_get_all(config, listen_locators, Z_CONFIG_LISTEN_KEY));
    _Z_RETURN_IF_ERR(_z_config_get_all(config, connect_locators, Z_CONFIG_CONNECT_KEY));

    size_t listen_len = _z_string_svec_len(listen_locators);
    size_t connect_len = _z_string_svec_len(connect_locators);

    if ((listen_len == 0) && (connect_len == 0)) {
        return _Z_RES_OK;
    }

#if Z_FEATURE_UNICAST_PEER == 0
    if ((listen_len > 0) && (connect_len > 0)) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
#endif

    if (listen_len > 0) {
        _zp_config_insert(config, Z_CONFIG_MODE_KEY, Z_CONFIG_MODE_PEER);
    }

    return _Z_RES_OK;
}

static z_result_t _z_config_get_mode(const _z_config_t *config, z_whatami_t *mode) {
    z_result_t ret = _Z_RES_OK;
    char *s_mode = _z_config_get(config, Z_CONFIG_MODE_KEY);
    *mode = Z_WHATAMI_CLIENT;  // By default, zenoh-pico will operate as a client
    if (s_mode != NULL) {
        if (_z_str_eq(s_mode, Z_CONFIG_MODE_CLIENT) == true) {
            *mode = Z_WHATAMI_CLIENT;
        } else if (_z_str_eq(s_mode, Z_CONFIG_MODE_PEER) == true) {
            *mode = Z_WHATAMI_PEER;
        } else {
            _Z_ERROR("Trying to configure an invalid mode: %s", s_mode);
            _Z_ERROR_LOG(_Z_ERR_CONFIG_INVALID_MODE);
            ret = _Z_ERR_CONFIG_INVALID_MODE;
        }
    }
    return ret;
}

static z_result_t _z_open_inner(_z_session_rc_t *zs, _z_string_t *locator, const _z_id_t *zid, int peer_op,
                                const _z_config_t *config) {
    z_result_t ret = _Z_RES_OK;
    _z_session_t *zn = _Z_RC_IN_VAL(zs);

    ret = _z_new_transport(&zn->_tp, zid, locator, zn->_mode, peer_op, config, &_Z_RC_IN_VAL(zs)->_runtime);
    if (ret != _Z_RES_OK) {
        return ret;
    }
    _z_transport_get_common(&zn->_tp)->_session = _z_session_rc_clone_as_weak(zs);
    _z_transport_get_common(&zn->_tp)->_state = _Z_TRANSPORT_STATE_OPEN;
#if Z_FEATURE_MULTICAST_DECLARATIONS == 1
    if (zn->_tp._type == _Z_TRANSPORT_MULTICAST_TYPE) {
        ret = _z_interest_pull_resource_from_peers(zn);
    }
#endif
    return ret;
}

static int32_t _z_get_remaining_timeout_ms(z_clock_t *start, int32_t timeout_ms) {
    if (timeout_ms < 0) {
        return -1;
    }

    if (timeout_ms == 0) {
        return 0;
    }

    unsigned long elapsed = z_clock_elapsed_ms(start);
    if (elapsed >= (unsigned long)timeout_ms) {
        return 0;
    }

    return timeout_ms - (int32_t)elapsed;
}

typedef struct {
    bool transport_opened;
    int32_t remaining_timeout_ms;
} _z_open_connect_result_t;

/*
 * Attempt connect locators that are still marked pending.
 *
 * The pending_peers array is both input and output:
 * - PENDING locators may still be attempted;
 * - a locator that succeeds as the primary transport is marked DONE;
 * - a locator that fails with a non-retryable error is marked FAILED;
 * - retryable failures remain PENDING for another attempt or later peer addition.
 * - if exit_on_non_retryable_failure is true, the first non-retryable error is returned immediately;
 * - retryable errors are governed by timeout/backoff and do not fail fast through this flag.
 *
 * On success, out reports that the primary transport is open and how much of the
 * original timeout remains. In peer mode, remaining PENDING locators are used to
 * decide which connect locators still need to be added as peers.
 */
static z_result_t _z_open_connect_locator(_z_session_rc_t *zn, _z_pending_peers_t *pending_peers, const _z_id_t *zid,
                                          _z_config_t *config, int32_t timeout_ms, bool exit_on_non_retryable_failure,
                                          _z_open_connect_result_t *out) {
    size_t connect_len = _z_pending_peer_svec_len(&pending_peers->_peers);
    z_result_t last_retryable_ret = _Z_ERR_TRANSPORT_OPEN_FAILED;
    z_result_t last_non_retryable_ret = _Z_RES_OK;

    out->transport_opened = false;
    out->remaining_timeout_ms = timeout_ms;

    if (connect_len == 0) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    z_result_t ret = _Z_ERR_TRANSPORT_OPEN_FAILED;
    z_clock_t now = z_clock_now();
    uint32_t sleep_ms = _Z_SLEEP_BACKOFF_MIN_MS;

    while (!out->transport_opened) {
        _Z_DEBUG("Attempting to open %zu connect locator(s)", connect_len);

        for (size_t i = 0; i < connect_len; i++) {
            _z_pending_peer_t *peer = _z_pending_peer_svec_get(&pending_peers->_peers, i);
            if (peer->_state != _Z_PENDING_PEER_STATE_PENDING) {
                continue;
            }

            _z_string_t *locator = &peer->_locator;

            ret = _z_open_inner(zn, locator, zid, _Z_PEER_OP_OPEN, config);
            if (ret == _Z_RES_OK) {
                out->transport_opened = true;
                peer->_state = _Z_PENDING_PEER_STATE_DONE;
                _Z_DEBUG("Successfully opened connect locator [%zu]: %.*s", i, (int)_z_string_len(locator),
                         _z_string_data(locator));
                out->remaining_timeout_ms = _z_get_remaining_timeout_ms(&now, timeout_ms);
                return _Z_RES_OK;
            }

            _Z_DEBUG("Failed to open connect locator [%zu]: %.*s (ret=%d)", i, (int)_z_string_len(locator),
                     _z_string_data(locator), ret);

            if (_z_transport_open_error_is_retryable(ret)) {
                last_retryable_ret = ret;
                continue;
            }

            // Non-retryable error.
            last_non_retryable_ret = ret;
            peer->_state = _Z_PENDING_PEER_STATE_FAILED;

            if (exit_on_non_retryable_failure) {
                out->remaining_timeout_ms = _z_get_remaining_timeout_ms(&now, timeout_ms);
                return ret;
            }

            _Z_DEBUG("Removing connect locator [%zu] from pending set due to non-retryable error", i);
        }

        if (!_z_pending_peers_has_pending(pending_peers)) {
            break;
        }

        if (!_z_backoff_sleep(&now, timeout_ms, &sleep_ms)) {
            break;
        }

        _Z_DEBUG("Retrying connect locators");
    }

    out->remaining_timeout_ms = _z_get_remaining_timeout_ms(&now, timeout_ms);

    if (last_non_retryable_ret != _Z_RES_OK) {
        return last_non_retryable_ret;
    }

    return last_retryable_ret;
}

z_result_t _z_open_locators_client(_z_session_rc_t *zn, const _z_string_svec_t *connect_locators, const _z_id_t *zid,
                                   _z_config_t *config, int32_t timeout_ms) {
    size_t connect_len = _z_string_svec_len(connect_locators);

    if (connect_len == 0) {
        _Z_ERROR("No connect locators configured in client mode");
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    _z_pending_peers_t pending_peers = _z_pending_peers_null();
    _Z_RETURN_IF_ERR(_z_pending_peers_copy_from_locators(&pending_peers, connect_locators));
    _z_open_connect_result_t connect_result;
    z_result_t ret = _z_open_connect_locator(zn, &pending_peers, zid, config, timeout_ms, false, &connect_result);
    _z_pending_peers_clear(&pending_peers);
    return ret;
}

z_result_t _z_open_bind_listener(_z_session_rc_t *zn, _z_string_t *locator, const _z_id_t *zid, _z_config_t *config,
                                 int32_t timeout_ms) {
    z_result_t ret = _Z_ERR_TRANSPORT_OPEN_FAILED;
    z_clock_t now = z_clock_now();
    uint32_t sleep_ms = _Z_SLEEP_BACKOFF_MIN_MS;

    while (ret != _Z_RES_OK) {
        ret = _z_open_inner(zn, locator, zid, _Z_PEER_OP_LISTEN, config);
        if (ret == _Z_RES_OK) {
            return _Z_RES_OK;
        }

        if (!_z_transport_open_error_is_retryable(ret)) {
            break;
        }

        if (!_z_backoff_sleep(&now, timeout_ms, &sleep_ms)) {
            break;
        }
    }

    return ret;
}

z_result_t _z_open_locators_peer(_z_session_rc_t *zn, _z_string_t *listen_locator,
                                 const _z_string_svec_t *connect_locators, const _z_id_t *zid, _z_config_t *config,
                                 int32_t listen_timeout_ms, bool listen_exit_on_failure, int32_t connect_timeout_ms,
                                 bool connect_exit_on_failure) {
    size_t connect_len = _z_string_svec_len(connect_locators);

    bool transport_opened = false;

#if Z_FEATURE_UNICAST_PEER == 0
    if ((listen_locator != NULL && connect_len > 0) || (connect_len > 1)) {
        _Z_ERROR("Multiple connect locators, or combined listen and connect locators, require peer support");
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }
#endif

    // First, try to open the optional listen locator.
    if (listen_locator != NULL) {
        z_result_t ret = _z_open_bind_listener(zn, listen_locator, zid, config, listen_timeout_ms);
        if (ret == _Z_RES_OK) {
            transport_opened = true;
            _Z_DEBUG("Successfully opened listen locator: %.*s", (int)_z_string_len(listen_locator),
                     _z_string_data(listen_locator));
        } else {
            _Z_DEBUG("Failed to open listen locator: %.*s (ret=%d)", (int)_z_string_len(listen_locator),
                     _z_string_data(listen_locator), ret);

            if (listen_exit_on_failure) {
                return ret;
            }
        }
    }

    _z_pending_peers_t pending_peers = _z_pending_peers_null();
    if (connect_len > 0) {
        _Z_RETURN_IF_ERR(_z_pending_peers_copy_from_locators(&pending_peers, connect_locators));
    }

    int32_t remaining_timeout_ms = connect_timeout_ms;
    if (!transport_opened && (connect_len > 0)) {
        _z_open_connect_result_t connect_result;
        _Z_CLEAN_RETURN_IF_ERR(_z_open_connect_locator(zn, &pending_peers, zid, config, connect_timeout_ms,
                                                       connect_exit_on_failure, &connect_result),
                               _z_pending_peers_clear(&pending_peers));
        transport_opened = connect_result.transport_opened;
        remaining_timeout_ms = connect_result.remaining_timeout_ms;
    }

    if (!transport_opened) {
        _Z_ERROR("Failed to establish primary transport via listen or connect locators");
        _z_pending_peers_clear(&pending_peers);
        return _Z_ERR_TRANSPORT_OPEN_FAILED;
    }

#if Z_FEATURE_UNICAST_PEER == 1
    if (_z_pending_peers_has_pending(&pending_peers)) {
        pending_peers._timeout_ms = remaining_timeout_ms;
        pending_peers._start = z_clock_now();
        pending_peers._sleep_ms = _Z_SLEEP_BACKOFF_MIN_MS;

        // Ownership of pending_peers is transferred to _z_add_peers() if background retries are needed.
        z_result_t ret = _z_add_peers(&_Z_RC_IN_VAL(zn)->_tp, zid, &pending_peers, config, connect_exit_on_failure);
        if (connect_exit_on_failure) {
            if (ret == _Z_ERR_TRANSPORT_OPEN_FAILED) {
                _z_pending_peers_clear(&pending_peers);
                return _Z_ERR_TRANSPORT_OPEN_PARTIAL_CONNECTIVITY;
            }
            _z_pending_peers_clear(&pending_peers);
            return ret;
        }
    }
#endif
    _z_pending_peers_clear(&pending_peers);
    return _Z_RES_OK;
}

static inline z_result_t _z_validate_open_timeout(int32_t timeout_ms) {
    return (timeout_ms >= -1) ? _Z_RES_OK : _Z_ERR_CONFIG_INVALID_VALUE;
}

static inline const char *_z_open_connect_exit_on_failure_default(z_whatami_t mode) {
    return (mode == Z_WHATAMI_CLIENT) ? Z_CONFIG_CONNECT_EXIT_ON_FAILURE_CLIENT_DEFAULT
                                      : Z_CONFIG_CONNECT_EXIT_ON_FAILURE_PEER_DEFAULT;
}

z_result_t _z_open_locators(_z_session_rc_t *zn, const _z_string_svec_t *listen_locators,
                            const _z_string_svec_t *connect_locators, const _z_id_t *zid, _z_config_t *config,
                            z_whatami_t mode) {
    size_t listen_len = _z_string_svec_len(listen_locators);
    size_t connect_len = _z_string_svec_len(connect_locators);

    if ((listen_len == 0) && (connect_len == 0)) {
        _Z_ERROR("No listen or connect locators configured");
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    if (listen_len > 1) {
        _Z_ERROR("Multiple listen locators are not supported in zenoh-pico");
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    if ((mode == Z_WHATAMI_CLIENT) && (listen_len > 0)) {
        _Z_ERROR("Listen locators are not supported in client mode");
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    int32_t listen_timeout_ms;
    bool listen_exit_on_failure;
    int32_t connect_timeout_ms;
    bool connect_exit_on_failure;

#if defined(Z_FEATURE_UNSTABLE_API)
    _Z_RETURN_IF_ERR(_z_config_get_i32_default(config, Z_CONFIG_LISTEN_TIMEOUT_KEY, Z_CONFIG_LISTEN_TIMEOUT_DEFAULT,
                                               &listen_timeout_ms));
    _Z_RETURN_IF_ERR(_z_validate_open_timeout(listen_timeout_ms));

    _Z_RETURN_IF_ERR(_z_config_get_bool_default(config, Z_CONFIG_LISTEN_EXIT_ON_FAILURE_KEY,
                                                Z_CONFIG_LISTEN_EXIT_ON_FAILURE_DEFAULT, &listen_exit_on_failure));

    _Z_RETURN_IF_ERR(_z_config_get_i32_default(config, Z_CONFIG_CONNECT_TIMEOUT_KEY, Z_CONFIG_CONNECT_TIMEOUT_DEFAULT,
                                               &connect_timeout_ms));
    _Z_RETURN_IF_ERR(_z_validate_open_timeout(connect_timeout_ms));

    const char *connect_exit_default = _z_open_connect_exit_on_failure_default(mode);
    _Z_RETURN_IF_ERR(_z_config_get_bool_default(config, Z_CONFIG_CONNECT_EXIT_ON_FAILURE_KEY, connect_exit_default,
                                                &connect_exit_on_failure));
#else
    if (!_z_str_parse_i32(Z_CONFIG_LISTEN_TIMEOUT_DEFAULT, &listen_timeout_ms)) {
        return _Z_ERR_CONFIG_INVALID_VALUE;
    }
    _Z_RETURN_IF_ERR(_z_validate_open_timeout(listen_timeout_ms));

    if (!_z_str_parse_bool(Z_CONFIG_LISTEN_EXIT_ON_FAILURE_DEFAULT, &listen_exit_on_failure)) {
        return _Z_ERR_CONFIG_INVALID_VALUE;
    }

    if (!_z_str_parse_i32(Z_CONFIG_CONNECT_TIMEOUT_DEFAULT, &connect_timeout_ms)) {
        return _Z_ERR_CONFIG_INVALID_VALUE;
    }
    _Z_RETURN_IF_ERR(_z_validate_open_timeout(connect_timeout_ms));

    if (!_z_str_parse_bool(_z_open_connect_exit_on_failure_default(mode), &connect_exit_on_failure)) {
        return _Z_ERR_CONFIG_INVALID_VALUE;
    }
#endif

    switch (mode) {
        case Z_WHATAMI_CLIENT:
            return _z_open_locators_client(zn, connect_locators, zid, config, connect_timeout_ms);

        case Z_WHATAMI_PEER: {
            _z_string_t *listen_locator = (listen_len == 1) ? _z_string_svec_get(listen_locators, 0) : NULL;
            return _z_open_locators_peer(zn, listen_locator, connect_locators, zid, config, listen_timeout_ms,
                                         listen_exit_on_failure, connect_timeout_ms, connect_exit_on_failure);
        }

        default:
            return _Z_ERR_CONFIG_INVALID_MODE;
    }
}

/**
 * Open transports based on the configured listen and connect locators.
 *
 * Semantics:
 * - A "primary transport" must be established before this function returns.
 *   This is achieved by either:
 *     - successfully binding a listen locator, or
 *     - successfully opening one connect locator.
 *
 * - In peer mode:
 *     - If a listen locator is provided, it is attempted first.
 *       A successful bind satisfies the primary transport requirement.
 *     - If no primary transport is established via listen, connect locators
 *       are attempted in order, with retry/backoff applied to retryable failures.
 *
 * - In client mode:
 *     - Only connect locators are used.
 *     - Connect locators are alternatives and at least one must succeed.
 *
 * - Connect locator behaviour:
 *     - Locators are attempted in sequence.
 *     - Retryable failures are retried with exponential backoff until timeout.
 *     - Non-retryable failures permanently exclude the locator from further attempts.
 *     - If configured, a non-retryable failure may cause immediate exit.
 *
 * - Once a primary transport is established in peer mode:
 *     - Remaining connect locators are treated as additional peers and may be added.
 *     - Peer addition may retry synchronously or be continued by a background task, depending on configuration.
 *     - If not all peers are added:
 *         - either an error is returned, or
 *         - partial connectivity is accepted, depending on configuration.
 *
 * - Timeout semantics:
 *     - A timeout of 0 disables retries.
 *     - A timeout of -1 allows infinite retry.
 *
 * Returns:
 * - _Z_RES_OK on success (primary transport established, and peer policy satisfied).
 * - An error if no primary transport could be established, or if peer policy requires
 *   failure (e.g. exit-on-failure with incomplete connectivity).
 */
z_result_t _z_open(_z_session_rc_t *zn, _z_config_t *config, const _z_id_t *zid) {
    z_result_t ret = _Z_RES_OK;
    _Z_RC_IN_VAL(zn)->_tp._type = _Z_TRANSPORT_NONE;

    _z_string_svec_t listen_locators = _z_string_svec_null();
    _z_string_svec_t connect_locators = _z_string_svec_null();

    ret = _z_locators_by_config(config, &listen_locators, &connect_locators);
    if (ret == _Z_RES_OK) {
        z_whatami_t mode;
        ret = _z_config_get_mode(config, &mode);
        if (ret == _Z_RES_OK) {
            _Z_RC_IN_VAL(zn)->_mode = mode;

            if ((_z_string_svec_len(&listen_locators) > 0) || (_z_string_svec_len(&connect_locators) > 0)) {
                ret = _z_open_locators(zn, &listen_locators, &connect_locators, zid, config, mode);
            } else {
                ret = _z_locators_by_scout(config, zid, &connect_locators);
                if (ret == _Z_RES_OK) {
                    if (_z_string_svec_len(&connect_locators) == 0) {
                        ret = _Z_ERR_SCOUT_NO_RESULTS;
                    } else {
                        ret = _z_open_locators(zn, &listen_locators, &connect_locators, zid, config, mode);
                    }
                }
            }
        }
    }

    _z_string_svec_clear(&listen_locators);
    _z_string_svec_clear(&connect_locators);
    return ret;
}

#if Z_FEATURE_AUTO_RECONNECT == 1
void _z_client_reopen_task_drop(void *ztc_arg) {
    _z_transport_common_t *tc = (_z_transport_common_t *)ztc_arg;
    if (tc->_state == _Z_TRANSPORT_STATE_RECONNECTING) {
        // Drop the weak session reference as the task is being dropped in the middle of reconnection.
        _z_session_weak_drop(&tc->_session);
        tc->_state = _Z_TRANSPORT_STATE_CLOSED;
    }
}

_z_fut_fn_result_t _z_client_reopen_task_fn(void *ztc_arg, _z_executor_t *executor) {
    _z_transport_common_t *tc = (_z_transport_common_t *)ztc_arg;
    _z_transport_tasks_t tasks_handles = tc->_tasks;
    _z_session_rc_t zs = _z_session_weak_upgrade(&tc->_session);  // should not fail
    _z_session_t *s = _Z_RC_IN_VAL(&zs);
    _z_session_weak_drop(&tc->_session);

    if (_z_config_is_empty(&s->_config)) {
        _z_session_rc_drop(&zs);
        return _z_fut_fn_result_ready();
    }
    _z_session_transport_mutex_lock(s);
    z_result_t ret = _z_open(&zs, &s->_config, &s->_local_zid);
    _z_session_transport_mutex_unlock(s);
    if (ret != _Z_RES_OK) {
        if (ret == _Z_ERR_TRANSPORT_OPEN_FAILED || ret == _Z_ERR_SCOUT_NO_RESULTS ||
            ret == _Z_ERR_TRANSPORT_TX_FAILED || ret == _Z_ERR_TRANSPORT_RX_FAILED ||
            ret == _Z_ERR_TRANSPORT_RX_DURATION_EXPIRED || ret == _Z_ERR_TRANSPORT_OPEN_PARTIAL_CONNECTIVITY) {
            _Z_DEBUG("Reopen failed, next try in 1s");
            tc->_session = _z_session_rc_clone_as_weak(&zs);
            tc->_state = _Z_TRANSPORT_STATE_RECONNECTING;
            tc->_tasks = tasks_handles;
            _z_session_rc_drop(&zs);
            return _z_fut_fn_result_wake_up_after(1000);
        } else {
            _Z_ERROR("Reopen failed, will not retry");
            tc->_state = _Z_TRANSPORT_STATE_CLOSED;
            _z_session_rc_drop(&zs);
            return _z_fut_fn_result_ready();
        }
    }

    tc->_tasks = tasks_handles;
    ret = _z_interest_resend_client_declarations(s);
    if (ret != _Z_RES_OK) {
        _Z_DEBUG("Resending declarations during reopen failed: %i", ret);
        _z_transport_clear(&s->_tp);
        tc->_session = _z_session_rc_clone_as_weak(&zs);
        tc->_state = _Z_TRANSPORT_STATE_RECONNECTING;
        _z_session_rc_drop(&zs);
        return _z_fut_fn_result_continue();
    }
    _z_session_rc_drop(&zs);
    _Z_DEBUG("Reconnected successfully");
    // Resume all sibling tasks that suspended themselves while waiting for reconnection.
    for (size_t i = 0; i < _Z_TRANSPORT_TASK_COUNT; i++) {
        _z_executor_resume_suspended_fut(executor, &tc->_tasks._task_handles[i]);
    }
    return _z_fut_fn_result_ready();
}
#endif  // Z_FEATURE_AUTO_RECONNECT == 1

bool _z_session_is_closed(const _z_session_t *session) {
    return _z_atomic_bool_load((_z_atomic_bool_t *)&session->_is_closed, _z_memory_order_acquire);
}

bool _z_session_has_router_peer(const _z_session_t *session) {
    if (session->_tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_transport_peer_unicast_slist_t *peers = session->_tp._transport._unicast._peers;
        while (peers != NULL) {
            _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(peers);
            if (peer->common._remote_whatami == Z_WHATAMI_ROUTER) {
                return true;
            }
            peers = _z_transport_peer_unicast_slist_next(peers);
        }
    } else if (session->_tp._type == _Z_TRANSPORT_MULTICAST_TYPE) {
        _z_transport_peer_multicast_slist_t *peers = session->_tp._transport._multicast._peers;
        while (peers != NULL) {
            _z_transport_peer_multicast_t *peer = _z_transport_peer_multicast_slist_value(peers);
            if (peer->common._remote_whatami == Z_WHATAMI_ROUTER) {
                return true;
            }
            peers = _z_transport_peer_multicast_slist_next(peers);
        }
    }
    return false;
}

_z_session_rc_t _z_session_weak_upgrade_if_open(const _z_session_weak_t *weak) {
    _z_session_rc_t sess_rc = _z_session_weak_upgrade(weak);
    if (!_Z_RC_IS_NULL(&sess_rc) && _z_session_is_closed(_Z_RC_IN_VAL(&sess_rc))) {
        _z_session_rc_drop(&sess_rc);
    }
    return sess_rc;
}

_z_config_t *_z_info(const _z_session_t *zn) {
    _z_config_t *ps = (_z_config_t *)z_malloc(sizeof(_z_config_t));
    if (ps != NULL) {
        _z_config_init(ps);
        _z_string_t s = _z_id_to_string(&zn->_local_zid);
        _zp_config_insert_string(ps, Z_INFO_PID_KEY, &s);
        _z_string_clear(&s);

        switch (zn->_tp._type) {
            case _Z_TRANSPORT_UNICAST_TYPE:
                _zp_unicast_info_session(&zn->_tp, ps, zn->_mode);
                break;
            case _Z_TRANSPORT_MULTICAST_TYPE:
            case _Z_TRANSPORT_RAWETH_TYPE:
                _zp_multicast_info_session(&zn->_tp, ps);
                break;
            default:
                break;
        }
    }

    return ps;
}

z_result_t _zp_read(_z_session_t *zn, bool single_read) { return _z_read(&zn->_tp, single_read); }

z_result_t _zp_send_keep_alive(_z_session_t *zn) { return _z_send_keep_alive(&zn->_tp); }

z_result_t _zp_send_join(_z_session_t *zn) { return _z_send_join(&zn->_tp); }

z_result_t _zp_start_transport_tasks(_z_session_t *zn) {
    switch (zn->_tp._type) {
#if Z_FEATURE_UNICAST_TRANSPORT == 1
        case _Z_TRANSPORT_UNICAST_TYPE: {
            _z_transport_common_t *tc = &zn->_tp._transport._unicast._common;
            // Order must match _Z_TRANSPORT_TASK_* index constants.
            _z_fut_fn_t tasks[_Z_TRANSPORT_TASK_COUNT] = {0};
            tasks[_Z_TRANSPORT_TASK_KEEP_ALIVE] = _zp_unicast_keep_alive_task_fn;
            tasks[_Z_TRANSPORT_TASK_LEASE] = _zp_unicast_lease_task_fn;
            tasks[_Z_TRANSPORT_TASK_READ] = _zp_unicast_read_task_fn;
#if Z_FEATURE_UNICAST_PEER == 1
            tasks[_Z_TRANSPORT_TASK_ADD_PEERS] = _zp_add_peers_task_fn;
#endif

            for (size_t i = 0; i < _ZP_ARRAY_SIZE(tasks); i++) {
                if (tasks[i] == NULL) continue;
                _z_fut_t f = _z_fut_null();
                f._fut_arg = &zn->_tp._transport._unicast;
                f._fut_fn = tasks[i];
                _z_fut_handle_t h = _z_runtime_spawn(&zn->_runtime, &f);
                if (_z_fut_handle_is_null(h)) {
                    _Z_ERROR_RETURN(_Z_ERR_FAILED_TO_SPAWN_TASK);
                }
#if Z_FEATURE_AUTO_RECONNECT == 1
                tc->_tasks._task_handles[i] = h;
#endif
            }
            break;
        }
#endif
#if Z_FEATURE_MULTICAST_TRANSPORT == 1
        case _Z_TRANSPORT_MULTICAST_TYPE: {
            _z_transport_common_t *tc = &zn->_tp._transport._multicast._common;
            // Order must match _Z_TRANSPORT_TASK_* index constants.
            _z_fut_fn_t tasks[_Z_TRANSPORT_TASK_COUNT] = {0};
            tasks[_Z_TRANSPORT_TASK_KEEP_ALIVE] = _zp_multicast_keep_alive_task_fn;
            tasks[_Z_TRANSPORT_TASK_LEASE] = _zp_multicast_lease_task_fn;
            tasks[_Z_TRANSPORT_TASK_READ] = _zp_multicast_read_task_fn;
            tasks[_Z_TRANSPORT_TASK_SEND_JOIN] = _zp_multicast_send_join_task_fn;

            for (size_t i = 0; i < _ZP_ARRAY_SIZE(tasks); i++) {
                if (tasks[i] == NULL) continue;
                _z_fut_t f = _z_fut_null();
                f._fut_arg = &zn->_tp._transport._multicast;
                f._fut_fn = tasks[i];
                _z_fut_handle_t h = _z_runtime_spawn(&zn->_runtime, &f);
                if (_z_fut_handle_is_null(h)) {
                    _Z_ERROR_RETURN(_Z_ERR_FAILED_TO_SPAWN_TASK);
                }
#if Z_FEATURE_AUTO_RECONNECT == 1
                tc->_tasks._task_handles[i] = h;
#endif
            }
            break;
        }
#endif
#if Z_FEATURE_RAWETH_TRANSPORT == 1
        case _Z_TRANSPORT_RAWETH_TYPE: {
            _z_transport_common_t *tc = &zn->_tp._transport._raweth._common;
            // Order must match _Z_TRANSPORT_TASK_* index constants.
            _z_fut_fn_t tasks[_Z_TRANSPORT_TASK_COUNT] = {0};
            tasks[_Z_TRANSPORT_TASK_KEEP_ALIVE] = _zp_multicast_keep_alive_task_fn;
            tasks[_Z_TRANSPORT_TASK_LEASE] = _zp_multicast_lease_task_fn;
            tasks[_Z_TRANSPORT_TASK_READ] = _zp_raweth_read_task_fn;
            tasks[_Z_TRANSPORT_TASK_SEND_JOIN] = _zp_multicast_send_join_task_fn;

            for (size_t i = 0; i < _ZP_ARRAY_SIZE(tasks); i++) {
                if (tasks[i] == NULL) continue;
                _z_fut_t f = _z_fut_null();
                f._fut_arg = &zn->_tp._transport._raweth;
                f._fut_fn = tasks[i];
                _z_fut_handle_t h = _z_runtime_spawn(&zn->_runtime, &f);
                if (_z_fut_handle_is_null(h)) {
                    _Z_ERROR_RETURN(_Z_ERR_FAILED_TO_SPAWN_TASK);
                }
#if Z_FEATURE_AUTO_RECONNECT == 1
                tc->_tasks._task_handles[i] = h;
#endif
            }
            break;
        }
#endif
        default:
            _Z_ERROR_LOG(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
            return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            break;
    }
    return _Z_RES_OK;
}
