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

#include "zenoh-pico/transport/unicast/tx.h"

#include "zenoh-pico/collections/algorithms_template.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/codec/network.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/utils.h"

static _z_zint_t _z_transport_unicast_get_sn(_z_transport_unicast_t *ztu, z_reliability_t reliability) {
    // FIXME: this is incorrect to use common sn, in case of multiple links.
    _z_zint_t sn;
    if (reliability == Z_RELIABILITY_RELIABLE) {
        sn = ztu->_common._sn_tx_reliable;
        ztu->_common._sn_tx_reliable = _z_sn_increment(ztu->_common._sn_res, ztu->_common._sn_tx_reliable);
    } else {
        sn = ztu->_common._sn_tx_best_effort;
        ztu->_common._sn_tx_best_effort = _z_sn_increment(ztu->_common._sn_res, ztu->_common._sn_tx_best_effort);
    }
    return sn;
}

static z_result_t _z_transport_unicast_flush_buffer(_z_transport_unicast_t *ztu,
                                                    const _z_peer_mask_bitset_t *opt_peers) {
    _z_wbuf_finalize(&ztu->_common._wbuf, ztu->_common._link->_cap._flow);
    if (opt_peers == NULL) {
        _z_transport_peer_unicast_t *peer = NULL;
        _ZP_FOREACH (_z_transport_peer_unicast_hset, &ztu->_peers, peer) {
            _z_link_send_wbuf(ztu->_common._link, &ztu->_common._wbuf, &peer->_socket);
        }
    } else {
        // FIXME: normally we should hold a peers lock from the moment we acquite the bitmask, to the moment we finish
        // iterating, since some peers can be removed, and even re-added at the positions specified by mask in the
        // meantime. Intentionally keeping non-thread-safe for now, since this is how it was before, before we rework
        // locking mechanisms.
        for (_z_peer_mask_bitset_iter_t it = _z_peer_mask_bitset_begin_true(opt_peers);
             it != _z_peer_mask_bitset_end(opt_peers); it = _z_peer_mask_bitset_iter_next_true(opt_peers, it)) {
            _z_transport_peer_unicast_t *peer =
                _z_transport_peer_unicast_hset_at(&ztu->_peers, (_z_transport_peer_unicast_hset_iter_t)it);
            _z_link_send_wbuf(ztu->_common._link, &ztu->_common._wbuf, &peer->_socket);
        }
    }
    // FIXME: This is wrong to use transmitted flag inside ztu->_common, since some peers might not be included in the
    // mask. Intenionally keeping it this way for now, since this is how it was before, before we rework the transport
    // peer management.
    ztu->_common._transmitted = true;
#if Z_FEATURE_BATCHING == 1
    ztu->_common._batch_count = 0;
#endif
    return _Z_RES_OK;
}

static z_result_t _z_transport_unicast_send_t_msg_inner(_z_transport_unicast_t *ztu,
                                                        const _z_transport_message_t *t_msg,
                                                        const _z_peer_mask_bitset_t *opt_peers) {
#if Z_FEATURE_BATCHING == 1
    if (ztu->_common._batch_state == _Z_BATCHING_ACTIVE && ztu->_common._batch_count > 0) {
        _Z_RETURN_IF_ERR(_z_transport_unicast_flush_buffer(ztu, opt_peers));
    }
#endif
    // FIXME: this is wrong to use ztu->_common._link->_cap._flow, in case we will ever have
    // multiple links with different flow capabilities, we should use the peer's link instead.
    // Intenionally keeping it this way for now, since this is how it was before, before we rework the transport peer
    // management.
    _z_wbuf_prepare(&ztu->_common._wbuf, ztu->_common._link->_cap._flow);
    _Z_RETURN_IF_ERR(_z_transport_message_encode(&ztu->_common._wbuf, t_msg));
    // Send message
    return _z_transport_unicast_flush_buffer(ztu, opt_peers);
}

z_result_t _z_transport_unicast_send_t_msg(_z_transport_unicast_t *ztu, const _z_transport_message_t *t_msg,
                                           const _z_peer_mask_bitset_t *opt_peers) {
    z_result_t ret = _Z_RES_OK;
    _Z_DEBUG("Send session message on transport unicast");
    _z_transport_tx_mutex_lock(&ztu->_common, true);

    ret = _z_transport_unicast_send_t_msg_inner(ztu, t_msg, opt_peers);

    _z_transport_tx_mutex_unlock(&ztu->_common);
    return ret;
}

#if Z_FEATURE_FRAGMENTATION == 1
static z_result_t _z_transport_unicast_send_fragment_inner(_z_transport_unicast_t *ztu, _z_wbuf_t *frag_buff,
                                                           const _z_network_message_t *n_msg,
                                                           z_reliability_t reliability, _z_zint_t first_sn,
                                                           const _z_peer_mask_bitset_t *opt_peers) {
    bool is_first = true;
    _z_zint_t sn = first_sn;
    // Encode message on temp buffer
    _Z_RETURN_IF_ERR(_z_network_message_encode(frag_buff, n_msg));
    // Fragment message
    while (_z_wbuf_len(frag_buff) > 0) {
        // Get fragment sequence number
        if (!is_first) {
            sn = _z_transport_unicast_get_sn(ztu, reliability);
        }
        // Serialize fragment
        _z_wbuf_prepare(&ztu->_common._wbuf, ztu->_common._link->_cap._flow);
        z_result_t ret = _z_wbuf_serialize_zenoh_fragment(&ztu->_common._wbuf, frag_buff, reliability, sn, is_first);
        if (ret != _Z_RES_OK) {
            _Z_ERROR("Fragment serialization failed with err %d", ret);
            return ret;
        }
        _z_transport_unicast_flush_buffer(ztu, opt_peers);
        is_first = false;
    }
    return _Z_RES_OK;
}

static z_result_t _z_transport_unicast_send_fragment(_z_transport_unicast_t *ztu, const _z_network_message_t *n_msg,
                                                     z_reliability_t reliability, _z_zint_t first_sn,
                                                     const _z_peer_mask_bitset_t *opt_peers) {
    // Create an expandable wbuf for fragmentation
    _z_wbuf_t frag_buff;
    _Z_RETURN_IF_ERR(_z_wbuf_init(&frag_buff, _Z_FRAG_BUFF_BASE_SIZE, true));
    // Send message as fragments
    z_result_t ret = _z_transport_unicast_send_fragment_inner(ztu, &frag_buff, n_msg, reliability, first_sn, opt_peers);
    // Clear the buffer as it's no longer required
    _z_wbuf_clear(&frag_buff);
    return ret;
}
#endif

static z_result_t _z_transport_unicast_send_n_msg_inner(_z_transport_unicast_t *ztu, const _z_network_message_t *n_msg,
                                                        z_reliability_t reliability,
                                                        const _z_peer_mask_bitset_t *opt_peers) {
    _z_zint_t sn = 0;
#if Z_FEATURE_BATCHING == 1
    bool batch_has_data = ztu->_common._batch_state == _Z_BATCHING_ACTIVE && ztu->_common._batch_count > 0;
    if (!batch_has_data) {
#endif
        _z_wbuf_prepare(&ztu->_common._wbuf, ztu->_common._link->_cap._flow);
        sn = _z_transport_unicast_get_sn(ztu, reliability);
        _z_transport_message_t t_msg = _z_t_msg_make_frame_header(sn, reliability);
        _Z_RETURN_IF_ERR(_z_transport_message_encode(&ztu->_common._wbuf, &t_msg));
#if Z_FEATURE_BATCHING == 1
    }
    size_t prev_wpos = _z_wbuf_get_wpos(&ztu->_common._wbuf);
#endif

    // Try encoding the network message
    z_result_t ret = _z_network_message_encode(&ztu->_common._wbuf, n_msg);

    if (ret == _Z_RES_OK) {
#if Z_FEATURE_BATCHING == 1
        if (ztu->_common._batch_state == _Z_BATCHING_ACTIVE && !_z_network_message_get_express_status(n_msg)) {
            // Increment batch
            ztu->_common._batch_count++;
            return _Z_RES_OK;
        } else {
#endif
            return _z_transport_unicast_flush_buffer(ztu, opt_peers);
#if Z_FEATURE_BATCHING == 1
        }
    } else if (batch_has_data) {
        // flush the buffer and retry sending again on an empty one
        _z_wbuf_set_wpos(&ztu->_common._wbuf, prev_wpos);
        _z_transport_unicast_flush_buffer(ztu, opt_peers);  // FIXME: this is incorrect, since it will send batch to
                                                            // current peers, instead of previous ones.
        return _z_transport_unicast_send_n_msg_inner(ztu, n_msg, reliability, opt_peers);
#endif
    } else {
#if Z_FEATURE_FRAGMENTATION == 1
        return _z_transport_unicast_send_fragment(ztu, n_msg, reliability, sn, opt_peers);
#else
        return ret;
#endif
    }
}

z_result_t _z_transport_unicast_send_n_msg(_z_transport_unicast_t *ztu, const _z_network_message_t *n_msg,
                                           z_reliability_t reliability, z_congestion_control_t cong_ctrl,
                                           const _z_peer_mask_bitset_t *opt_peers) {
    z_result_t ret = _Z_RES_OK;
    _Z_DEBUG("Send network message");

    // Acquire the lock and drop the message if needed
    if (!_z_transport_batch_hold_tx_mutex()) {
        ret = _z_transport_tx_mutex_lock(&ztu->_common, cong_ctrl == Z_CONGESTION_CONTROL_BLOCK);
    }
    if (ret != _Z_RES_OK) {
        _Z_INFO("Dropping zenoh message because of congestion control");
        return ret;
    }
    ret = _z_transport_unicast_send_n_msg_inner(ztu, n_msg, reliability, opt_peers);
    if (!_z_transport_batch_hold_tx_mutex()) {
        _z_transport_tx_mutex_unlock(&ztu->_common);
    }
    return ret;
}

#if Z_FEATURE_BATCHING == 1
z_result_t _z_transport_unicast_send_n_batch(_z_transport_unicast_t *ztu) {
    z_result_t ret = _Z_RES_OK;
    if (!_z_transport_batch_hold_tx_mutex()) {
        _Z_RETURN_IF_ERR(_z_transport_tx_mutex_lock(&ztu->_common, true));
    }
    if (ztu->_common._batch_count > 0) {
        _Z_DEBUG("Send network batch");
        // FIXME: this is incorrect, since it will send batch to all peers, instead of the ones that were used to create
        // the batch.
        ret = _z_transport_unicast_flush_buffer(ztu, NULL);
    }
    if (!_z_transport_batch_hold_tx_mutex()) {
        _z_transport_tx_mutex_unlock(&ztu->_common);
    }
    return ret;
}
#endif
