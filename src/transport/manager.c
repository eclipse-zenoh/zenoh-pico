/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include "zenoh-pico/transport/manager.h"

_z_transport_p_result_t _z_new_transport_client(_z_str_t locator, _z_bytes_t local_pid)
{
    _z_transport_p_result_t ret;
    _z_transport_t *zt = NULL;

    _z_link_p_result_t res_zl = _z_open_link(locator);
    if (res_zl.tag == _Z_RES_ERR)
        goto ERR_1;

    if (res_zl.value.link->is_multicast == 0)
    {
        _z_transport_unicast_establish_param_result_t res_tp_param = _z_transport_unicast_open_client(res_zl.value.link, local_pid);
        if (res_tp_param.tag == _Z_RES_ERR)
            goto ERR_2;

        zt = _z_transport_unicast_new(res_zl.value.link, res_tp_param.value.transport_unicast_establish_param);
    }
    else
    {
        _z_transport_multicast_establish_param_result_t res_tp_param = _z_transport_multicast_open_client(res_zl.value.link, local_pid);
        if (res_tp_param.tag == _Z_RES_ERR)
            goto ERR_2;

        // @TODO: not implemented
    }

    ret.tag = _Z_RES_OK;
    ret.value.transport = zt;
    return ret;

ERR_2:
    _z_link_free(&res_zl.value.link);
ERR_1:
    ret.tag = _Z_RES_ERR;
    ret.value.error = -1;
    return ret;
}

_z_transport_p_result_t _z_new_transport_peer(_z_str_t locator, _z_bytes_t local_pid)
{
    _z_transport_p_result_t ret;
    _z_transport_t *zt = NULL;

    _z_link_p_result_t res_zl = _z_listen_link(locator);
    if (res_zl.tag == _Z_RES_ERR)
        goto ERR_1;

    if (res_zl.value.link->is_multicast == 0)
    {
        _z_transport_unicast_establish_param_result_t res_tp_param = _z_transport_unicast_open_peer(res_zl.value.link, local_pid);
        if (res_tp_param.tag == _Z_RES_ERR)
            goto ERR_2;

        zt = _z_transport_unicast_new(res_zl.value.link, res_tp_param.value.transport_unicast_establish_param);
    }
    else
    {
        _z_transport_multicast_establish_param_result_t res_tp_param = _z_transport_multicast_open_peer(res_zl.value.link, local_pid);
        if (res_tp_param.tag == _Z_RES_ERR)
            goto ERR_2;

        zt = _z_transport_multicast_new(res_zl.value.link, res_tp_param.value.transport_multicast_establish_param);
    }

    ret.tag = _Z_RES_OK;
    ret.value.transport = zt;
    return ret;

ERR_2:
    _z_link_free(&res_zl.value.link);
ERR_1:
    ret.tag = _Z_RES_ERR;
    ret.value.error = -1;
    return ret;
}

_z_transport_manager_t *_z_transport_manager_init()
{
    _z_transport_manager_t *ztm = (_z_transport_manager_t *)malloc(sizeof(_z_transport_manager_t));

    // Randomly generate a peer ID
    srand(time(NULL));
    ztm->local_pid = _z_bytes_make(Z_PID_LENGTH);
    for (unsigned int i = 0; i < ztm->local_pid.len; i++)
        ((uint8_t *)ztm->local_pid.val)[i] = rand() % 255;

    ztm->link_manager = _z_link_manager_init();

    return ztm;
}

void _z_transport_manager_free(_z_transport_manager_t **ztm)
{
    _z_transport_manager_t *ptr = *ztm;

    // Clean up PIDs
    _z_bytes_clear(&ptr->local_pid);

    // Clean up managers
    _z_link_manager_free(&ptr->link_manager);

    free(ptr);
    *ztm = NULL;
}

_z_transport_p_result_t _z_new_transport(_z_transport_manager_t *ztm, _z_str_t locator, uint8_t mode)
{
    _z_transport_p_result_t ret;
    if (mode == 0) // FIXME: use enum
        ret = _z_new_transport_client(locator, ztm->local_pid);
    else
        ret = _z_new_transport_peer(locator, ztm->local_pid);

    return ret;
}
