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

#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/transport/link/rx.h"
#include "zenoh-pico/transport/link/tx.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/utils/logging.h"

int _zn_unicast_send_close(_zn_transport_unicast_t *ztu, uint8_t reason, int link_only)
{
    _zn_transport_message_t cm = _zn_transport_message_init(_ZN_MID_CLOSE);

    cm.body.close.pid = ((zn_session_t*)ztu->session)->tp_manager->local_pid;
    cm.body.close.reason = reason;
    _ZN_SET_FLAG(cm.header, _ZN_FLAG_T_I);
    if (link_only)
        _ZN_SET_FLAG(cm.header, _ZN_FLAG_T_K);

    int res = _zn_unicast_send_t_msg(ztu, &cm);

    // Free the message
    _zn_transport_message_free(&cm);

    return res;
}

int _zn_multicast_send_close(_zn_transport_multicast_t *ztm, uint8_t reason, int link_only)
{
    // TODO: to be implemented

    return -1;
}

int _zn_send_close(_zn_transport_t *zt, uint8_t reason, int link_only)
{
    if (zt->type == _ZN_TRANSPORT_UNICAST_TYPE)
        return _zn_unicast_send_close(&zt->transport.unicast, reason, link_only);
    else if (zt->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        return _zn_multicast_send_close(&zt->transport.multicast, reason, link_only);
    else
        return -1;
}

_zn_transport_t *_zn_transport_unicast_init()
{
    _zn_transport_t *zt = (_zn_transport_t *)malloc(sizeof(_zn_transport_t));
    zt->type = _ZN_TRANSPORT_UNICAST_TYPE;

    // Initialize the mutexes
    z_mutex_init(&zt->transport.unicast.mutex_tx);
    z_mutex_init(&zt->transport.unicast.mutex_rx);

    // Initialize the read and write buffers
    zt->transport.unicast.wbuf = _z_wbuf_make(ZN_WRITE_BUF_LEN, 0);
    zt->transport.unicast.zbuf = _z_zbuf_make(ZN_READ_BUF_LEN);
    // Initialize the defragmentation buffers
    zt->transport.unicast.dbuf_reliable = _z_wbuf_make(0, 1);
    zt->transport.unicast.dbuf_best_effort = _z_wbuf_make(0, 1);

    // Set default SN resolution
    zt->transport.unicast.sn_resolution = 0;
    zt->transport.unicast.sn_resolution_half = zt->transport.unicast.sn_resolution / 2;

    // The initial SN at TX side
    zt->transport.unicast.sn_tx_reliable = 0;
    zt->transport.unicast.sn_tx_best_effort = 0;

    // The initial SN at RX side
    zt->transport.unicast.sn_rx_reliable = 0;
    zt->transport.unicast.sn_rx_best_effort = 0;

    // Tasks
    zt->transport.unicast.read_task_running = 0;
    zt->transport.unicast.read_task = NULL;
    zt->transport.unicast.lease_task_running = 0;
    zt->transport.unicast.lease_task = NULL;

    // Notifiers
    zt->transport.unicast.received = 0;
    zt->transport.unicast.transmitted = 0;

    // Remote peer PID
    //zt->transport.unicast.remote_pid;

    // Transport link for unicast
    zt->transport.unicast.link = NULL;

    return zt;
}

_zn_transport_establish_param_result_t _zn_transport_unicast_open_client(const _zn_link_t *zl, const z_bytes_t local_pid)
{
    _zn_transport_establish_param_result_t ret;
    _zn_transport_establish_param_t param;

    // Build the open message
    _zn_transport_message_t ism = _zn_transport_message_init(_ZN_MID_INIT);

    ism.body.init.options = 0;
    ism.body.init.version = ZN_PROTO_VERSION;
    ism.body.init.whatami = ZN_CLIENT;
    ism.body.init.pid = local_pid;
    ism.body.init.sn_resolution = ZN_SN_RESOLUTION;

    if (ZN_SN_RESOLUTION != ZN_SN_RESOLUTION_DEFAULT)
        _ZN_SET_FLAG(ism.header, _ZN_FLAG_T_S);

    // Encode and send the message
    _Z_DEBUG("Sending InitSyn\n");
    int res = _zn_send_t_msg_nt(zl, &ism);
    if (res != 0)
        goto ERR_1;
    
    // The announced sn resolution
    param.sn_resolution = ism.body.init.sn_resolution;
    _zn_transport_message_free(&ism);

    _zn_transport_message_p_result_t r_iam = _zn_recv_t_msg_nt(zl);
    if (r_iam.tag == _z_res_t_ERR)
        goto ERR_1;

    _zn_transport_message_t *p_iam = r_iam.value.transport_message;
    switch (_ZN_MID(p_iam->header))
    {
    case _ZN_MID_INIT:
    {
        if _ZN_HAS_FLAG (p_iam->header, _ZN_FLAG_T_A)
        {
            // Handle SN resolution option if present
            if _ZN_HAS_FLAG (p_iam->header, _ZN_FLAG_T_S)
            {
                // The resolution in the InitAck must be less or equal than the resolution in the InitSyn,
                // otherwise the InitAck message is considered invalid and it should be treated as a
                // CLOSE message with L==0 by the Initiating Peer -- the recipient of the InitAck message.
                if (p_iam->body.init.sn_resolution <= param.sn_resolution)
                    param.sn_resolution = p_iam->body.init.sn_resolution;
                else
                    goto ERR_2;
            }

            // The initial SN at TX side
            param.initial_sn_tx = (z_zint_t)rand() % param.sn_resolution;

            // Initialize the Local and Remote Peer IDs
            _z_bytes_copy(&param.remote_pid, &p_iam->body.init.pid);

            // Create the OpenSyn message
            _zn_transport_message_t osm = _zn_transport_message_init(_ZN_MID_OPEN);
            osm.body.open.lease = ZN_TRANSPORT_LEASE;
            if (ZN_TRANSPORT_LEASE % 1000 == 0)
                _ZN_SET_FLAG(osm.header, _ZN_FLAG_T_T2);
            osm.body.open.initial_sn = param.initial_sn_tx;
            osm.body.open.cookie = p_iam->body.init.cookie;

            // Encode and send the message
            _Z_DEBUG("Sending OpenSyn\n");
            res = _zn_send_t_msg_nt(zl, &osm);
            
            if (res != 0)
                goto ERR_3;

            _zn_transport_message_p_result_t r_oam = _zn_recv_t_msg_nt(zl);
            if (r_oam.tag == _z_res_t_ERR)
                goto ERR_3;

            // TODO: decode openack

            _zn_transport_message_p_result_free(&r_oam);

            break;
        }
        else
            goto ERR_2;
    }

    default:
    {
        goto ERR_2;
    }
    }

    _zn_transport_message_free(p_iam);
    _zn_transport_message_p_result_free(&r_iam);

    ret.tag = _z_res_t_OK;
    ret.value.transport_establish_param = param;
    return ret;

ERR_3:
    _z_bytes_free(&param.remote_pid);
ERR_2:
    _zn_transport_message_free(p_iam);
    _zn_transport_message_p_result_free(&r_iam);
ERR_1:
    ret.tag = _z_res_t_ERR;
    ret.value.error = -1;
    return ret;
}

_zn_transport_establish_param_result_t _zn_transport_multicast_open_client(const _zn_link_t *zt, const z_bytes_t local_pid)
{
    _zn_transport_establish_param_result_t ret;
    ret.tag = _z_res_t_ERR;
    ret.value.error = -1;

    // TODO: not implemented

    return ret;
}

_zn_transport_establish_param_result_t _zn_transport_unicast_open_peer(const _zn_link_t *zt, const z_bytes_t local_pid)
{
    _zn_transport_establish_param_result_t ret;
    ret.tag = _z_res_t_ERR;
    ret.value.error = -1;

    // TODO: not implemented

    return ret;
}

_zn_transport_establish_param_result_t _zn_transport_multicast_open_peer(const _zn_link_t *zt, const z_bytes_t local_pid)
{
    _zn_transport_establish_param_result_t ret;
    ret.tag = _z_res_t_ERR;
    ret.value.error = -1;

    // TODO: not implemented

    return ret;
}

int _zn_transport_unicast_close(_zn_transport_unicast_t *ztu, uint8_t reason)
{
    return _zn_unicast_send_close(ztu, reason, 0);
}

int _zn_transport_multicast_close(_zn_transport_multicast_t *ztm, uint8_t reason)
{
    // TODO: to be implemented
    return -1;
}

int _zn_transport_close(_zn_transport_t *zt, uint8_t reason)
{
    return _zn_send_close(zt, reason, 0);
}

void _zn_transport_unicast_clear(_zn_transport_unicast_t *ztu)
{
    // Clean up the mutexes
    z_mutex_free(&ztu->mutex_tx);
    z_mutex_free(&ztu->mutex_rx);

    // Clean up the buffers
    _z_wbuf_free(&ztu->wbuf);
    _z_zbuf_free(&ztu->zbuf);
    _z_wbuf_free(&ztu->dbuf_reliable);
    _z_wbuf_free(&ztu->dbuf_best_effort);

    // Clean up PIDs
    _z_bytes_free(&ztu->remote_pid);

    if (ztu->link != NULL)
        _zn_link_free((_zn_link_t **)&ztu->link);
}

void _zn_transport_multicast_clear(_zn_transport_multicast_t *ztm)
{
    // TODO: to be implemented
}

void _zn_transport_free(_zn_transport_t **zt)
{
    _zn_transport_t *ptr = *zt;

    if (ptr->type == _ZN_TRANSPORT_UNICAST_TYPE)
        _zn_transport_unicast_clear(&ptr->transport.unicast);
    else if (ptr->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        _zn_transport_multicast_clear(&ptr->transport.multicast);

    free(ptr);
    *zt = NULL;
}
