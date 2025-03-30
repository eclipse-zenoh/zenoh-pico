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

#include <stddef.h>
#include <stdlib.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/declarations.h"
#include "zenoh-pico/protocol/definitions/network.h"
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
    _z_hello_list_t *hellos = _z_scout_inner(what, *zid, &mcast_locator, timeout, true);
    if (hellos != NULL) {
        _z_hello_t *hello = _z_hello_list_head(hellos);
        _z_string_svec_copy(locators, &hello->_locators, true);
    }
    _z_hello_list_free(&hellos);
    return ret;
}

static z_result_t _z_locators_by_config(_z_config_t *config, _z_string_svec_t *locators, int *peer_op) {
    char *connect = _z_config_get(config, Z_CONFIG_CONNECT_KEY);
    char *listen = _z_config_get(config, Z_CONFIG_LISTEN_KEY);
    if (connect == NULL && listen == NULL) {
        return _Z_RES_OK;
    }
#if Z_FEATURE_UNICAST_PEER == 1
    if (listen != NULL) {
        // Add listen as first endpoint
        _z_string_t s = _z_string_copy_from_str(listen);
        _Z_RETURN_IF_ERR(_z_string_svec_append(locators, &s, true));
        _zp_config_insert(config, Z_CONFIG_MODE_KEY, Z_CONFIG_MODE_PEER);
    } else {
        *peer_op = _Z_PEER_OP_OPEN;
    }
    // Add open endpoints
    return _z_config_get_all(config, locators, Z_CONFIG_CONNECT_KEY);
#else
    uint_fast8_t key = Z_CONFIG_CONNECT_KEY;
    if (listen != NULL) {
        if (connect == NULL) {
            key = Z_CONFIG_LISTEN_KEY;
            _zp_config_insert(config, Z_CONFIG_MODE_KEY, Z_CONFIG_MODE_PEER);
        } else {
            return _Z_ERR_GENERIC;
        }
    } else {
        *peer_op = _Z_PEER_OP_OPEN;
    }
    return _z_config_get_all(config, locators, key);
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
            ret = _Z_ERR_CONFIG_INVALID_MODE;
        }
    }
    return ret;
}

static z_result_t _z_open_inner(_z_session_rc_t *zn, _z_string_t *locator, const _z_id_t *zid, int peer_op) {
    z_result_t ret = _Z_RES_OK;

    ret = _z_new_transport(&_Z_RC_IN_VAL(zn)->_tp, zid, locator, _Z_RC_IN_VAL(zn)->_mode, peer_op);
    if (ret != _Z_RES_OK) {
        return ret;
    }

    _z_transport_get_common(&_Z_RC_IN_VAL(zn)->_tp)->_session = zn;
    return ret;
}

z_result_t _z_open(_z_session_rc_t *zn, _z_config_t *config, const _z_id_t *zid) {
    z_result_t ret = _Z_RES_OK;
    _Z_RC_IN_VAL(zn)->_tp._type = _Z_TRANSPORT_NONE;

    int peer_op = _Z_PEER_OP_LISTEN;
    _z_string_svec_t locators = _z_string_svec_null();
    // Try getting locators from config
    _Z_RETURN_IF_ERR(_z_locators_by_config(config, &locators, &peer_op));
    size_t len = _z_string_svec_len(&locators);

    z_whatami_t mode;
    ret = _z_config_get_mode(config, &mode);
    if (ret != _Z_RES_OK) {
        return ret;
    }
    _Z_RC_IN_VAL(zn)->_mode = mode;

    if (len > 0) {
        // Use first locator to open session
        _z_string_t *locator = _z_string_svec_get(&locators, 0);
        ret = _z_open_inner(zn, locator, zid, peer_op);
#if Z_FEATURE_UNICAST_PEER == 1
        // Add other locators as peers if applicable
        if ((ret == _Z_RES_OK) && (mode == Z_WHATAMI_PEER)) {
            for (size_t i = 1; i < len; i++) {
                // Add peer
                locator = _z_string_svec_get(&locators, i);
                ret = _z_new_peer(&_Z_RC_IN_VAL(zn)->_tp, &_Z_RC_IN_VAL(zn)->_local_zid, locator);
                if (ret != _Z_RES_OK) {
                    break;
                }
            }
        }
#endif
    } else {  // No locators in config, scout
        _Z_RETURN_IF_ERR(_z_locators_by_scout(config, zid, &locators));
        len = _z_string_svec_len(&locators);
        if (len == 0) {
            return _Z_ERR_SCOUT_NO_RESULTS;
        }
        // Loop on locators until we successfully open one
        for (size_t i = 0; i < len; i++) {
            _z_string_t *locator = _z_string_svec_get(&locators, i);
            ret = _z_open_inner(zn, locator, zid, peer_op);
            if (ret == _Z_RES_OK) {
                break;
            }
        }
    }
    _z_string_svec_clear(&locators);
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
        ret = _z_open(zn, &zs->_config, &zs->_local_zid);
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
        ret = _zp_start_lease_task(zs, zs->_lease_task_attr);
        if (ret != _Z_RES_OK) {
            return ret;
        }
        ret = _zp_start_read_task(zs, zs->_read_task_attr);
        if (ret != _Z_RES_OK) {
            return ret;
        }
#endif  // Z_FEATURE_MULTI_THREAD == 1

        if (ret == _Z_RES_OK && !_z_network_message_list_is_empty(zs->_decalaration_cache)) {
            _z_network_message_list_t *iter = zs->_decalaration_cache;
            while (iter != NULL) {
                _z_network_message_t *n_msg = _z_network_message_list_head(iter);
                ret = _z_send_n_msg(zs, n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
                if (ret != _Z_RES_OK) {
                    _Z_DEBUG("Send message during reopen failed: %i", ret);
                    continue;
                }

                iter = _z_network_message_list_tail(iter);
            }
        }
    } while (ret != _Z_RES_OK);

    return ret;
}

void _z_cache_declaration(_z_session_t *zs, const _z_network_message_t *n_msg) {
    if (_z_config_is_empty(&zs->_config)) {
        return;
    }
    zs->_decalaration_cache = _z_network_message_list_push_back(zs->_decalaration_cache, _z_n_msg_clone(n_msg));
}

#define _Z_CACHE_DECLARATION_UNDECLARE_FILTER(tp)                                                                     \
    static bool _z_cache_declaration_undeclare_filter_##tp(const _z_network_message_t *left,                          \
                                                           const _z_network_message_t *right) {                       \
        return left->_body._declare._decl._body._undecl_##tp._id == right->_body._declare._decl._body._decl_##tp._id; \
    }
_Z_CACHE_DECLARATION_UNDECLARE_FILTER(kexpr)
_Z_CACHE_DECLARATION_UNDECLARE_FILTER(subscriber)
_Z_CACHE_DECLARATION_UNDECLARE_FILTER(queryable)
_Z_CACHE_DECLARATION_UNDECLARE_FILTER(token)

void _z_prune_declaration(_z_session_t *zs, const _z_network_message_t *n_msg) {
    if (n_msg->_tag != _Z_N_DECLARE) {
        _Z_ERROR("Invalid net message for _z_prune_declaration: %i", n_msg->_tag);
        return;
    }
#ifdef Z_BUILD_DEBUG
    size_t cnt_before = _z_network_message_list_len(zs->_decalaration_cache);
#endif
    const _z_declaration_t *decl = &n_msg->_body._declare._decl;
    switch (decl->_tag) {
        case _Z_UNDECL_KEXPR:
            zs->_decalaration_cache = _z_network_message_list_drop_filter(
                zs->_decalaration_cache, _z_cache_declaration_undeclare_filter_kexpr, n_msg);
            break;
        case _Z_UNDECL_SUBSCRIBER:
            zs->_decalaration_cache = _z_network_message_list_drop_filter(
                zs->_decalaration_cache, _z_cache_declaration_undeclare_filter_subscriber, n_msg);
            break;
        case _Z_UNDECL_QUERYABLE:
            zs->_decalaration_cache = _z_network_message_list_drop_filter(
                zs->_decalaration_cache, _z_cache_declaration_undeclare_filter_queryable, n_msg);
            break;
        case _Z_UNDECL_TOKEN:
            zs->_decalaration_cache = _z_network_message_list_drop_filter(
                zs->_decalaration_cache, _z_cache_declaration_undeclare_filter_token, n_msg);
            break;
        default:
            _Z_ERROR("Invalid decl for _z_prune_declaration: %i", decl->_tag);
    };
#ifdef Z_BUILD_DEBUG
    size_t cnt_after = _z_network_message_list_len(zs->_decalaration_cache);
    assert(cnt_before == cnt_after + 1);
#endif
}
#endif  // Z_FEATURE_AUTO_RECONNECT == 1

void _z_close(_z_session_t *zn) { _z_session_close(zn, _Z_CLOSE_GENERIC); }

bool _z_session_is_closed(const _z_session_t *session) { return session->_tp._type == _Z_TRANSPORT_NONE; }

_z_session_rc_t _z_session_weak_upgrade_if_open(const _z_session_weak_t *session) {
    _z_session_rc_t sess_rc = _z_session_weak_upgrade(session);
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

z_result_t _zp_read(_z_session_t *zn) { return _z_read(&zn->_tp); }

z_result_t _zp_send_keep_alive(_z_session_t *zn) { return _z_send_keep_alive(&zn->_tp); }

z_result_t _zp_send_join(_z_session_t *zn) { return _z_send_join(&zn->_tp); }

#if Z_FEATURE_MULTI_THREAD == 1
z_result_t _zp_start_read_task(_z_session_t *zn, z_task_attr_t *attr) {
    z_result_t ret = _Z_RES_OK;
    // Allocate task
    _z_task_t *task = (_z_task_t *)z_malloc(sizeof(_z_task_t));
    if (task == NULL) {
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
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            break;
    }
    // Free task if operation failed
    if (ret != _Z_RES_OK) {
        z_free(task);
#if Z_FEATURE_AUTO_RECONNECT == 1
    } else {
        zn->_read_task_attr = attr;
#endif
    }
    return ret;
}

z_result_t _zp_start_lease_task(_z_session_t *zn, z_task_attr_t *attr) {
    z_result_t ret = _Z_RES_OK;
    // Allocate task
    _z_task_t *task = (_z_task_t *)z_malloc(sizeof(_z_task_t));
    if (task == NULL) {
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
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            break;
    }
    // Free task if operation failed
    if (ret != _Z_RES_OK) {
        z_free(task);
#if Z_FEATURE_AUTO_RECONNECT == 1
    } else {
        zn->_lease_task_attr = attr;
#endif
    }
    return ret;
}

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
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            break;
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
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            break;
    }
    return ret;
}
#endif  // Z_FEATURE_MULTI_THREAD == 1
