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
    _z_transport_p_result_t ret;
    _z_transport_t *zt = NULL;

    _z_link_p_result_t res_zl = _z_open_link(locator);
    if (res_zl._tag == _Z_RES_ERR) goto ERR_1;

#if Z_UNICAST_TRANSPORT == 1
    if (!_Z_LINK_IS_MULTICAST(res_zl._value._link->_capabilities)) {
        _z_transport_unicast_establish_param_result_t res_tp_param =
            _z_transport_unicast_open_client(res_zl._value._link, local_pid);
        if (res_tp_param._tag == _Z_RES_ERR) goto ERR_2;

        zt = _z_transport_unicast_new(res_zl._value._link, res_tp_param._value._transport_unicast_establish_param);
    } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
        if (_Z_LINK_IS_MULTICAST(res_zl._value._link->_capabilities)) {
        _z_transport_multicast_establish_param_result_t res_tp_param =
            _z_transport_multicast_open_client(res_zl._value._link, local_pid);
        if (res_tp_param._tag == _Z_RES_ERR) goto ERR_2;

        zt = _z_transport_multicast_new(res_zl._value._link, res_tp_param._value._transport_multicast_establish_param);
    } else
#endif  // Z_MULTICAST_TRANSPORT == 1
        __asm__("nop");

    ret._tag = _Z_RES_OK;
    ret._value._transport = zt;
    return ret;

ERR_2:
    _z_link_free(&res_zl._value._link);
ERR_1:
    ret._tag = _Z_RES_ERR;
    ret._value._error = -1;
    return ret;
}

_z_transport_p_result_t _z_new_transport_peer(char *locator, _z_bytes_t local_pid) {
    _z_transport_p_result_t ret;
    _z_transport_t *zt = NULL;

    _z_link_p_result_t res_zl = _z_listen_link(locator);
    if (res_zl._tag == _Z_RES_ERR) goto ERR_1;

#if Z_UNICAST_TRANSPORT == 1
    if (!_Z_LINK_IS_MULTICAST(res_zl._value._link->_capabilities)) {
        _z_transport_unicast_establish_param_result_t res_tp_param =
            _z_transport_unicast_open_peer(res_zl._value._link, local_pid);
        if (res_tp_param._tag == _Z_RES_ERR) goto ERR_2;

        zt = _z_transport_unicast_new(res_zl._value._link, res_tp_param._value._transport_unicast_establish_param);
    } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
        if (_Z_LINK_IS_MULTICAST(res_zl._value._link->_capabilities)) {
        _z_transport_multicast_establish_param_result_t res_tp_param =
            _z_transport_multicast_open_peer(res_zl._value._link, local_pid);
        if (res_tp_param._tag == _Z_RES_ERR) goto ERR_2;

        zt = _z_transport_multicast_new(res_zl._value._link, res_tp_param._value._transport_multicast_establish_param);
    } else
#endif  // Z_MULTICAST_TRANSPORT == 1
        __asm__("nop");

    ret._tag = _Z_RES_OK;
    ret._value._transport = zt;
    return ret;

ERR_2:
    _z_link_free(&res_zl._value._link);
ERR_1:
    ret._tag = _Z_RES_ERR;
    ret._value._error = -1;
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

_z_transport_p_result_t _z_new_transport(_z_transport_manager_t *ztm, char *locator, uint8_t mode) {
    _z_transport_p_result_t ret;
    if (mode == 0)  // FIXME: use enum
        ret = _z_new_transport_client(locator, ztm->_local_pid);
    else
        ret = _z_new_transport_peer(locator, ztm->_local_pid);

    return ret;
}
