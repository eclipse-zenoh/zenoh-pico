//
// Copyright (c) 2026 ZettaScale Technology
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

#include "zenoh-pico/transport/multicast/tx.h"

#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/codec/network.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/utils.h"

_z_zint_t _z_transport_multicast_get_sn(_z_transport_multicast_t *ztm, z_reliability_t reliability) {
    _z_zint_t sn;
    if (reliability == Z_RELIABILITY_RELIABLE) {
        sn = ztm->_common._sn_tx_reliable;
        ztm->_common._sn_tx_reliable = _z_sn_increment(ztm->_common._sn_res, ztm->_common._sn_tx_reliable);
    } else {
        sn = ztm->_common._sn_tx_best_effort;
        ztm->_common._sn_tx_best_effort = _z_sn_increment(ztm->_common._sn_res, ztm->_common._sn_tx_best_effort);
    }
    return sn;
}

static z_result_t _z_transport_multicast_flush_buffer(_z_transport_multicast_t *ztm) {
    _z_wbuf_finalize(&ztm->_common._wbuf, ztm->_common._link->_cap._flow);
    _Z_RETURN_IF_ERR(_z_link_send_wbuf(ztm->_common._link, &ztm->_common._wbuf, NULL));
    ztm->_common._transmitted = true;
#if Z_FEATURE_BATCHING == 1
    ztm->_common._batch_count = 0;
#endif
    return _Z_RES_OK;
}

static z_result_t _z_transport_multicast_send_t_msg_inner(_z_transport_multicast_t *ztm,
                                                          const _z_transport_message_t *t_msg) {
#if Z_FEATURE_BATCHING == 1
    if (ztm->_common._batch_state == _Z_BATCHING_ACTIVE || ztm->_common._batch_count > 0) {
        _Z_RETURN_IF_ERR(_z_transport_multicast_flush_buffer(ztm));
    }
#endif
    _z_wbuf_prepare(&ztm->_common._wbuf, ztm->_common._link->_cap._flow);
    _Z_RETURN_IF_ERR(_z_transport_message_encode(&ztm->_common._wbuf, t_msg));
    // Send message
    return _z_transport_multicast_flush_buffer(ztm);
}

z_result_t _z_transport_multicast_send_t_msg(_z_transport_multicast_t *ztm, const _z_transport_message_t *t_msg) {
    z_result_t ret = _Z_RES_OK;
    _Z_DEBUG("Send session message on transport multicast");
    // If sending to a peer list, make sure the peer mutex is locked
    _z_transport_tx_mutex_lock(&ztm->_common, true);

    ret = _z_transport_multicast_send_t_msg_inner(ztm, t_msg);

    _z_transport_tx_mutex_unlock(&ztm->_common);
    return ret;
}

#if Z_FEATURE_FRAGMENTATION == 1
static z_result_t _z_transport_multicast_send_fragment_inner(_z_transport_multicast_t *ztm, _z_wbuf_t *frag_buff,
                                                             const _z_network_message_t *n_msg,
                                                             z_reliability_t reliability, _z_zint_t first_sn) {
    bool is_first = true;
    _z_zint_t sn = first_sn;
    // Encode message on temp buffer
    _Z_RETURN_IF_ERR(_z_network_message_encode(frag_buff, n_msg));
    // Fragment message
    while (_z_wbuf_len(frag_buff) > 0) {
        // Get fragment sequence number
        if (!is_first) {
            sn = _z_transport_multicast_get_sn(ztm, reliability);
        }
        // Serialize fragment
        _z_wbuf_prepare(&ztm->_common._wbuf, ztm->_common._link->_cap._flow);
        z_result_t ret = _z_wbuf_serialize_zenoh_fragment(&ztm->_common._wbuf, frag_buff, reliability, sn, is_first);
        if (ret != _Z_RES_OK) {
            _Z_ERROR("Fragment serialization failed with err %d", ret);
            return ret;
        }
        _z_transport_multicast_flush_buffer(ztm);
        is_first = false;
    }
    return _Z_RES_OK;
}

static z_result_t _z_transport_multicast_send_fragment(_z_transport_multicast_t *ztm, const _z_network_message_t *n_msg,
                                                       z_reliability_t reliability, _z_zint_t first_sn) {
    // Create an expandable wbuf for fragmentation
    _z_wbuf_t frag_buff;
    _Z_RETURN_IF_ERR(_z_wbuf_init(&frag_buff, _Z_FRAG_BUFF_BASE_SIZE, true));
    // Send message as fragments
    z_result_t ret = _z_transport_multicast_send_fragment_inner(ztm, &frag_buff, n_msg, reliability, first_sn);
    // Clear the buffer as it's no longer required
    _z_wbuf_clear(&frag_buff);
    return ret;
}
#endif

static z_result_t _z_transport_multicast_send_n_msg_inner(_z_transport_multicast_t *ztm,
                                                          const _z_network_message_t *n_msg,
                                                          z_reliability_t reliability) {
    _z_zint_t sn = 0;
#if Z_FEATURE_BATCHING == 1
    bool batch_has_data = ztm->_common._batch_state == _Z_BATCHING_ACTIVE && ztm->_common._batch_count > 0;
    if (!batch_has_data) {
#endif
        _z_wbuf_prepare(&ztm->_common._wbuf, ztm->_common._link->_cap._flow);
        sn = _z_transport_multicast_get_sn(ztm, reliability);
        _z_transport_message_t t_msg = _z_t_msg_make_frame_header(sn, reliability);
        _Z_RETURN_IF_ERR(_z_transport_message_encode(&ztm->_common._wbuf, &t_msg));
#if Z_FEATURE_BATCHING == 1
    }
    size_t prev_wpos = _z_wbuf_get_wpos(&ztm->_common._wbuf);
#endif

    // Try encoding the network message
    z_result_t ret = _z_network_message_encode(&ztm->_common._wbuf, n_msg);

    if (ret == _Z_RES_OK) {
#if Z_FEATURE_BATCHING == 1
        if (ztm->_common._batch_state == _Z_BATCHING_ACTIVE && !_z_network_message_get_express_status(n_msg)) {
            // Increment batch
            ztm->_common._batch_count++;
            return _Z_RES_OK;
        } else {
#endif
            return _z_transport_multicast_flush_buffer(ztm);
#if Z_FEATURE_BATCHING == 1
        }
    } else if (batch_has_data) {
        // flush the buffer and retry sending again on an empty one
        _z_wbuf_set_wpos(&ztm->_common._wbuf, prev_wpos);
        _z_transport_multicast_flush_buffer(ztm);
        return _z_transport_multicast_send_n_msg_inner(ztm, n_msg, reliability);
#endif
    } else {
#if Z_FEATURE_FRAGMENTATION == 1
        return _z_transport_multicast_send_fragment(ztm, n_msg, reliability, sn);
#else
        return ret;
#endif
    }
}

z_result_t _z_transport_multicast_send_n_msg(_z_transport_multicast_t *ztm, const _z_network_message_t *n_msg,
                                             z_reliability_t reliability, z_congestion_control_t cong_ctrl) {
    z_result_t ret = _Z_RES_OK;
    _Z_DEBUG("Send network message");

    // Acquire the lock and drop the message if needed
    if (!_z_transport_batch_hold_tx_mutex()) {
        ret = _z_transport_tx_mutex_lock(&ztm->_common, cong_ctrl == Z_CONGESTION_CONTROL_BLOCK);
    }
    if (ret != _Z_RES_OK) {
        _Z_INFO("Dropping zenoh message because of congestion control");
        return ret;
    }
    ret = _z_transport_multicast_send_n_msg_inner(ztm, n_msg, reliability);
    if (!_z_transport_batch_hold_tx_mutex()) {
        _z_transport_tx_mutex_unlock(&ztm->_common);
    }
    return ret;
}

#if Z_FEATURE_BATCHING == 1
z_result_t _z_transport_multicast_send_n_batch(_z_transport_multicast_t *ztm) {
    z_result_t ret = _Z_RES_OK;
    if (!_z_transport_batch_hold_tx_mutex()) {
        _Z_RETURN_IF_ERR(_z_transport_tx_mutex_lock(&ztm->_common, true));
    }
    if (ztm->_common._batch_count > 0) {
        _Z_DEBUG("Send network batch");
        ret = _z_transport_multicast_flush_buffer(ztm);
    }
    if (!_z_transport_batch_hold_tx_mutex()) {
        _z_transport_tx_mutex_unlock(&ztm->_common);
    }
    return ret;
}
#endif
