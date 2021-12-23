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
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/transport/link/rx.h"
#include "zenoh-pico/transport/link/tx.h"
#include "zenoh-pico/utils/logging.h"

int _zn_unicast_send_close(_zn_transport_unicast_t *ztu, uint8_t reason, int link_only)
{
    z_bytes_t pid = _z_bytes_duplicate(&((zn_session_t *)ztu->session)->tp_manager->local_pid);
    _zn_transport_message_t cm = _zn_t_msg_make_close(reason, pid, link_only);

    int res = _zn_unicast_send_t_msg(ztu, &cm);

    // Free the message
    _zn_t_msg_clear(&cm);

    return res;
}

int _zn_multicast_send_close(_zn_transport_multicast_t *ztm, uint8_t reason, int link_only)
{
    z_bytes_t pid = _z_bytes_duplicate(&((zn_session_t *)ztm->session)->tp_manager->local_pid);
    _zn_transport_message_t cm = _zn_t_msg_make_close(reason, pid, link_only);

    int res = _zn_multicast_send_t_msg(ztm, &cm);

    // Free the message
    _zn_t_msg_clear(&cm);

    return res;
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

_zn_transport_t *_zn_transport_unicast_new(_zn_link_t *link, _zn_transport_unicast_establish_param_t param)
{
    _zn_transport_t *zt = (_zn_transport_t *)malloc(sizeof(_zn_transport_t));
    zt->type = _ZN_TRANSPORT_UNICAST_TYPE;

    // Initialize the mutexes
    z_mutex_init(&zt->transport.unicast.mutex_tx);
    z_mutex_init(&zt->transport.unicast.mutex_rx);

    // Initialize the read and write buffers
    uint16_t mtu = link->mtu < ZN_BATCH_SIZE ? link->mtu : ZN_BATCH_SIZE;
    zt->transport.unicast.wbuf = _z_wbuf_make(mtu, 0);
    zt->transport.unicast.zbuf = _z_zbuf_make(ZN_BATCH_SIZE);
    // Initialize the defragmentation buffers
    zt->transport.unicast.dbuf_reliable = _z_wbuf_make(0, 1);
    zt->transport.unicast.dbuf_best_effort = _z_wbuf_make(0, 1);

    // Set default SN resolution
    zt->transport.unicast.sn_resolution = param.sn_resolution;
    zt->transport.unicast.sn_resolution_half = param.sn_resolution / 2;

    // The initial SN at TX side
    zt->transport.unicast.sn_tx_reliable = param.initial_sn_tx;
    zt->transport.unicast.sn_tx_best_effort = param.initial_sn_tx;

    // The initial SN at RX side
    zt->transport.unicast.sn_rx_reliable = param.initial_sn_rx;
    zt->transport.unicast.sn_rx_best_effort = param.initial_sn_rx;

    // Tasks
    zt->transport.unicast.read_task_running = 0;
    zt->transport.unicast.read_task = NULL;
    zt->transport.unicast.lease_task_running = 0;
    zt->transport.unicast.lease_task = NULL;

    // Notifiers
    zt->transport.unicast.received = 0;
    zt->transport.unicast.transmitted = 0;

    // Remote peer PID
    _z_bytes_move(&zt->transport.unicast.remote_pid, &param.remote_pid);

    // Transport lease
    zt->transport.unicast.lease = param.lease;

    // Transport link for unicast
    zt->transport.unicast.link = link;

    return zt;
}

_zn_transport_t *_zn_transport_multicast_new(_zn_link_t *link, _zn_transport_multicast_establish_param_t param)
{
    _zn_transport_t *zt = (_zn_transport_t *)malloc(sizeof(_zn_transport_t));
    zt->type = _ZN_TRANSPORT_MULTICAST_TYPE;

    // Initialize the mutexes
    z_mutex_init(&zt->transport.multicast.mutex_tx);
    z_mutex_init(&zt->transport.multicast.mutex_rx);
    z_mutex_init(&zt->transport.multicast.mutex_peer);

    // Initialize the read and write buffers
    uint16_t mtu = link->mtu < ZN_BATCH_SIZE ? link->mtu : ZN_BATCH_SIZE;
    zt->transport.multicast.wbuf = _z_wbuf_make(mtu, 0);
    zt->transport.multicast.zbuf = _z_zbuf_make(ZN_BATCH_SIZE);

    // Set default SN resolution
    zt->transport.multicast.sn_resolution = param.sn_resolution;
    zt->transport.multicast.sn_resolution_half = param.sn_resolution / 2;
    // The initial SN at TX side
    zt->transport.multicast.sn_tx_reliable = param.initial_sn_tx;
    zt->transport.multicast.sn_tx_best_effort = param.initial_sn_tx;

    // Initialize peer list
    zt->transport.multicast.peers = _zn_transport_peer_entry_list_new();

    // Tasks
    zt->transport.multicast.read_task_running = 0;
    zt->transport.multicast.read_task = NULL;
    zt->transport.multicast.lease_task_running = 0;
    zt->transport.multicast.lease_task = NULL;
    zt->transport.multicast.keep_alive = ZN_KEEP_ALIVE_INTERVAL;

    // Notifiers
    zt->transport.multicast.transmitted = 0;

    // Transport link for unicast
    zt->transport.multicast.link = link;

    return zt;
}

_zn_transport_unicast_establish_param_result_t _zn_transport_unicast_open_client(const _zn_link_t *zl, const z_bytes_t local_pid)
{
    _zn_transport_unicast_establish_param_result_t ret;
    _zn_transport_unicast_establish_param_t param;

    // Build the open message
    uint8_t version = ZN_PROTO_VERSION;
    z_zint_t whatami = ZN_CLIENT;
    z_zint_t sn_resolution = ZN_SN_RESOLUTION;
    int is_qos = 0;

    z_bytes_t pid = _z_bytes_duplicate(&local_pid);
    _zn_transport_message_t ism = _zn_t_msg_make_init_syn(version, whatami, sn_resolution, pid, is_qos);

    // Encode and send the message
    _Z_DEBUG("Sending InitSyn\n");
    int res = _zn_link_send_t_msg(zl, &ism);
    if (res != 0)
        goto ERR_1;

    // The announced sn resolution
    param.sn_resolution = ism.body.init.sn_resolution;
    _zn_t_msg_clear(&ism);

    _zn_transport_message_result_t r_iam = _zn_link_recv_t_msg(zl);
    if (r_iam.tag == _z_res_t_ERR)
        goto ERR_1;

    _zn_transport_message_t iam = r_iam.value.transport_message;
    switch (_ZN_MID(iam.header))
    {
    case _ZN_MID_INIT:
    {
        if _ZN_HAS_FLAG (iam.header, _ZN_FLAG_T_A)
        {
            // Handle SN resolution option if present
            if _ZN_HAS_FLAG (iam.header, _ZN_FLAG_T_S)
            {
                // The resolution in the InitAck must be less or equal than the resolution in the InitSyn,
                // otherwise the InitAck message is considered invalid and it should be treated as a
                // CLOSE message with L==0 by the Initiating Peer -- the recipient of the InitAck message.
                if (iam.body.init.sn_resolution <= param.sn_resolution)
                    param.sn_resolution = iam.body.init.sn_resolution;
                else
                    goto ERR_2;
            }

            // The initial SN at TX side
            param.initial_sn_tx = (z_zint_t)rand() % param.sn_resolution;

            // Initialize the Local and Remote Peer IDs
            _z_bytes_copy(&param.remote_pid, &iam.body.init.pid);

            // Create the OpenSyn message
            z_zint_t lease = ZN_TRANSPORT_LEASE;
            z_zint_t initial_sn = param.initial_sn_tx;
            z_bytes_t cookie = iam.body.init.cookie;

            _zn_transport_message_t osm = _zn_t_msg_make_open_syn(lease, initial_sn, cookie);

            // Encode and send the message
            _Z_DEBUG("Sending OpenSyn\n");
            res = _zn_link_send_t_msg(zl, &osm);

            if (res != 0)
                goto ERR_3;

            _zn_transport_message_result_t r_oam = _zn_link_recv_t_msg(zl);
            if (r_oam.tag == _z_res_t_ERR)
                goto ERR_3;
            _zn_transport_message_t oam = r_oam.value.transport_message;

            if (_ZN_HAS_FLAG(oam.header, _ZN_FLAG_T_A))
            {
                // The session lease
                param.lease = oam.body.open.lease;

                // The initial SN at RX side. Initialize the session as we had already received
                // a message with a SN equal to initial_sn - 1.
                param.initial_sn_rx = _zn_sn_decrement(param.sn_resolution, oam.body.open.initial_sn);
            }
            else
                goto ERR_3;

            _zn_t_msg_clear(&oam);

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

    _zn_t_msg_clear(&iam);

    ret.tag = _z_res_t_OK;
    ret.value.transport_unicast_establish_param = param;
    return ret;

ERR_3:
    _z_bytes_clear(&param.remote_pid);
ERR_2:
    _zn_t_msg_clear(&iam);
ERR_1:
    ret.tag = _z_res_t_ERR;
    ret.value.error = -1;
    return ret;
}

_zn_transport_multicast_establish_param_result_t _zn_transport_multicast_open_client(const _zn_link_t *zl, const z_bytes_t local_pid)
{
    _zn_transport_multicast_establish_param_result_t ret;
    ret.tag = _z_res_t_ERR;
    ret.value.error = -1;

    // @TODO: not implemented

    return ret;
}

_zn_transport_unicast_establish_param_result_t _zn_transport_unicast_open_peer(const _zn_link_t *zl, const z_bytes_t local_pid)
{
    _zn_transport_unicast_establish_param_result_t ret;
    ret.tag = _z_res_t_ERR;
    ret.value.error = -1;

    // @TODO: not implemented

    return ret;
}

_zn_transport_multicast_establish_param_result_t _zn_transport_multicast_open_peer(const _zn_link_t *zl, const z_bytes_t local_pid)
{
    _zn_transport_multicast_establish_param_result_t ret;
    _zn_transport_multicast_establish_param_t param;
    param.is_qos = 0; // FIXME: make transport aware of qos configuration
    param.initial_sn_tx = 0;
    param.sn_resolution = ZN_SN_RESOLUTION;

    // Explicitly send a JOIN message upon startup
    // FIXME: make transport aware of qos configuration
    _zn_conduit_sn_list_t next_sns;
    next_sns.is_qos = 0;
    next_sns.val.plain.best_effort = param.initial_sn_tx;
    next_sns.val.plain.reliable = param.initial_sn_tx;

    z_bytes_t pid = _z_bytes_duplicate(&local_pid);
    _zn_transport_message_t jsm = _zn_t_msg_make_join(ZN_PROTO_VERSION, ZN_PEER, ZN_TRANSPORT_LEASE, param.sn_resolution, pid, next_sns);

    // Encode and send the message
    _Z_DEBUG("Sending Join\n");
    int res = _zn_link_send_t_msg(zl, &jsm);

    _zn_t_msg_clear(&jsm);

    if (res != 0)
        goto ERR_1;

    ret.tag = _z_res_t_OK;
    ret.value.transport_multicast_establish_param = param;
    return ret;

ERR_1:
    ret.tag = _z_res_t_ERR;
    ret.value.error = -1;
    return ret;
}

int _zn_transport_unicast_close(_zn_transport_unicast_t *ztu, uint8_t reason)
{
    return _zn_unicast_send_close(ztu, reason, 0);
}

int _zn_transport_multicast_close(_zn_transport_multicast_t *ztm, uint8_t reason)
{
    return _zn_multicast_send_close(ztm, reason, 0);
}

int _zn_transport_close(_zn_transport_t *zt, uint8_t reason)
{
    return _zn_send_close(zt, reason, 0);
}

void _zn_transport_unicast_clear(_zn_transport_unicast_t *ztu)
{
    // Clean up tasks
    z_task_join(ztu->read_task);
    z_task_join(ztu->lease_task);
    z_task_free(&ztu->read_task);
    z_task_free(&ztu->lease_task);

    // Clean up the mutexes
    z_mutex_free(&ztu->mutex_tx);
    z_mutex_free(&ztu->mutex_rx);

    // Clean up the buffers
    _z_wbuf_clear(&ztu->wbuf);
    _z_zbuf_clear(&ztu->zbuf);
    _z_wbuf_clear(&ztu->dbuf_reliable);
    _z_wbuf_clear(&ztu->dbuf_best_effort);

    // Clean up PIDs
    _z_bytes_clear(&ztu->remote_pid);

    if (ztu->link != NULL)
        _zn_link_free((_zn_link_t **)&ztu->link);
}

void _zn_transport_multicast_clear(_zn_transport_multicast_t *ztm)
{
    // Clean up tasks
    z_task_join(ztm->read_task);
    z_task_join(ztm->lease_task);
    z_task_free(&ztm->read_task);
    z_task_free(&ztm->lease_task);

    // Clean up the mutexes
    z_mutex_free(&ztm->mutex_tx);
    z_mutex_free(&ztm->mutex_rx);
    z_mutex_free(&ztm->mutex_peer);

    // Clean up the buffers
    _z_wbuf_clear(&ztm->wbuf);
    _z_zbuf_clear(&ztm->zbuf);

    // Clean up peer list
    _zn_transport_peer_entry_list_free(&ztm->peers);

    if (ztm->link != NULL)
        _zn_link_free((_zn_link_t **)&ztm->link);
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
