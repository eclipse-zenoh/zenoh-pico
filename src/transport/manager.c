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

_z_transport_p_result_t _z_new_transport_client(char *locator, _z_bytes_t local_pid) {
    _z_transport_p_result_t ret = {._tag = _Z_RES_OK, ._value = NULL};

    _z_link_p_result_t res_zl = _z_open_link(locator);
    if (res_zl._tag == _Z_RES_OK) {
#if Z_UNICAST_TRANSPORT == 1
        if (_Z_LINK_IS_MULTICAST(res_zl._value->_capabilities) == false) {
            _z_transport_unicast_establish_param_result_t res_tp_param =
                _z_transport_unicast_open_client(res_zl._value, local_pid);
            if (res_tp_param._tag == _Z_RES_OK) {
                ret._value = _z_transport_unicast_new(res_zl._value, res_tp_param._value);
            } else {
                _z_link_free(&res_zl._value);
                ret._tag = res_tp_param._tag;
            }
        } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
            if (_Z_LINK_IS_MULTICAST(res_zl._value->_capabilities) == true) {
            _z_transport_multicast_establish_param_result_t res_tp_param =
                _z_transport_multicast_open_client(res_zl._value, local_pid);
            if (res_tp_param._tag == _Z_RES_OK) {
                ret._value = _z_transport_multicast_new(res_zl._value, res_tp_param._value);
            } else {
                _z_link_free(&res_zl._value);
                ret._tag = res_tp_param._tag;
            }
        } else
#endif  // Z_MULTICAST_TRANSPORT == 1
        {
            _z_link_free(&res_zl._value);
            ret._tag = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
        }
    } else {
        ret._tag = res_zl._tag;
    }

    return ret;
}

_z_transport_p_result_t _z_new_transport_peer(char *locator, _z_bytes_t local_pid) {
    _z_transport_p_result_t ret = {._tag = _Z_RES_OK, ._value = NULL};

    _z_link_p_result_t res_zl = _z_listen_link(locator);
    if (res_zl._tag == _Z_RES_OK) {
#if Z_UNICAST_TRANSPORT == 1
        if (_Z_LINK_IS_MULTICAST(res_zl._value->_capabilities) == false) {
            _z_transport_unicast_establish_param_result_t res_tp_param =
                _z_transport_unicast_open_peer(res_zl._value, local_pid);
            if (res_tp_param._tag == _Z_RES_OK) {
                ret._value = _z_transport_unicast_new(res_zl._value, res_tp_param._value);
            } else {
                _z_link_free(&res_zl._value);
                ret._tag = res_tp_param._tag;
            }
        } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
            if (_Z_LINK_IS_MULTICAST(res_zl._value->_capabilities) == true) {
            _z_transport_multicast_establish_param_result_t res_tp_param =
                _z_transport_multicast_open_peer(res_zl._value, local_pid);
            if (res_tp_param._tag == _Z_RES_OK) {
                ret._value = _z_transport_multicast_new(res_zl._value, res_tp_param._value);
            } else {
                _z_link_free(&res_zl._value);
                ret._tag = res_tp_param._tag;
            }
        } else
#endif  // Z_MULTICAST_TRANSPORT == 1
        {
            _z_link_free(&res_zl._value);
            ret._tag = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
        }
    } else {
        ret._tag = res_zl._tag;
    }

    return ret;
}

_z_transport_manager_t *_z_transport_manager_init() {
    _z_transport_manager_t *ztm = (_z_transport_manager_t *)z_malloc(sizeof(_z_transport_manager_t));

    // Randomly generate a peer ID
    ztm->_local_pid = _z_bytes_make(Z_ZID_LENGTH);
    z_random_fill((uint8_t *)ztm->_local_pid.start, ztm->_local_pid.len);

    ztm->_link_manager = _z_link_manager_init();

    return ztm;
}

void _z_transport_manager_free(_z_transport_manager_t **ztm) {
    _z_transport_manager_t *ptr = *ztm;

    // Clean up PIDs
    _z_bytes_clear(&ptr->_local_pid);

    // Clean up managers
    _z_link_manager_free(&ptr->_link_manager);

    z_free(ptr);
    *ztm = NULL;
}

_z_transport_p_result_t _z_new_transport(_z_transport_manager_t *ztm, char *locator, z_whatami_t mode) {
    _z_transport_p_result_t ret;

    if (mode == Z_WHATAMI_CLIENT) {
        ret = _z_new_transport_client(locator, ztm->_local_pid);
    } else {
        ret = _z_new_transport_peer(locator, ztm->_local_pid);
    }

    return ret;
}
