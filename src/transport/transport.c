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

#include <stdlib.h>
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/transport/link/rx.h"
#include "zenoh-pico/transport/link/tx.h"
#include "zenoh-pico/utils/logging.h"

int _z_unicast_send_close(_z_transport_unicast_t *ztu, uint8_t reason, int link_only)
{
    _z_bytes_t pid = _z_bytes_wrap(((_z_session_t *)ztu->_session)->_tp_manager->_local_pid.start, ((_z_session_t *)ztu->_session)->_tp_manager->_local_pid.len);
    _z_transport_message_t cm = _z_t_msg_make_close(reason, pid, link_only);

    int res = _z_unicast_send_t_msg(ztu, &cm);

    // Free the message
    _z_t_msg_clear(&cm);

    return res;
}

int _z_multicast_send_close(_z_transport_multicast_t *ztm, uint8_t reason, int link_only)
{
    _z_bytes_t pid = _z_bytes_wrap(((_z_session_t *)ztm->_session)->_tp_manager->_local_pid.start, ((_z_session_t *)ztm->_session)->_tp_manager->_local_pid.len);
    _z_transport_message_t cm = _z_t_msg_make_close(reason, pid, link_only);

    int res = _z_multicast_send_t_msg(ztm, &cm);

    // Free the message
    _z_t_msg_clear(&cm);

    return res;
}

int _z_send_close(_z_transport_t *zt, uint8_t reason, int link_only)
{
    if (zt->_type == _Z_TRANSPORT_UNICAST_TYPE)
        return _z_unicast_send_close(&zt->_transport._unicast, reason, link_only);
    else if (zt->_type == _Z_TRANSPORT_MULTICAST_TYPE)
        return _z_multicast_send_close(&zt->_transport._multicast, reason, link_only);
    else
        return -1;
}

_z_transport_t *_z_transport_unicast_new(_z_link_t *link, _z_transport_unicast_establish_param_t param)
{
    _z_transport_t *zt = (_z_transport_t *)z_malloc(sizeof(_z_transport_t));
    zt->_type = _Z_TRANSPORT_UNICAST_TYPE;

    // Initialize the mutexes
    _z_mutex_init(&zt->_transport._unicast._mutex_tx);
    _z_mutex_init(&zt->_transport._unicast._mutex_rx);

    // Initialize the read and write buffers
    uint16_t mtu = link->_mtu < Z_BATCH_SIZE_TX ? link->_mtu : Z_BATCH_SIZE_TX;
    zt->_transport._unicast._wbuf = _z_wbuf_make(mtu, 0);
    zt->_transport._unicast._zbuf = _z_zbuf_make(Z_BATCH_SIZE_RX);

    // Initialize the defragmentation buffers
#if Z_DYNAMIC_MEMORY_ALLOCATION == 1
    zt->_transport._unicast._dbuf_reliable = _z_wbuf_make(0, 1);
    zt->_transport._unicast._dbuf_best_effort = _z_wbuf_make(0, 1);
#else
    zt->_transport._unicast._dbuf_reliable = _z_wbuf_make(Z_FRAG_MAX_SIZE, 0);
    zt->_transport._unicast._dbuf_best_effort = _z_wbuf_make(Z_FRAG_MAX_SIZE, 0);
#endif

    // Set default SN resolution
    zt->_transport._unicast._sn_resolution = param._sn_resolution;
    zt->_transport._unicast._sn_resolution_half = param._sn_resolution / 2;

    // The initial SN at TX side
    zt->_transport._unicast._sn_tx_reliable = param._initial_sn_tx;
    zt->_transport._unicast._sn_tx_best_effort = param._initial_sn_tx;

    // The initial SN at RX side
    zt->_transport._unicast._sn_rx_reliable = param._initial_sn_rx;
    zt->_transport._unicast._sn_rx_best_effort = param._initial_sn_rx;

    // Tasks
    zt->_transport._unicast._read_task_running = 0;
    zt->_transport._unicast._read_task = NULL;
    zt->_transport._unicast._lease_task_running = 0;
    zt->_transport._unicast._lease_task = NULL;

    // Notifiers
    zt->_transport._unicast._received = 0;
    zt->_transport._unicast._transmitted = 0;

    // Remote peer PID
    _z_bytes_move(&zt->_transport._unicast._remote_pid, &param._remote_pid);

    // Transport lease
    zt->_transport._unicast._lease = param._lease;

    // Transport link for unicast
    zt->_transport._unicast._link = link;

    return zt;
}

_z_transport_t *_z_transport_multicast_new(_z_link_t *link, _z_transport_multicast_establish_param_t param)
{
    _z_transport_t *zt = (_z_transport_t *)z_malloc(sizeof(_z_transport_t));
    zt->_type = _Z_TRANSPORT_MULTICAST_TYPE;

    // Initialize the mutexes
    _z_mutex_init(&zt->_transport._multicast._mutex_tx);
    _z_mutex_init(&zt->_transport._multicast._mutex_rx);
    _z_mutex_init(&zt->_transport._multicast._mutex_peer);

    // Initialize the read and write buffers
    uint16_t mtu = link->_mtu < Z_BATCH_SIZE_TX ? link->_mtu : Z_BATCH_SIZE_TX;
    zt->_transport._multicast._wbuf = _z_wbuf_make(mtu, 0);
    zt->_transport._multicast._zbuf = _z_zbuf_make(Z_BATCH_SIZE_RX);

    // Set default SN resolution
    zt->_transport._multicast._sn_resolution = param._sn_resolution;
    zt->_transport._multicast._sn_resolution_half = param._sn_resolution / 2;
    // The initial SN at TX side
    zt->_transport._multicast._sn_tx_reliable = param._initial_sn_tx;
    zt->_transport._multicast._sn_tx_best_effort = param._initial_sn_tx;

    // Initialize peer list
    zt->_transport._multicast._peers = _z_transport_peer_entry_list_new();

    // Tasks
    zt->_transport._multicast._read_task_running = 0;
    zt->_transport._multicast._read_task = NULL;
    zt->_transport._multicast._lease_task_running = 0;
    zt->_transport._multicast._lease_task = NULL;
    zt->_transport._multicast._lease = Z_TRANSPORT_LEASE;

    // Notifiers
    zt->_transport._multicast._transmitted = 0;

    // Transport link for unicast
    zt->_transport._multicast._link = link;

    return zt;
}

_z_transport_unicast_establish_param_result_t _z_transport_unicast_open_client(const _z_link_t *zl, const _z_bytes_t local_pid)
{
    _z_transport_unicast_establish_param_result_t ret;
    _z_transport_unicast_establish_param_t param;

    // Build the open message
    uint8_t version = Z_PROTO_VERSION;
    _z_zint_t whatami = Z_WHATAMI_CLIENT;
    _z_zint_t sn_resolution = Z_SN_RESOLUTION;
    int is_qos = 0;

    _z_bytes_t pid = _z_bytes_wrap(local_pid.start, local_pid.len);
    _z_transport_message_t ism = _z_t_msg_make_init_syn(version, whatami, sn_resolution, pid, is_qos);

    // Encode and send the message
    _Z_INFO("Sending Z_INIT(Syn)\n");
    int res = _z_link_send_t_msg(zl, &ism);
    if (res != 0) {
        goto ERR_1;
    }

    // The announced sn resolution
    param._sn_resolution = ism._body._init._sn_resolution;
    _z_t_msg_clear(&ism);

    _z_transport_message_result_t r_iam = _z_link_recv_t_msg(zl);
    if (r_iam._tag == _Z_RES_ERR) {
        goto ERR_1;
    }

    _z_transport_message_t iam = r_iam._value._transport_message;
    switch (_Z_MID(iam._header))
    {
    case _Z_MID_INIT:
    {
        _Z_INFO("Received Z_INIT(Ack)\n");
        if _Z_HAS_FLAG (iam._header, _Z_FLAG_T_A) {
            // Handle SN resolution option if present
            if _Z_HAS_FLAG (iam._header, _Z_FLAG_T_S) {
                // The resolution in the InitAck must be less or equal than the resolution in the InitSyn,
                // otherwise the InitAck message is considered invalid and it should be treated as a
                // CLOSE message with L==0 by the Initiating Peer -- the recipient of the InitAck message.
                if (iam._body._init._sn_resolution <= param._sn_resolution) {
                    param._sn_resolution = iam._body._init._sn_resolution;
                } else {
                    goto ERR_2;
                }
            }

            // The initial SN at TX side
            z_random_fill(&param._initial_sn_tx, sizeof(param._initial_sn_tx));
            param._initial_sn_tx = param._initial_sn_tx % param._sn_resolution;

            // Initialize the Local and Remote Peer IDs
            _z_bytes_copy(&param._remote_pid, &iam._body._init._pid);

            // Create the OpenSyn message
            _z_zint_t lease = Z_TRANSPORT_LEASE;
            _z_zint_t initial_sn = param._initial_sn_tx;
            _z_bytes_t cookie = iam._body._init._cookie;

            _z_transport_message_t osm = _z_t_msg_make_open_syn(lease, initial_sn, cookie);

            // Encode and send the message
            _Z_INFO("Sending Z_OPEN(Syn)\n");
            res = _z_link_send_t_msg(zl, &osm);

            if (res != 0) {
                goto ERR_3;
            }

            _z_transport_message_result_t r_oam = _z_link_recv_t_msg(zl);
            if (r_oam._tag == _Z_RES_ERR) {
                goto ERR_3;
            }
            _z_transport_message_t oam = r_oam._value._transport_message;
            _Z_INFO("Received Z_OPEN(Ack)\n");

            if (_Z_HAS_FLAG(oam._header, _Z_FLAG_T_A)) {
                // The session lease
                param._lease = oam._body._open._lease;

                // The initial SN at RX side. Initialize the session as we had already received
                // a message with a SN equal to initial_sn - 1.
                param._initial_sn_rx = _z_sn_decrement(param._sn_resolution, oam._body._open._initial_sn);
            } else {
                goto ERR_3;
            }

            _z_t_msg_clear(&oam);
            break;
        } else {
            goto ERR_2;
        }
    }

    default:
    {
        goto ERR_2;
    }
    }

    _z_t_msg_clear(&iam);

    ret._tag = _Z_RES_OK;
    ret._value._transport_unicast_establish_param = param;
    return ret;

ERR_3:
    _z_bytes_clear(&param._remote_pid);
ERR_2:
    _z_t_msg_clear(&iam);
ERR_1:
    ret._tag = _Z_RES_ERR;
    ret._value._error = -1;
    return ret;
}

_z_transport_multicast_establish_param_result_t _z_transport_multicast_open_client(const _z_link_t *zl, const _z_bytes_t local_pid)
{
    (void) (zl);
    (void) (local_pid);
    _z_transport_multicast_establish_param_result_t ret;
    ret._tag = _Z_RES_ERR;
    ret._value._error = -1;

    // @TODO: not implemented

    return ret;
}

_z_transport_unicast_establish_param_result_t _z_transport_unicast_open_peer(const _z_link_t *zl, const _z_bytes_t local_pid)
{
    (void) (zl);
    (void) (local_pid);
    _z_transport_unicast_establish_param_result_t ret;
    ret._tag = _Z_RES_ERR;
    ret._value._error = -1;

    // @TODO: not implemented

    return ret;
}

_z_transport_multicast_establish_param_result_t _z_transport_multicast_open_peer(const _z_link_t *zl, const _z_bytes_t local_pid)
{
    _z_transport_multicast_establish_param_result_t ret;
    _z_transport_multicast_establish_param_t param;
    param._is_qos = 0; // FIXME: make transport aware of qos configuration
    param._initial_sn_tx = 0;
    param._sn_resolution = Z_SN_RESOLUTION;

    // Explicitly send a JOIN message upon startup
    // FIXME: make transport aware of qos configuration
    _z_conduit_sn_list_t next_sns;
    next_sns._is_qos = 0;
    next_sns._val._plain._best_effort = param._initial_sn_tx;
    next_sns._val._plain._reliable = param._initial_sn_tx;

    _z_bytes_t pid = _z_bytes_wrap(local_pid.start, local_pid.len);
    _z_transport_message_t jsm = _z_t_msg_make_join(Z_PROTO_VERSION, Z_WHATAMI_PEER, Z_TRANSPORT_LEASE, param._sn_resolution, pid, next_sns);

    // Encode and send the message
    _Z_INFO("Sending Z_JOIN message\n");
    int res = _z_link_send_t_msg(zl, &jsm);

    _z_t_msg_clear(&jsm);

    if (res != 0)
        goto ERR_1;

    ret._tag = _Z_RES_OK;
    ret._value._transport_multicast_establish_param = param;
    return ret;

ERR_1:
    ret._tag = _Z_RES_ERR;
    ret._value._error = -1;
    return ret;
}

int _z_transport_unicast_close(_z_transport_unicast_t *ztu, uint8_t reason)
{
    return _z_unicast_send_close(ztu, reason, 0);
}

int _z_transport_multicast_close(_z_transport_multicast_t *ztm, uint8_t reason)
{
    return _z_multicast_send_close(ztm, reason, 0);
}

int _z_transport_close(_z_transport_t *zt, uint8_t reason)
{
    return _z_send_close(zt, reason, 0);
}

void _z_transport_unicast_clear(_z_transport_unicast_t *ztu)
{
    // Clean up tasks
    if (ztu->_read_task != NULL)
    {
        _z_task_join(ztu->_read_task);
        _z_task_free(&ztu->_read_task);
    }
    if (ztu->_lease_task != NULL)
    {
        _z_task_join(ztu->_lease_task);
        _z_task_free(&ztu->_lease_task);
    }

    // Clean up the mutexes
    _z_mutex_free(&ztu->_mutex_tx);
    _z_mutex_free(&ztu->_mutex_rx);

    // Clean up the buffers
    _z_wbuf_clear(&ztu->_wbuf);
    _z_zbuf_clear(&ztu->_zbuf);
    _z_wbuf_clear(&ztu->_dbuf_reliable);
    _z_wbuf_clear(&ztu->_dbuf_best_effort);

    // Clean up PIDs
    _z_bytes_clear(&ztu->_remote_pid);

    if (ztu->_link != NULL)
        _z_link_free((_z_link_t **)&ztu->_link);
}

void _z_transport_multicast_clear(_z_transport_multicast_t *ztm)
{
    // Clean up tasks
    if (ztm->_read_task != NULL)
    {
        _z_task_join(ztm->_read_task);
        _z_task_free(&ztm->_read_task);
    }
    if (ztm->_lease_task != NULL)
    {
        _z_task_join(ztm->_lease_task);
        _z_task_free(&ztm->_lease_task);
    }

    // Clean up the mutexes
    _z_mutex_free(&ztm->_mutex_tx);
    _z_mutex_free(&ztm->_mutex_rx);
    _z_mutex_free(&ztm->_mutex_peer);

    // Clean up the buffers
    _z_wbuf_clear(&ztm->_wbuf);
    _z_zbuf_clear(&ztm->_zbuf);

    // Clean up peer list
    _z_transport_peer_entry_list_free(&ztm->_peers);

    if (ztm->_link != NULL)
        _z_link_free((_z_link_t **)&ztm->_link);
}

void _z_transport_free(_z_transport_t **zt)
{
    _z_transport_t *ptr = *zt;

    if (ptr->_type == _Z_TRANSPORT_UNICAST_TYPE)
        _z_transport_unicast_clear(&ptr->_transport._unicast);
    else if (ptr->_type == _Z_TRANSPORT_MULTICAST_TYPE)
        _z_transport_multicast_clear(&ptr->_transport._multicast);

    z_free(ptr);
    *zt = NULL;
}
