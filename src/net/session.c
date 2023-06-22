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

#include "zenoh-pico/net/session.h"

#include <stddef.h>
#include <stdlib.h>

#include "zenoh-pico/net/memory.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/link/task/join.h"
#include "zenoh-pico/transport/link/task/lease.h"
#include "zenoh-pico/transport/link/task/read.h"
#include "zenoh-pico/utils/logging.h"

int8_t __z_open_inner(_z_session_t *zn, char *locator, z_whatami_t mode) {
    int8_t ret = _Z_RES_OK;

    _z_bytes_t local_zid = _z_bytes_empty();

#if Z_UNICAST_TRANSPORT == 1 || Z_MULTICAST_TRANSPORT == 1
    ret = _z_session_generate_zid(&local_zid, Z_ZID_LENGTH);
    if (ret == _Z_RES_OK) {
        ret = _z_new_transport(&zn->_tp, &local_zid, locator, mode);
        if (ret != _Z_RES_OK) {
            _z_bytes_clear(&local_zid);
        }
    } else {
        _z_bytes_clear(&local_zid);
    }
#else
    ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
#endif

    if (ret == _Z_RES_OK) {
        ret = _z_session_init(zn, &local_zid);
    }

    return ret;
}

int8_t _z_open(_z_session_t *zn, _z_config_t *config) {
    int8_t ret = _Z_RES_OK;

    if (config != NULL) {
        _z_str_array_t locators = _z_str_array_empty();
        if (_z_config_get(config, Z_CONFIG_PEER_KEY) == NULL) {  // Scout if peer is not configured
            char *opt_as_str = _z_config_get(config, Z_CONFIG_SCOUTING_WHAT_KEY);
            if (opt_as_str == NULL) {
                opt_as_str = Z_CONFIG_SCOUTING_WHAT_DEFAULT;
            }
            z_whatami_t what = strtol(opt_as_str, NULL, 10);

            opt_as_str = _z_config_get(config, Z_CONFIG_MULTICAST_LOCATOR_KEY);
            if (opt_as_str == NULL) {
                opt_as_str = Z_CONFIG_MULTICAST_LOCATOR_DEFAULT;
            }
            char *mcast_locator = opt_as_str;

            opt_as_str = _z_config_get(config, Z_CONFIG_SCOUTING_TIMEOUT_KEY);
            if (opt_as_str == NULL) {
                opt_as_str = Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT;
            }
            uint32_t timeout = strtoul(opt_as_str, NULL, 10);

            // Scout and return upon the first result
            _z_hello_list_t *hellos = _z_scout_inner(what, mcast_locator, timeout, true);
            if (hellos != NULL) {
                _z_hello_t *hello = _z_hello_list_head(hellos);
                _z_str_array_copy(&locators, &hello->locators);
            }
            _z_hello_list_free(&hellos);
        } else {
            locators = _z_str_array_make(1);
            locators.val[0] = _z_str_clone(_z_config_get(config, Z_CONFIG_PEER_KEY));
        }

        ret = _Z_ERR_SCOUT_NO_RESULTS;
        for (size_t i = 0; i < locators.len; i++) {
            ret = _Z_RES_OK;

            char *locator = locators.val[i];
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
                ret = __z_open_inner(zn, locator, mode);
                if (ret == _Z_RES_OK) {
                    break;
                }
            } else {
                _Z_ERROR("Trying to configure an invalid mode.\n");
            }
        }
        _z_str_array_clear(&locators);
    } else {
        _Z_ERROR("A valid config is missing.\n");
    }

    return ret;
}

void _z_close(_z_session_t *zn) { _z_session_close(zn, _Z_CLOSE_GENERIC); }

_z_config_t *_z_info(const _z_session_t *zn) {
    _z_config_t *ps = (_z_config_t *)z_malloc(sizeof(_z_config_t));
    if (ps != NULL) {
        _z_config_init(ps);
        _zp_config_insert(ps, Z_INFO_PID_KEY, _z_string_from_bytes(&zn->_local_zid));

#if Z_UNICAST_TRANSPORT == 1
        if (zn->_tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
            _zp_config_insert(ps, Z_INFO_ROUTER_PID_KEY,
                              _z_string_from_bytes(&zn->_tp._transport._unicast._remote_zid));
        } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
            if (zn->_tp._type == _Z_TRANSPORT_MULTICAST_TYPE) {
            _z_transport_peer_entry_list_t *xs = zn->_tp._transport._multicast._peers;
            while (xs != NULL) {
                _z_transport_peer_entry_t *peer = _z_transport_peer_entry_list_head(xs);
                _zp_config_insert(ps, Z_INFO_PEER_PID_KEY, _z_string_from_bytes(&peer->_remote_zid));

                xs = _z_transport_peer_entry_list_tail(xs);
            }
        } else
#endif  // Z_MULTICAST_TRANSPORT == 1
        {
            __asm__("nop");
        }
    }

    return ps;
}

int8_t _zp_read(_z_session_t *zn) { return _z_read(&zn->_tp); }

int8_t _zp_send_keep_alive(_z_session_t *zn) { return _z_send_keep_alive(&zn->_tp); }

int8_t _zp_send_join(_z_session_t *zn) { return _z_send_join(&zn->_tp); }

#if Z_MULTI_THREAD == 1
int8_t _zp_start_read_task(_z_session_t *zn) {
    int8_t ret = _Z_RES_OK;

    _z_task_t *task = (_z_task_t *)z_malloc(sizeof(_z_task_t));
    if (task != NULL) {
        (void)memset(task, 0, sizeof(_z_task_t));

#if Z_UNICAST_TRANSPORT == 1
        if (zn->_tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
            zn->_tp._transport._unicast._read_task = task;
            zn->_tp._transport._unicast._read_task_running = true;
            if (_z_task_init(task, NULL, _zp_unicast_read_task, &zn->_tp._transport._unicast) != _Z_RES_OK) {
                zn->_tp._transport._unicast._read_task_running = false;
                ret = _Z_ERR_SYSTEM_TASK_FAILED;
                z_free(task);
            }
        } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
            if (zn->_tp._type == _Z_TRANSPORT_MULTICAST_TYPE) {
            zn->_tp._transport._multicast._read_task = task;
            zn->_tp._transport._multicast._read_task_running = true;
            if (_z_task_init(task, NULL, _zp_multicast_read_task, &zn->_tp._transport._multicast) != _Z_RES_OK) {
                zn->_tp._transport._multicast._read_task_running = false;
                ret = _Z_ERR_SYSTEM_TASK_FAILED;
                z_free(task);
            }
        } else
#endif  // Z_MULTICAST_TRANSPORT == 1
        {
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            z_free(task);
        }
    }

    return ret;
}

int8_t _zp_stop_read_task(_z_session_t *zn) {
    int8_t ret = _Z_RES_OK;

#if Z_UNICAST_TRANSPORT == 1
    if (zn->_tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        zn->_tp._transport._unicast._read_task_running = false;
    } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
        if (zn->_tp._type == _Z_TRANSPORT_MULTICAST_TYPE) {
        zn->_tp._transport._multicast._read_task_running = false;
    } else
#endif  // Z_MULTICAST_TRANSPORT == 1
    {
        ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }

    return ret;
}

int8_t _zp_start_lease_task(_z_session_t *zn) {
    int8_t ret = _Z_RES_OK;

    _z_task_t *task = (_z_task_t *)z_malloc(sizeof(_z_task_t));
    if (task != NULL) {
        (void)memset(task, 0, sizeof(_z_task_t));

#if Z_UNICAST_TRANSPORT == 1
        if (zn->_tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
            zn->_tp._transport._unicast._lease_task = task;
            zn->_tp._transport._unicast._lease_task_running = true;
            if (_z_task_init(task, NULL, _zp_unicast_lease_task, &zn->_tp._transport._unicast) != _Z_RES_OK) {
                zn->_tp._transport._unicast._lease_task_running = false;
                ret = _Z_ERR_SYSTEM_TASK_FAILED;
                z_free(task);
            }
        } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
            if (zn->_tp._type == _Z_TRANSPORT_MULTICAST_TYPE) {
            zn->_tp._transport._multicast._lease_task = task;
            zn->_tp._transport._multicast._lease_task_running = true;
            if (_z_task_init(task, NULL, _zp_multicast_lease_task, &zn->_tp._transport._multicast) != _Z_RES_OK) {
                zn->_tp._transport._multicast._lease_task_running = false;
                ret = _Z_ERR_SYSTEM_TASK_FAILED;
                z_free(task);
            }
        } else
#endif  // Z_MULTICAST_TRANSPORT == 1
        {
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            z_free(task);
        }
    }

    return ret;
}

int8_t _zp_stop_lease_task(_z_session_t *zn) {
    int8_t ret = _Z_RES_OK;

#if Z_UNICAST_TRANSPORT == 1
    if (zn->_tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        zn->_tp._transport._unicast._lease_task_running = false;
    } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
        if (zn->_tp._type == _Z_TRANSPORT_MULTICAST_TYPE) {
        zn->_tp._transport._multicast._lease_task_running = false;
    } else
#endif  // Z_MULTICAST_TRANSPORT == 1
    {
        ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }

    return ret;
}
#endif  // Z_MULTI_THREAD == 1
