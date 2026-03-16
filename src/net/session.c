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
#include <stdlib.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/declarations.h"
#include "zenoh-pico/protocol/definitions/network.h"
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
#include "zenoh-pico/utils/uuid.h"

#define _Z_OPEN_BACKOFF_MIN_MS 100
#define _Z_OPEN_BACKOFF_MAX_MS 1000

#if Z_FEATURE_MULTI_THREAD == 1
static bool _zp_read_task_is_running_session(const _z_session_t *zs) {
    _z_transport_common_t *common = _z_transport_get_common((_z_transport_t *)&zs->_tp);
    if (common == NULL) {
        return false;
    }
    return common->_read_task_running;
}

static bool _zp_lease_task_is_running_session(const _z_session_t *zs) {
    _z_transport_common_t *common = _z_transport_get_common((_z_transport_t *)&zs->_tp);
    if (common == NULL) {
        return false;
    }
    return common->_lease_task_running;
}

#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_PERIODIC_TASKS == 1
static bool _zp_periodic_task_is_running(const _z_session_t *zs) { return zs->_periodic_scheduler._task_running; }
#endif
#endif
#endif

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
    char *listen = _z_config_get(config, Z_CONFIG_LISTEN_KEY);
    char *connect = _z_config_get(config, Z_CONFIG_CONNECT_KEY);

    if ((listen == NULL) && (connect == NULL)) {
        return _Z_RES_OK;
    }

#if Z_FEATURE_UNICAST_PEER == 1
    if (listen != NULL) {
        _z_string_t s = _z_string_copy_from_str(listen);
        _Z_RETURN_IF_ERR(_z_string_svec_append(listen_locators, &s, true));
        _zp_config_insert(config, Z_CONFIG_MODE_KEY, Z_CONFIG_MODE_PEER);
    }
    return _z_config_get_all(config, connect_locators, Z_CONFIG_CONNECT_KEY);
#else
    if ((listen != NULL) && (connect != NULL)) _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

if (listen != NULL) {
    _Z_RETURN_IF_ERR(_z_config_get_all(config, listen_locators, Z_CONFIG_LISTEN_KEY));
    _zp_config_insert(config, Z_CONFIG_MODE_KEY, Z_CONFIG_MODE_PEER);
    return _Z_BATCHING_ACTIVE
} else {
    return _z_config_get_all(config, connect_locators, Z_CONFIG_CONNECT_KEY);
}
#endif
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

    ret = _z_new_transport(&zn->_tp, zid, locator, zn->_mode, peer_op, config);
    if (ret != _Z_RES_OK) {
        return ret;
    }
    _z_transport_get_common(&zn->_tp)->_session = _z_session_rc_clone_as_weak(zs);
#if Z_FEATURE_MULTICAST_DECLARATIONS == 1
    if (zn->_tp._type == _Z_TRANSPORT_MULTICAST_TYPE) {
        ret = _z_interest_pull_resource_from_peers(zn);
    }
#endif
    return ret;
}

z_result_t _z_open_locators(_z_session_rc_t *zn, const _z_string_svec_t *listen_locators,
                            const _z_string_svec_t *connect_locators, const _z_id_t *zid, _z_config_t *config,
                            z_whatami_t mode, uint32_t timeout_ms) {
    size_t listen_len = _z_string_svec_len(listen_locators);
    size_t connect_len = _z_string_svec_len(connect_locators);

    if ((listen_len == 0) && (connect_len == 0)) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    z_result_t ret = _Z_ERR_TRANSPORT_OPEN_FAILED;
    z_clock_t now = z_clock_now();
    uint32_t sleep_ms = _Z_OPEN_BACKOFF_MIN_MS;
    size_t opened_connect_idx = SIZE_MAX;

    /* Open a single primary locator.
     * If listen locators are configured, one of them is used as the primary open.
     * Otherwise one connect locator is opened with retry/backoff.
     */
    while (ret != _Z_RES_OK) {
        if (listen_len > 0) {
            for (size_t i = 0; i < listen_len; i++) {
                _z_string_t *locator = _z_string_svec_get(listen_locators, i);
                ret = _z_open_inner(zn, locator, zid, _Z_PEER_OP_LISTEN, config);
                if (ret == _Z_RES_OK) {
                    break;
                }
            }
        } else {
            for (size_t i = 0; i < connect_len; i++) {
                _z_string_t *locator = _z_string_svec_get(connect_locators, i);
                ret = _z_open_inner(zn, locator, zid, _Z_PEER_OP_OPEN, config);
                if (ret == _Z_RES_OK) {
                    opened_connect_idx = i;
                    break;
                }
            }
        }

        if (ret != _Z_RES_OK) {
            unsigned long elapsed = z_clock_elapsed_ms(&now);
            if (elapsed >= timeout_ms) {
                break;
            }

            uint32_t remaining_ms = timeout_ms - (uint32_t)elapsed;
            uint32_t current_sleep_ms = sleep_ms;
            if (current_sleep_ms > remaining_ms) {
                current_sleep_ms = remaining_ms;
            }

            z_sleep_ms(current_sleep_ms);

            if (sleep_ms < _Z_OPEN_BACKOFF_MAX_MS) {
                sleep_ms <<= 1;
                if (sleep_ms > _Z_OPEN_BACKOFF_MAX_MS) {
                    sleep_ms = _Z_OPEN_BACKOFF_MAX_MS;
                }
            }
        }
    }

#if Z_FEATURE_UNICAST_PEER == 1
    if ((ret == _Z_RES_OK) && (mode == Z_WHATAMI_PEER)) {
        for (size_t i = 0; i < connect_len; i++) {
            if (i == opened_connect_idx) {
                continue;
            }

            _z_string_t *locator = _z_string_svec_get(connect_locators, i);
            z_result_t peer_ret = _z_new_peer(&_Z_RC_IN_VAL(zn)->_tp, &_Z_RC_IN_VAL(zn)->_local_zid, locator, config);
            if (peer_ret != _Z_RES_OK) {
                _Z_WARN("Failed to add peer locator %zu", i);
            }
        }
    }
#endif

    return ret;
}

z_result_t _z_open(_z_session_rc_t *zn, _z_config_t *config, uint32_t timeout_ms, const _z_id_t *zid) {
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
                ret = _z_open_locators(zn, &listen_locators, &connect_locators, zid, config, mode, timeout_ms);
            } else {
                ret = _z_locators_by_scout(config, zid, &connect_locators);
                if (ret == _Z_RES_OK) {
                    if (_z_string_svec_len(&connect_locators) == 0) {
                        ret = _Z_ERR_SCOUT_NO_RESULTS;
                    } else {
                        ret = _z_open_locators(zn, &listen_locators, &connect_locators, zid, config, mode, timeout_ms);
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

z_result_t _z_reopen(_z_session_rc_t *zn) {
    z_result_t ret = _Z_RES_OK;
    _z_session_t *zs = _Z_RC_IN_VAL(zn);
    if (_z_config_is_empty(&zs->_config)) {
        return ret;
    }

    do {
        _z_session_transport_mutex_lock(zs);
        ret = _z_open(zn, &zs->_config, 0, &zs->_local_zid);
        _z_session_transport_mutex_unlock(zs);
        if (ret != _Z_RES_OK) {
            if (ret == _Z_ERR_TRANSPORT_OPEN_FAILED || ret == _Z_ERR_SCOUT_NO_RESULTS ||
                ret == _Z_ERR_TRANSPORT_TX_FAILED || ret == _Z_ERR_TRANSPORT_RX_FAILED) {
                _Z_DEBUG("Reopen failed, next try in 1s");
                z_sleep_s(1);
                continue;
            } else {
                return ret;
            }
        }

#if Z_FEATURE_MULTI_THREAD == 1
        if (zs->_lease_task_should_run) {
            ret = _zp_start_lease_task(zs, zs->_lease_task_attr);
            if (ret != _Z_RES_OK) {
                return ret;
            }
        }
        if (zs->_read_task_should_run) {
            ret = _zp_start_read_task(zs, zs->_read_task_attr);
            if (ret != _Z_RES_OK) {
                return ret;
            }
        }
#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_PERIODIC_TASKS == 1
        if (zs->_periodic_task_should_run) {
            ret = _zp_start_periodic_scheduler_task(zs, zs->_periodic_scheduler_task_attr);
            if (ret != _Z_RES_OK) {
                return ret;
            }
        }
#endif
#endif
#endif  // Z_FEATURE_MULTI_THREAD == 1

        if (ret == _Z_RES_OK && !_z_network_message_slist_is_empty(zs->_declaration_cache)) {
            _z_network_message_slist_t *iter = zs->_declaration_cache;
            while (iter != NULL) {
                _z_network_message_t *n_msg = _z_network_message_slist_value(iter);
                ret = _z_send_n_msg(zs, n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK, NULL);
                if (ret != _Z_RES_OK) {
                    _Z_DEBUG("Send message during reopen failed: %i", ret);
                    continue;
                }

                iter = _z_network_message_slist_next(iter);
            }
        }
    } while (ret != _Z_RES_OK);

    return ret;
}

void _z_cache_declaration(_z_session_t *zs, const _z_network_message_t *n_msg) {
    if (_z_config_is_empty(&zs->_config)) {
        return;
    }
    zs->_declaration_cache = _z_network_message_slist_push_back(zs->_declaration_cache, n_msg);
}

#define _Z_CACHE_DECLARATION_UNDECLARE_FILTER(tp)                                                                     \
    static bool _z_cache_declaration_undeclare_filter_##tp(const _z_network_message_t *left,                          \
                                                           const _z_network_message_t *right) {                       \
        return left->_tag == _Z_N_DECLARE && right->_tag == _Z_N_DECLARE &&                                           \
               left->_body._declare._decl._body._undecl_##tp._id == right->_body._declare._decl._body._decl_##tp._id; \
    }
_Z_CACHE_DECLARATION_UNDECLARE_FILTER(kexpr)
_Z_CACHE_DECLARATION_UNDECLARE_FILTER(subscriber)
_Z_CACHE_DECLARATION_UNDECLARE_FILTER(queryable)
_Z_CACHE_DECLARATION_UNDECLARE_FILTER(token)

static bool _z_cache_declaration_undeclare_filter_interest(const _z_network_message_t *left,
                                                           const _z_network_message_t *right) {
    return left->_tag == _Z_N_INTEREST && right->_tag == _Z_N_INTEREST &&
           left->_body._interest._interest._id == right->_body._interest._interest._id;
}

void _z_prune_declaration(_z_session_t *zs, const _z_network_message_t *n_msg) {
#ifdef Z_BUILD_DEBUG
    size_t cnt_before = _z_network_message_slist_len(zs->_declaration_cache);
#endif
    switch (n_msg->_tag) {
        case _Z_N_DECLARE: {
            const _z_declaration_t *decl = &n_msg->_body._declare._decl;
            switch (decl->_tag) {
                case _Z_UNDECL_KEXPR:
                    zs->_declaration_cache = _z_network_message_slist_drop_first_filter(
                        zs->_declaration_cache, _z_cache_declaration_undeclare_filter_kexpr, n_msg);
                    break;
                case _Z_UNDECL_SUBSCRIBER:
                    zs->_declaration_cache = _z_network_message_slist_drop_first_filter(
                        zs->_declaration_cache, _z_cache_declaration_undeclare_filter_subscriber, n_msg);
                    break;
                case _Z_UNDECL_QUERYABLE:
                    zs->_declaration_cache = _z_network_message_slist_drop_first_filter(
                        zs->_declaration_cache, _z_cache_declaration_undeclare_filter_queryable, n_msg);
                    break;
                case _Z_UNDECL_TOKEN:
                    zs->_declaration_cache = _z_network_message_slist_drop_first_filter(
                        zs->_declaration_cache, _z_cache_declaration_undeclare_filter_token, n_msg);
                    break;
                default:
                    _Z_ERROR("Invalid decl for _z_prune_declaration: %i", decl->_tag);
            };
            break;
        }
        case _Z_N_INTEREST:
            zs->_declaration_cache = _z_network_message_slist_drop_first_filter(
                zs->_declaration_cache, _z_cache_declaration_undeclare_filter_interest, n_msg);
            break;
        default:
            _Z_ERROR("Invalid net message for _z_prune_declaration: %i", n_msg->_tag);
            return;
    };
#ifdef Z_BUILD_DEBUG
    size_t cnt_after = _z_network_message_slist_len(zs->_declaration_cache);
    assert(cnt_before == cnt_after + 1);
#endif
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

#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_PERIODIC_TASKS == 1
z_result_t _zp_process_periodic_tasks(_z_session_t *zn) {
    return _zp_periodic_scheduler_process_tasks(&zn->_periodic_scheduler);
}

z_result_t _zp_periodic_task_add(_z_session_t *zn, _zp_closure_periodic_task_t *closure, uint64_t period_ms,
                                 uint32_t *id) {
    return _zp_periodic_scheduler_add(&zn->_periodic_scheduler, closure, period_ms, id);
}

z_result_t _zp_periodic_task_remove(_z_session_t *zn, uint32_t id) {
    return _zp_periodic_scheduler_remove(&zn->_periodic_scheduler, id);
}
#endif
#endif

#if Z_FEATURE_MULTI_THREAD == 1
z_result_t _zp_start_read_task(_z_session_t *zn, z_task_attr_t *attr) {
    if (_zp_read_task_is_running_session(zn)) {
        zn->_read_task_should_run = true;
        return _Z_RES_OK;
    }

    z_result_t ret = _Z_RES_OK;
    // Allocate task
    _z_task_t *task = (_z_task_t *)z_malloc(sizeof(_z_task_t));
    if (task == NULL) {
        _Z_ERROR_LOG(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Call transport function
    switch (zn->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            ret = _zp_unicast_start_read_task(&zn->_tp, attr, task);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            ret = _zp_multicast_start_read_task(&zn->_tp, attr, task);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            ret = _zp_raweth_start_read_task(&zn->_tp, attr, task);
            break;
        default:
            _Z_ERROR_LOG(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            break;
    }
    // Free task if operation failed
    if (ret != _Z_RES_OK) {
        z_free(task);
        return ret;
    }

#if Z_FEATURE_AUTO_RECONNECT == 1
    zn->_read_task_attr = attr;
#endif
    zn->_read_task_should_run = true;
    return ret;
}

z_result_t _zp_start_lease_task(_z_session_t *zn, z_task_attr_t *attr) {
    if (_zp_lease_task_is_running_session(zn)) {
        zn->_lease_task_should_run = true;
        return _Z_RES_OK;
    }

    z_result_t ret = _Z_RES_OK;
    // Allocate task
    _z_task_t *task = (_z_task_t *)z_malloc(sizeof(_z_task_t));
    if (task == NULL) {
        _Z_ERROR_LOG(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Call transport function
    switch (zn->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            ret = _zp_unicast_start_lease_task(&zn->_tp, attr, task);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            ret = _zp_multicast_start_lease_task(&zn->_tp._transport._multicast, attr, task);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            ret = _zp_multicast_start_lease_task(&zn->_tp._transport._raweth, attr, task);
            break;
        default:
            _Z_ERROR_LOG(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            break;
    }
    // Free task if operation failed
    if (ret != _Z_RES_OK) {
        z_free(task);
        return ret;
    }

#if Z_FEATURE_AUTO_RECONNECT == 1
    zn->_lease_task_attr = attr;
#endif
    zn->_lease_task_should_run = true;
    return ret;
}

#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_PERIODIC_TASKS == 1
z_result_t _zp_start_periodic_scheduler_task(_z_session_t *zn, z_task_attr_t *attr) {
    if (_zp_periodic_task_is_running(zn)) {
        zn->_periodic_task_should_run = true;
        return _Z_RES_OK;
    }

    // Allocate task
    _z_task_t *task = (_z_task_t *)z_malloc(sizeof(_z_task_t));
    if (task == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    z_result_t ret = _zp_periodic_scheduler_start_task(&zn->_periodic_scheduler, attr, task);
    if (ret != _Z_RES_OK) {
        z_free(task);
        _Z_ERROR_RETURN(ret);
    }
    // Attach task
    zn->_periodic_scheduler_task = task;
#if Z_FEATURE_AUTO_RECONNECT == 1
    zn->_periodic_scheduler_task_attr = attr;
#endif
    zn->_periodic_task_should_run = true;
    return ret;
}
#endif  // Z_FEATURE_PERIODIC_TASKS == 1
#endif  // Z_FEATURE_UNSTABLE_API

z_result_t _zp_stop_read_task(_z_session_t *zn) {
    z_result_t ret = _Z_RES_OK;
    // Call transport function
    switch (zn->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            ret = _zp_unicast_stop_read_task(&zn->_tp);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            ret = _zp_multicast_stop_read_task(&zn->_tp);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            ret = _zp_raweth_stop_read_task(&zn->_tp);
            break;
        default:
            _Z_ERROR_LOG(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            break;
    }
    if (ret == _Z_RES_OK) {
        zn->_read_task_should_run = false;
    }
    return ret;
}

z_result_t _zp_stop_lease_task(_z_session_t *zn) {
    z_result_t ret = _Z_RES_OK;
    // Call transport function
    switch (zn->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            ret = _zp_unicast_stop_lease_task(&zn->_tp);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            ret = _zp_multicast_stop_lease_task(&zn->_tp._transport._multicast);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            ret = _zp_multicast_stop_lease_task(&zn->_tp._transport._raweth);
            break;
        default:
            _Z_ERROR_LOG(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            break;
    }
    if (ret == _Z_RES_OK) {
        zn->_lease_task_should_run = false;
    }
    return ret;
}

#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_PERIODIC_TASKS == 1
z_result_t _zp_stop_periodic_scheduler_task(_z_session_t *zn) {
    if (zn->_periodic_scheduler_task == NULL) {
        return _Z_ERR_INVALID;
    }

    z_result_t ret = _zp_periodic_scheduler_stop_task(&zn->_periodic_scheduler);
    if (ret != _Z_RES_OK) {
        _Z_ERROR_RETURN(ret);
    }

    ret = _z_task_join(zn->_periodic_scheduler_task);
    if (ret != _Z_RES_OK) {
        _Z_ERROR_RETURN(ret);
    }

    _z_task_t *task = zn->_periodic_scheduler_task;
    zn->_periodic_scheduler_task = NULL;
    z_free(task);

    zn->_periodic_task_should_run = false;
#if Z_FEATURE_AUTO_RECONNECT == 1
    zn->_periodic_scheduler_task_attr = NULL;
#endif

    return _Z_RES_OK;
}
#endif  // Z_FEATURE_PERIODIC_TASKS == 1
#endif  // Z_FEATURE_UNSTABLE_API
#endif  // Z_FEATURE_MULTI_THREAD == 1
