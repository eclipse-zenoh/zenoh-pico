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

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/net/sample.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/common/lease.h"
#include "zenoh-pico/transport/common/read.h"
#include "zenoh-pico/transport/multicast.h"
#include "zenoh-pico/transport/multicast/lease.h"
#include "zenoh-pico/transport/multicast/read.h"
#include "zenoh-pico/transport/raweth/read.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/unicast.h"
#include "zenoh-pico/transport/unicast/lease.h"
#include "zenoh-pico/transport/unicast/read.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/uuid.h"

int8_t __z_open_inner(_z_session_rc_t *zn, char *locator, z_whatami_t mode) {
    int8_t ret = _Z_RES_OK;

    _z_id_t local_zid = _z_id_empty();
    ret = _z_session_generate_zid(&local_zid, Z_ZID_LENGTH);
    if (ret != _Z_RES_OK) {
        local_zid = _z_id_empty();
        return ret;
    }
    ret = _z_new_transport(&_Z_RC_IN_VAL(zn)->_tp, &local_zid, locator, mode);
    if (ret != _Z_RES_OK) {
        local_zid = _z_id_empty();
        return ret;
    }
    ret = _z_session_init(zn, &local_zid);
    return ret;
}

int8_t _z_open(_z_session_rc_t *zn, _z_config_t *config) {
    int8_t ret = _Z_RES_OK;

    _z_id_t zid = _z_id_empty();
    char *opt_as_str = _z_config_get(config, Z_CONFIG_SESSION_ZID_KEY);
    if (opt_as_str != NULL) {
        _z_uuid_to_bytes(zid.id, opt_as_str);
    }

    if (config != NULL) {
        _z_string_svec_t locators = _z_string_svec_make(0);
        char *connect = _z_config_get(config, Z_CONFIG_CONNECT_KEY);
        char *listen = _z_config_get(config, Z_CONFIG_LISTEN_KEY);
        if (connect == NULL && listen == NULL) {  // Scout if peer is not configured
            opt_as_str = _z_config_get(config, Z_CONFIG_SCOUTING_WHAT_KEY);
            if (opt_as_str == NULL) {
                opt_as_str = (char *)Z_CONFIG_SCOUTING_WHAT_DEFAULT;
            }
            z_what_t what = strtol(opt_as_str, NULL, 10);

            opt_as_str = _z_config_get(config, Z_CONFIG_MULTICAST_LOCATOR_KEY);
            if (opt_as_str == NULL) {
                opt_as_str = (char *)Z_CONFIG_MULTICAST_LOCATOR_DEFAULT;
            }
            char *mcast_locator = opt_as_str;

            opt_as_str = _z_config_get(config, Z_CONFIG_SCOUTING_TIMEOUT_KEY);
            if (opt_as_str == NULL) {
                opt_as_str = (char *)Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT;
            }
            uint32_t timeout = (uint32_t)strtoul(opt_as_str, NULL, 10);

            // Scout and return upon the first result
            _z_hello_list_t *hellos = _z_scout_inner(what, zid, mcast_locator, timeout, true);
            if (hellos != NULL) {
                _z_hello_t *hello = _z_hello_list_head(hellos);
                _z_string_svec_copy(&locators, &hello->_locators);
            }
            _z_hello_list_free(&hellos);
        } else {
            uint_fast8_t key = Z_CONFIG_CONNECT_KEY;
            if (listen != NULL) {
                if (connect == NULL) {
                    key = Z_CONFIG_LISTEN_KEY;
                    _zp_config_insert(config, Z_CONFIG_MODE_KEY, Z_CONFIG_MODE_PEER);
                } else {
                    return _Z_ERR_GENERIC;
                }
            }
            locators = _z_string_svec_make(1);
            _z_string_t s = _z_string_make(_z_config_get(config, key));
            _z_string_svec_append(&locators, &s);
        }

        ret = _Z_ERR_SCOUT_NO_RESULTS;
        size_t len = _z_string_svec_len(&locators);
        for (size_t i = 0; i < len; i++) {
            ret = _Z_RES_OK;

            _z_string_t *locator = _z_string_svec_get(&locators, i);
            // @TODO: check invalid configurations
            // For example, client mode in multicast links

            // Check operation mode
            char *s_mode = _z_config_get(config, Z_CONFIG_MODE_KEY);
            z_whatami_t mode = Z_WHATAMI_CLIENT;  // By default, zenoh-pico will operate as a client
            if (s_mode != NULL) {
                if (_z_str_eq(s_mode, Z_CONFIG_MODE_CLIENT) == true) {
                    mode = Z_WHATAMI_CLIENT;
                } else if (_z_str_eq(s_mode, Z_CONFIG_MODE_PEER) == true) {
                    mode = Z_WHATAMI_PEER;
                } else {
                    ret = _Z_ERR_CONFIG_INVALID_MODE;
                }
            }

            if (ret == _Z_RES_OK) {
                ret = __z_open_inner(zn, locator->val, mode);
                if (ret == _Z_RES_OK) {
                    break;
                }
            } else {
                _Z_ERROR("Trying to configure an invalid mode.");
            }
        }
        _z_string_svec_clear(&locators);
    } else {
        _Z_ERROR("A valid config is missing.");
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_close(_z_session_t *zn) { _z_session_close(zn, _Z_CLOSE_GENERIC); }

_z_config_t *_z_info(const _z_session_t *zn) {
    _z_config_t *ps = (_z_config_t *)z_malloc(sizeof(_z_config_t));
    if (ps != NULL) {
        _z_config_init(ps);
        _z_slice_t local_zid = _z_slice_wrap(zn->_local_zid.id, _z_id_len(zn->_local_zid));
        // TODO(sasahcmc): is it zero terminated???
        // rework it!!!
        _zp_config_insert(ps, Z_INFO_PID_KEY, _z_string_convert_bytes(&local_zid).val);

        switch (zn->_tp._type) {
            case _Z_TRANSPORT_UNICAST_TYPE:
                _zp_unicast_info_session(&zn->_tp, ps);
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

int8_t _zp_read(_z_session_t *zn) { return _z_read(&zn->_tp); }

int8_t _zp_send_keep_alive(_z_session_t *zn) { return _z_send_keep_alive(&zn->_tp); }

int8_t _zp_send_join(_z_session_t *zn) { return _z_send_join(&zn->_tp); }

#if Z_FEATURE_MULTI_THREAD == 1
int8_t _zp_start_read_task(_z_session_t *zn, z_task_attr_t *attr) {
    int8_t ret = _Z_RES_OK;
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
    }
    return ret;
}

int8_t _zp_start_lease_task(_z_session_t *zn, z_task_attr_t *attr) {
    int8_t ret = _Z_RES_OK;
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
    }
    return ret;
}

int8_t _zp_stop_read_task(_z_session_t *zn) {
    int8_t ret = _Z_RES_OK;
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

int8_t _zp_stop_lease_task(_z_session_t *zn) {
    int8_t ret = _Z_RES_OK;
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
