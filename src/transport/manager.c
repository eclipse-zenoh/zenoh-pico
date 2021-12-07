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
#include "zenoh-pico/session/session.h"

_zn_transport_p_result_t _zn_new_transport_client(z_str_t locator, z_bytes_t local_pid)
{
    _zn_transport_p_result_t ret;
    _zn_transport_t *zt = NULL;

    _zn_link_p_result_t res_zl = _zn_open_link(locator, 0);
    if (res_zl.tag == _z_res_t_ERR)
        goto ERR_1;

    if (res_zl.value.link->is_multicast == 0)
    {
        _zn_transport_unicast_establish_param_result_t res_tp_param = _zn_transport_unicast_open_client(res_zl.value.link, local_pid);
        if (res_tp_param.tag == _z_res_t_ERR)
            goto ERR_2;

        zt = _zn_transport_unicast_init();
        zt->transport.unicast.sn_resolution = res_tp_param.value.transport_unicast_establish_param.sn_resolution;
        zt->transport.unicast.sn_resolution_half = zt->transport.unicast.sn_resolution / 2;
        zt->transport.unicast.sn_tx_reliable = res_tp_param.value.transport_unicast_establish_param.initial_sn_tx;
        zt->transport.unicast.sn_tx_best_effort = res_tp_param.value.transport_unicast_establish_param.initial_sn_tx;
        zt->transport.unicast.sn_rx_reliable = res_tp_param.value.transport_unicast_establish_param.initial_sn_rx;
        zt->transport.unicast.sn_rx_best_effort = res_tp_param.value.transport_unicast_establish_param.initial_sn_rx;
        zt->transport.unicast.lease = res_tp_param.value.transport_unicast_establish_param.lease;
        zt->transport.unicast.remote_pid = res_tp_param.value.transport_unicast_establish_param.remote_pid;

        zt->transport.unicast.link = res_zl.value.link;
    }
    else
    {
        _zn_transport_multicast_establish_param_result_t res_tp_param = _zn_transport_multicast_open_client(res_zl.value.link, local_pid);
        if (res_tp_param.tag == _z_res_t_ERR)
            goto ERR_2;

        // TODO: not implemented
    }

    ret.tag = _z_res_t_OK;
    ret.value.transport = zt;
    return ret;

ERR_2:
    _zn_link_free(&res_zl.value.link);
ERR_1:
    ret.tag = _z_res_t_ERR;
    ret.value.error = -1;
    return ret;
}

_zn_transport_p_result_t _zn_new_transport_peer(z_str_t locator, z_bytes_t local_pid)
{
    _zn_transport_p_result_t ret;
    _zn_transport_t *zt = NULL;

    _zn_link_p_result_t res_zl = _zn_listen_link(locator, 0);
    if (res_zl.tag == _z_res_t_ERR)
        goto ERR_1;

    if (res_zl.value.link->is_multicast == 0)
    {
        _zn_transport_unicast_establish_param_result_t res_tp_param = _zn_transport_unicast_open_peer(res_zl.value.link, local_pid);
        if (res_tp_param.tag == _z_res_t_ERR)
            goto ERR_2;

        zt = _zn_transport_unicast_init();
        zt->transport.unicast.sn_resolution = res_tp_param.value.transport_unicast_establish_param.sn_resolution;
        zt->transport.unicast.sn_resolution_half = zt->transport.unicast.sn_resolution / 2;
        zt->transport.unicast.sn_tx_reliable = res_tp_param.value.transport_unicast_establish_param.initial_sn_tx;
        zt->transport.unicast.sn_tx_best_effort = res_tp_param.value.transport_unicast_establish_param.initial_sn_tx;
        zt->transport.unicast.sn_rx_reliable = res_tp_param.value.transport_unicast_establish_param.initial_sn_rx;
        zt->transport.unicast.sn_rx_best_effort = res_tp_param.value.transport_unicast_establish_param.initial_sn_rx;
        zt->transport.unicast.lease = res_tp_param.value.transport_unicast_establish_param.lease;
        zt->transport.unicast.remote_pid = res_tp_param.value.transport_unicast_establish_param.remote_pid;

        zt->transport.unicast.link = res_zl.value.link;
    }
    else
    {
        _zn_transport_multicast_establish_param_result_t res_tp_param = _zn_transport_multicast_open_peer(res_zl.value.link, local_pid);
        if (res_tp_param.tag == _z_res_t_ERR)
            goto ERR_2;

        zt = _zn_transport_multicast_init();
        zt->transport.multicast.sn_resolution = res_tp_param.value.transport_multicast_establish_param.sn_resolution;
        zt->transport.multicast.sn_resolution_half = zt->transport.multicast.sn_resolution / 2;
        zt->transport.multicast.sn_tx_reliable = res_tp_param.value.transport_multicast_establish_param.initial_sn_tx;
        zt->transport.multicast.sn_tx_best_effort = res_tp_param.value.transport_multicast_establish_param.initial_sn_tx;
        zt->transport.multicast.keep_alive = ZN_KEEP_ALIVE_INTERVAL;

        zt->transport.multicast.link = res_zl.value.link;
    }

    ret.tag = _z_res_t_OK;
    ret.value.transport = zt;
    return ret;

ERR_2:
    _zn_link_free(&res_zl.value.link);
ERR_1:
    ret.tag = _z_res_t_ERR;
    ret.value.error = -1;
    return ret;
}

_zn_transport_manager_t *_zn_transport_manager_init()
{
    _zn_transport_manager_t *ztm = (_zn_transport_manager_t *)malloc(sizeof(_zn_transport_manager_t));

    // Randomly generate a peer ID
    srand(time(NULL));
    ztm->local_pid = _z_bytes_make(ZN_PID_LENGTH);
    for (unsigned int i = 0; i < ztm->local_pid.len; i++)
        ((uint8_t *)ztm->local_pid.val)[i] = rand() % 255;

    ztm->link_manager = _zn_link_manager_init();

    return ztm;
}

void _zn_transport_manager_free(_zn_transport_manager_t **ztm)
{
    _zn_transport_manager_t *ptr = *ztm;

    // Clean up PIDs
    _z_bytes_clear(&ptr->local_pid);

    // Clean up managers
    _zn_link_manager_free(&ptr->link_manager);

    free(ptr);
    *ztm = NULL;
}

_zn_transport_p_result_t _zn_new_transport(_zn_transport_manager_t *ztm, z_str_t locator, uint8_t mode)
{
    _zn_transport_p_result_t ret;
    if (mode == 0) // FIXME: use enum
        ret = _zn_new_transport_client(locator, ztm->local_pid);
    else
        ret = _zn_new_transport_peer(locator, ztm->local_pid);

    return ret;
}
