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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/link/link.h"
#include "zenoh-pico/transport/common/rx.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/transport/multicast/rx.h"
#include "zenoh-pico/transport/unicast.h"
#include "zenoh-pico/transport/unicast/lease.h"
#include "zenoh-pico/transport/unicast/read.h"
#include "zenoh-pico/transport/unicast/rx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_UNICAST_TRANSPORT == 1

z_result_t _z_unicast_transport_create(_z_transport_t *zt, _z_link_t *zl,
                                       _z_transport_unicast_establish_param_t *param) {
    z_result_t ret = _Z_RES_OK;

    zt->_type = _Z_TRANSPORT_UNICAST_TYPE;
    _z_transport_unicast_t *ztu = &zt->_transport._unicast;
#if Z_FEATURE_FRAGMENTATION == 1
    // Patch
    zt->_transport._unicast._patch = param->_patch;
#endif

// Initialize batching data
#if Z_FEATURE_BATCHING == 1
    ztu->_common._batch_state = _Z_BATCHING_IDLE;
    ztu->_common._batch_count = 0;
#endif

#if Z_FEATURE_MULTI_THREAD == 1
    // Initialize the mutexes
    ret = _z_mutex_init(&ztu->_common._mutex_tx);
    if (ret == _Z_RES_OK) {
        ret = _z_mutex_init(&ztu->_common._mutex_rx);
        if (ret != _Z_RES_OK) {
            _z_mutex_drop(&ztu->_common._mutex_tx);
        }
    }
#endif  // Z_FEATURE_MULTI_THREAD == 1

    // Initialize the read and write buffers
    if (ret == _Z_RES_OK) {
        uint16_t mtu = (zl->_mtu < param->_batch_size) ? zl->_mtu : param->_batch_size;
        size_t wbuf_size = mtu;
        size_t zbuf_size = param->_batch_size;

        // Initialize tx rx buffers
        ztu->_common._wbuf = _z_wbuf_make(wbuf_size, false);
        ztu->_common._zbuf = _z_zbuf_make(zbuf_size);

        // Initialize resources pool
        ztu->_common._arc_pool = _z_arc_slice_svec_make(_Z_RES_POOL_INIT_SIZE);
        ztu->_common._msg_pool = _z_network_message_svec_make(_Z_RES_POOL_INIT_SIZE);

        // Clean up the buffers if one of them failed to be allocated
        if ((ztu->_common._msg_pool._capacity == 0) || (ztu->_common._arc_pool._capacity == 0) ||
            (_z_wbuf_capacity(&ztu->_common._wbuf) != wbuf_size) ||
            (_z_zbuf_capacity(&ztu->_common._zbuf) != zbuf_size)) {
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            _Z_ERROR("Not enough memory to allocate transport tx rx buffers!");

#if Z_FEATURE_MULTI_THREAD == 1
            _z_mutex_drop(&ztu->_common._mutex_tx);
            _z_mutex_drop(&ztu->_common._mutex_rx);
#endif  // Z_FEATURE_MULTI_THREAD == 1

            _z_wbuf_clear(&ztu->_common._wbuf);
            _z_zbuf_clear(&ztu->_common._zbuf);
        }

#if Z_FEATURE_FRAGMENTATION == 1
        // Initialize the defragmentation buffers
        ztu->_state_reliable = _Z_DBUF_STATE_NULL;
        ztu->_state_best_effort = _Z_DBUF_STATE_NULL;
        ztu->_dbuf_reliable = _z_wbuf_null();
        ztu->_dbuf_best_effort = _z_wbuf_null();
#endif  // Z_FEATURE_FRAGMENTATION == 1
    }

    if (ret == _Z_RES_OK) {
        // Set default SN resolution
        ztu->_common._sn_res = _z_sn_max(param->_seq_num_res);

        // The initial SN at TX side
        ztu->_common._sn_tx_reliable = param->_initial_sn_tx;
        ztu->_common._sn_tx_best_effort = param->_initial_sn_tx;

        // The initial SN at RX side
        _z_zint_t initial_sn_rx = _z_sn_decrement(ztu->_common._sn_res, param->_initial_sn_rx);
        ztu->_sn_rx_reliable = initial_sn_rx;
        ztu->_sn_rx_best_effort = initial_sn_rx;

#if Z_FEATURE_MULTI_THREAD == 1
        // Tasks
        ztu->_common._read_task_running = false;
        ztu->_common._read_task = NULL;
        ztu->_common._lease_task_running = false;
        ztu->_common._lease_task = NULL;
#endif  // Z_FEATURE_MULTI_THREAD == 1

        // Notifiers
        ztu->_received = 0;
        ztu->_common._transmitted = 0;

        // Transport lease
        ztu->_common._lease = param->_lease;

        // Transport link for unicast
        ztu->_common._link = *zl;

        // Remote peer PID
        ztu->_remote_zid = param->_remote_zid;
    } else {
        param->_remote_zid = _z_id_empty();
    }

    return ret;
}

static z_result_t _z_unicast_handshake_client(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                              const _z_id_t *local_zid, enum z_whatami_t whatami) {
    _z_transport_message_t ism = _z_t_msg_make_init_syn(whatami, *local_zid);
    param->_seq_num_res = ism._body._init._seq_num_res;  // The announced sn resolution
    param->_req_id_res = ism._body._init._req_id_res;    // The announced req id resolution
    param->_batch_size = ism._body._init._batch_size;    // The announced batch size

    // Encode and send the message
    _Z_DEBUG("Sending Z_INIT(Syn)");
    z_result_t ret = _z_link_send_t_msg(zl, &ism);
    _z_t_msg_clear(&ism);
    if (ret != _Z_RES_OK) {
        return ret;
    }
    // Try to receive response
    _z_transport_message_t iam;
    _Z_RETURN_IF_ERR(_z_link_recv_t_msg(&iam, zl));
    if ((_Z_MID(iam._header) != _Z_MID_T_INIT) || !_Z_HAS_FLAG(iam._header, _Z_FLAG_T_INIT_A)) {
        _z_t_msg_clear(&iam);
        return _Z_ERR_MESSAGE_UNEXPECTED;
    }
    _Z_DEBUG("Received Z_INIT(Ack)");
    // Any of the size parameters in the InitAck must be less or equal than the one in the InitSyn,
    // otherwise the InitAck message is considered invalid and it should be treated as a
    // CLOSE message with L==0 by the Initiating Peer -- the recipient of the InitAck message.
    if (iam._body._init._seq_num_res <= param->_seq_num_res) {
        param->_seq_num_res = iam._body._init._seq_num_res;
    } else {
        ret = _Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION;
    }
    if (iam._body._init._req_id_res <= param->_req_id_res) {
        param->_req_id_res = iam._body._init._req_id_res;
    } else {
        ret = _Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION;
    }
    if (iam._body._init._batch_size <= param->_batch_size) {
        param->_batch_size = iam._body._init._batch_size;
    } else {
        ret = _Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION;
    }
#if Z_FEATURE_FRAGMENTATION == 1
    if (iam._body._init._patch <= ism._body._init._patch) {
        param->_patch = iam._body._init._patch;
    } else {
        // TODO: Use a better error code?
        ret = _Z_ERR_GENERIC;
    }
#endif
    if (ret != _Z_RES_OK) {
        _z_t_msg_clear(&iam);
        return ret;
    }
    param->_key_id_res = 0x08 << param->_key_id_res;
    param->_req_id_res = 0x08 << param->_req_id_res;

    // The initial SN at TX side
    z_random_fill(&param->_initial_sn_tx, sizeof(param->_initial_sn_tx));
    param->_initial_sn_tx = param->_initial_sn_tx & !_z_sn_modulo_mask(param->_seq_num_res);

    // Initialize the Local and Remote Peer IDs
    param->_remote_zid = iam._body._init._zid;

    // Create the OpenSyn message
    _z_zint_t lease = Z_TRANSPORT_LEASE;
    _z_zint_t initial_sn = param->_initial_sn_tx;
    _z_slice_t cookie;
    _z_slice_copy(&cookie, &iam._body._init._cookie);
    _z_transport_message_t osm = _z_t_msg_make_open_syn(lease, initial_sn, cookie);
    _z_t_msg_clear(&iam);
    // Encode and send the message
    _Z_DEBUG("Sending Z_OPEN(Syn)");
    ret = _z_link_send_t_msg(zl, &osm);
    _z_t_msg_clear(&osm);
    if (ret != _Z_RES_OK) {
        return ret;
    }
    // Try to receive response
    _z_transport_message_t oam;
    _Z_RETURN_IF_ERR(_z_link_recv_t_msg(&oam, zl));
    if ((_Z_MID(oam._header) != _Z_MID_T_OPEN) || !_Z_HAS_FLAG(oam._header, _Z_FLAG_T_OPEN_A)) {
        _z_t_msg_clear(&oam);
        ret = _Z_ERR_MESSAGE_UNEXPECTED;
    }
    _Z_DEBUG("Received Z_OPEN(Ack)");
    param->_lease = oam._body._open._lease;  // The session lease
    // The initial SN at RX side. Initialize the session as we had already received
    // a message with a SN equal to initial_sn - 1.
    param->_initial_sn_rx = oam._body._open._initial_sn;
    _z_t_msg_clear(&oam);
    return _Z_RES_OK;
}

// TODO: Activate if we add peer unicast support
#if 0
static z_result_t _z_unicast_handshake_listener(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                                const _z_id_t *local_zid, enum z_whatami_t whatami) {
    // Read t message from link
    _z_transport_message_t tmsg;
    z_result_t ret = _z_link_recv_t_msg(&tmsg, zl);
    if (ret != _Z_RES_OK) {
        return ret;
    }
    // Receive InitSyn
    if (_Z_MID(tmsg._header) != _Z_MID_T_INIT || _Z_HAS_FLAG(tmsg._header, _Z_FLAG_T_INIT_A)) {
        _z_t_msg_clear(&tmsg);
        return _Z_ERR_MESSAGE_UNEXPECTED;
    }
    _Z_DEBUG("Received Z_INIT(Syn)");
    // Encode InitAck
    _z_slice_t cookie = _z_slice_null();
    _z_transport_message_t iam = _z_t_msg_make_init_ack(whatami, *local_zid, cookie);
    // Any of the size parameters in the InitAck must be less or equal than the one in the InitSyn,
    if (iam._body._init._seq_num_res > tmsg._body._init._seq_num_res) {
        iam._body._init._seq_num_res = tmsg._body._init._seq_num_res;
    }
    if (iam._body._init._req_id_res > tmsg._body._init._req_id_res) {
        iam._body._init._req_id_res = tmsg._body._init._req_id_res;
    }
    if (iam._body._init._batch_size > tmsg._body._init._batch_size) {
        iam._body._init._batch_size = tmsg._body._init._batch_size;
    }
#if Z_FEATURE_FRAGMENTATION == 1
    if (iam._body._init._patch > tmsg._body._init._patch) {
        iam._body._init._patch = tmsg._body._init._patch;
    }
#endif
    param->_remote_zid = tmsg._body._init._zid;
    param->_seq_num_res = iam._body._init._seq_num_res;
    param->_req_id_res = iam._body._init._req_id_res;
    param->_batch_size = iam._body._init._batch_size;
    param->_key_id_res = 0x08 << param->_key_id_res;
    param->_req_id_res = 0x08 << param->_req_id_res;
    _z_t_msg_clear(&tmsg);
    // Send InitAck
    _Z_DEBUG("Sending Z_INIT(Ack)");
    ret = _z_link_send_t_msg(zl, &iam);
    _z_t_msg_clear(&iam);
    if (ret != _Z_RES_OK) {
        return ret;
    }
    // Read t message from link
    ret = _z_link_recv_t_msg(&tmsg, zl);
    if (ret != _Z_RES_OK) {
        return ret;
    }
    // Receive OpenSyn
    if (_Z_MID(tmsg._header) != _Z_MID_T_OPEN || _Z_HAS_FLAG(tmsg._header, _Z_FLAG_T_INIT_A)) {
        _z_t_msg_clear(&tmsg);
        return _Z_ERR_MESSAGE_UNEXPECTED;
    }
    _Z_DEBUG("Received Z_OPEN(Syn)");
    // Process message
    param->_lease = tmsg._body._open._lease;
    param->_initial_sn_rx = tmsg._body._open._initial_sn;
    _z_t_msg_clear(&tmsg);

    // Init sn, tx side
    z_random_fill(&param->_initial_sn_tx, sizeof(param->_initial_sn_tx));
    param->_initial_sn_tx = param->_initial_sn_tx & !_z_sn_modulo_mask(param->_seq_num_res);

    // Encode OpenAck
    _z_zint_t lease = Z_TRANSPORT_LEASE;
    _z_zint_t initial_sn = param->_initial_sn_tx;
    _z_transport_message_t oam = _z_t_msg_make_open_ack(lease, initial_sn);

    // Encode and send the message
    _Z_DEBUG("Sending Z_OPEN(Ack)");
    ret = _z_link_send_t_msg(zl, &oam);
    _z_t_msg_clear(&oam);
    if (ret != _Z_RES_OK) {
        return ret;
    }
    // Handshake finished
    return _Z_RES_OK;
}
#else
static z_result_t _z_unicast_handshake_listener(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                                const _z_id_t *local_zid, enum z_whatami_t whatami) {
    _ZP_UNUSED(param);
    _ZP_UNUSED(zl);
    _ZP_UNUSED(local_zid);
    _ZP_UNUSED(whatami);
    return _Z_ERR_TRANSPORT_OPEN_FAILED;
}
#endif

z_result_t _z_unicast_open_client(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                  const _z_id_t *local_zid) {
    return _z_unicast_handshake_client(param, zl, local_zid, Z_WHATAMI_CLIENT);
}

z_result_t _z_unicast_open_peer(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                const _z_id_t *local_zid, int peer_op) {
    z_result_t ret = _Z_RES_OK;
    if (peer_op == _Z_PEER_OP_OPEN) {
        ret = _z_unicast_handshake_client(param, zl, local_zid, Z_WHATAMI_PEER);
    } else {
        ret = _z_unicast_handshake_listener(param, zl, local_zid, Z_WHATAMI_PEER);
    }
    return ret;
}

z_result_t _z_unicast_send_close(_z_transport_unicast_t *ztu, uint8_t reason, bool link_only) {
    z_result_t ret = _Z_RES_OK;
    // Send and clear message
    _z_transport_message_t cm = _z_t_msg_make_close(reason, link_only);
    ret = _z_transport_tx_send_t_msg(&ztu->_common, &cm);
    _z_t_msg_clear(&cm);
    return ret;
}

z_result_t _z_unicast_transport_close(_z_transport_unicast_t *ztu, uint8_t reason) {
    return _z_unicast_send_close(ztu, reason, false);
}

void _z_unicast_transport_clear(_z_transport_t *zt) {
    _z_transport_unicast_t *ztu = &zt->_transport._unicast;
#if Z_FEATURE_MULTI_THREAD == 1
    // Clean up tasks
    if (ztu->_common._read_task != NULL) {
        _z_task_join(ztu->_common._read_task);
        z_free(ztu->_common._read_task);
    }
    if (ztu->_common._lease_task != NULL) {
        _z_task_join(ztu->_common._lease_task);
        z_free(ztu->_common._lease_task);
    }

    // Clean up the mutexes
    _z_mutex_drop(&ztu->_common._mutex_tx);
    _z_mutex_drop(&ztu->_common._mutex_rx);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    // Clean up the buffers
    _z_wbuf_clear(&ztu->_common._wbuf);
    _z_zbuf_clear(&ztu->_common._zbuf);
    _z_arc_slice_svec_release(&ztu->_common._arc_pool);
    _z_network_message_svec_release(&ztu->_common._msg_pool);
#if Z_FEATURE_FRAGMENTATION == 1
    _z_wbuf_clear(&ztu->_dbuf_reliable);
    _z_wbuf_clear(&ztu->_dbuf_best_effort);
#endif

    // Clean up PIDs
    ztu->_remote_zid = _z_id_empty();
    _z_link_clear(&ztu->_common._link);
}

#else

z_result_t _z_unicast_transport_create(_z_transport_t *zt, _z_link_t *zl,
                                       _z_transport_unicast_establish_param_t *param) {
    _ZP_UNUSED(zt);
    _ZP_UNUSED(zl);
    _ZP_UNUSED(param);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _z_unicast_open_client(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                  const _z_id_t *local_zid) {
    _ZP_UNUSED(param);
    _ZP_UNUSED(zl);
    _ZP_UNUSED(local_zid);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _z_unicast_open_peer(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                const _z_id_t *local_zid, int peer_op) {
    _ZP_UNUSED(param);
    _ZP_UNUSED(zl);
    _ZP_UNUSED(local_zid);
    _ZP_UNUSED(peer_op);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _z_unicast_send_close(_z_transport_unicast_t *ztu, uint8_t reason, bool link_only) {
    _ZP_UNUSED(ztu);
    _ZP_UNUSED(reason);
    _ZP_UNUSED(link_only);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _z_unicast_transport_close(_z_transport_unicast_t *ztu, uint8_t reason) {
    _ZP_UNUSED(ztu);
    _ZP_UNUSED(reason);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

void _z_unicast_transport_clear(_z_transport_t *zt) { _ZP_UNUSED(zt); }

#endif  // Z_FEATURE_UNICAST_TRANSPORT == 1
