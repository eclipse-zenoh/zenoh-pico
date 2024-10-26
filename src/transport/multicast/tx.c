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

#include "zenoh-pico/transport/common/tx.h"

#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/codec/network.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/transport/multicast/transport.h"
#include "zenoh-pico/transport/multicast/tx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_MULTICAST_TRANSPORT == 1

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - ztm->_mutex_inner
 */
static _z_zint_t __unsafe_z_multicast_get_sn(_z_transport_multicast_t *ztm, z_reliability_t reliability) {
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

#if Z_FEATURE_FRAGMENTATION == 1
static z_result_t _z_multicast_send_fragment_inner(_z_transport_multicast_t *ztm, _z_wbuf_t *fbf,
                                                   const _z_network_message_t *n_msg, z_reliability_t reliability,
                                                   _z_zint_t first_sn) {
    bool is_first = true;
    _z_zint_t sn = first_sn;
    // Encode message on temp buffer
    _Z_RETURN_IF_ERR(_z_network_message_encode(fbf, n_msg));
    // Fragment message
    while (_z_wbuf_len(fbf) > 0) {
        // Get fragment sequence number
        if (!is_first) {
            sn = __unsafe_z_multicast_get_sn(ztm, reliability);
        }
        is_first = false;
        // Serialize fragment
        __unsafe_z_prepare_wbuf(&ztm->_common._wbuf, ztm->_common._link._cap._flow);
        z_result_t ret = __unsafe_z_serialize_zenoh_fragment(&ztm->_common._wbuf, fbf, reliability, sn);
        if (ret != _Z_RES_OK) {
            _Z_ERROR("Fragment serialization failed with err %d", ret);
            return ret;
        }
        // Send fragment
        __unsafe_z_finalize_wbuf(&ztm->_common._wbuf, ztm->_common._link._cap._flow);
        _Z_RETURN_IF_ERR(_z_link_send_wbuf(&ztm->_common._link, &ztm->_common._wbuf));
        ztm->_common._transmitted = true;  // Tell session we transmitted data
    }
    return _Z_RES_OK;
}

static z_result_t _z_multicast_send_fragment(_z_transport_multicast_t *ztm, const _z_network_message_t *n_msg,
                                             z_reliability_t reliability, _z_zint_t first_sn) {
    // Create an expandable wbuf for fragmentation
    _z_wbuf_t fbf = _z_wbuf_make(_Z_FRAG_BUFF_BASE_SIZE, true);
    // Send message as fragments
    z_result_t ret = _z_multicast_send_fragment_inner(ztm, &fbf, n_msg, reliability, first_sn);
    // Clear the buffer as it's no longer required
    _z_wbuf_clear(&fbf);
    return ret;
}

#else
static z_result_t _z_multicast_send_fragment(_z_transport_multicast_t *ztm, const _z_network_message_t *n_msg,
                                             z_reliability_t reliability, _z_zint_t first_sn) {
    _ZP_UNUSED(ztm);
    _ZP_UNUSED(fbf);
    _ZP_UNUSED(n_msg);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(first_sn);
    _Z_INFO("Sending the message required fragmentation feature that is deactivated.");
    return _Z_RES_OK;
}
#endif

static inline bool _z_multicast_batch_has_data(_z_transport_multicast_t *ztm) {
#if Z_FEATURE_BATCHING == 1
    return (ztm->_batch_state == _Z_BATCHING_ACTIVE) && (ztm->_batch_count > 0);
#else
    _ZP_UNUSED(ztm);
    return false;
#endif
}

static z_result_t __unsafe_z_multicast_flush_buffer(_z_transport_multicast_t *ztm) {
    // Send network message
    __unsafe_z_finalize_wbuf(&ztm->_common._wbuf, ztm->_common._link._cap._flow);
    _Z_RETURN_IF_ERR(_z_link_send_wbuf(&ztm->_common._link, &ztm->_common._wbuf));
    ztm->_common._transmitted = true;  // Tell session we transmitted data
#if Z_FEATURE_BATCHING == 1
    ztm->_batch_count = 0;
#endif
    return _Z_RES_OK;
}

static z_result_t _z_multicast_flush_or_incr_batch(_z_transport_multicast_t *ztm) {
#if Z_FEATURE_BATCHING == 1
    if (ztm->_batch_state == _Z_BATCHING_ACTIVE) {
        // Increment batch count
        ztm->_batch_count++;
        return _Z_RES_OK;
    } else {
        return __unsafe_z_multicast_flush_buffer(ztm);
    }
#else
    return __unsafe_z_multicast_flush_buffer(ztm);
#endif
}

static z_result_t _z_multicast_batch_overflow(_z_transport_multicast_t *ztm, const _z_network_message_t *n_msg,
                                              z_reliability_t reliability, _z_zint_t sn, size_t prev_wpos) {
#if Z_FEATURE_BATCHING == 1
    // Remove partially encoded data
    _z_wbuf_set_wpos(&ztm->_common._wbuf, prev_wpos);
    // Send batch
    _Z_RETURN_IF_ERR(__unsafe_z_multicast_flush_buffer(ztm));
    // Init buffer
    __unsafe_z_prepare_wbuf(&ztm->_common._wbuf, ztm->_common._link._cap._flow);
    sn = __unsafe_z_multicast_get_sn(ztm, reliability);
    _z_transport_message_t t_msg = _z_t_msg_make_frame_header(sn, reliability);
    _Z_RETURN_IF_ERR(_z_transport_message_encode(&ztm->_common._wbuf, &t_msg));
    // Retry encode
    z_result_t ret = _z_network_message_encode(&ztm->_common._wbuf, n_msg);
    if (ret != _Z_RES_OK) {
        // Message still doesn't fit in buffer, send as fragments
        return _z_multicast_send_fragment(ztm, n_msg, reliability, sn);
    } else {
        // Increment batch
        ztm->_batch_count++;
    }
    return _Z_RES_OK;
#else
    _ZP_UNUSED(ztm);
    _ZP_UNUSED(n_msg);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(sn);
    _ZP_UNUSED(prev_wpos);
    return _Z_RES_OK;
#endif
}

static size_t _z_multicast_save_wpos(_z_wbuf_t *wbuf) {
#if Z_FEATURE_BATCHING == 1
    return _z_wbuf_get_wpos(wbuf);
#else
    _ZP_UNUSED(wbuf);
    return 0;
#endif
}

static z_result_t __unsafe_z_multicast_send_n_msg(_z_transport_multicast_t *ztm, const _z_network_message_t *n_msg,
                                                  z_reliability_t reliability) {
    // Init buffer
    _z_zint_t sn = 0;
    bool batch_has_data = _z_multicast_batch_has_data(ztm);
    if (!batch_has_data) {
        __unsafe_z_prepare_wbuf(&ztm->_common._wbuf, ztm->_common._link._cap._flow);
        sn = __unsafe_z_multicast_get_sn(ztm, reliability);
        _z_transport_message_t t_msg = _z_t_msg_make_frame_header(sn, reliability);
        _Z_RETURN_IF_ERR(_z_transport_message_encode(&ztm->_common._wbuf, &t_msg));
    }
    // Try encoding the network message
    size_t prev_wpos = _z_multicast_save_wpos(&ztm->_common._wbuf);
    z_result_t ret = _z_network_message_encode(&ztm->_common._wbuf, n_msg);
    if (ret == _Z_RES_OK) {
        // Flush buffer or increase batch
        return _z_multicast_flush_or_incr_batch(ztm);
    } else if (!batch_has_data) {
        // Message doesn't fit in buffer, send as fragments
        return _z_multicast_send_fragment(ztm, n_msg, reliability, sn);
    } else {
        // Buffer is too full for message
        return _z_multicast_batch_overflow(ztm, n_msg, reliability, sn, prev_wpos);
    }
    return _Z_RES_OK;
}

z_result_t _z_multicast_send_t_msg(_z_transport_multicast_t *ztm, const _z_transport_message_t *t_msg) {
    z_result_t ret = _Z_RES_OK;
    _Z_DEBUG("Send session message");
    _z_transport_tx_mutex_lock(&ztm->_common, true);

    // Encode transport message
    __unsafe_z_prepare_wbuf(&ztm->_common._wbuf, ztm->_common._link._cap._flow);
    ret = _z_transport_message_encode(&ztm->_common._wbuf, t_msg);
    if (ret == _Z_RES_OK) {
        // Send message
        __unsafe_z_finalize_wbuf(&ztm->_common._wbuf, ztm->_common._link._cap._flow);
        ret = _z_link_send_wbuf(&ztm->_common._link, &ztm->_common._wbuf);
        if (ret == _Z_RES_OK) {
            ztm->_common._transmitted = true;  // Tell session we transmitted data
        }
    }
    _z_transport_tx_mutex_unlock(&ztm->_common);
    return ret;
}

z_result_t _z_multicast_send_n_msg(_z_session_t *zn, const _z_network_message_t *n_msg, z_reliability_t reliability,
                                   z_congestion_control_t cong_ctrl) {
    z_result_t ret = _Z_RES_OK;
    _Z_DEBUG("Send network message");
    _z_transport_multicast_t *ztm = &zn->_tp._transport._multicast;

    // Acquire the lock and drop the message if needed
    ret = _z_transport_tx_mutex_lock(&ztm->_common, cong_ctrl == Z_CONGESTION_CONTROL_BLOCK);
    if (ret != _Z_RES_OK) {
        _Z_INFO("Dropping zenoh message because of congestion control");
        return ret;
    }
    // Process message
    ret = __unsafe_z_multicast_send_n_msg(ztm, n_msg, reliability);
    _z_transport_tx_mutex_unlock(&ztm->_common);
    return ret;
}

z_result_t _z_multicast_send_n_batch(_z_session_t *zn, z_congestion_control_t cong_ctrl) {
#if Z_FEATURE_BATCHING == 1
    _z_transport_multicast_t *ztm = &zn->_tp._transport._multicast;
    // Check batch size
    if (ztm->_batch_count > 0) {
        // Acquire the lock and drop the message if needed
        z_result_t ret = _z_transport_tx_mutex_lock(&ztm->_common, cong_ctrl == Z_CONGESTION_CONTROL_BLOCK);
        if (ret != _Z_RES_OK) {
            _Z_INFO("Dropping zenoh batch because of congestion control");
            return ret;
        }
        // Send batch
        _Z_DEBUG("Send network batch");
        ret = __unsafe_z_multicast_flush_buffer(ztm);
        _z_transport_tx_mutex_unlock(&ztm->_common);
        return ret;
    }
    return _Z_RES_OK;
#else
    _ZP_UNUSED(zn);
    _ZP_UNUSED(cong_ctrl);
    return _Z_RES_OK;
#endif
}

#else
z_result_t _z_multicast_send_t_msg(_z_transport_multicast_t *ztm, const _z_transport_message_t *t_msg) {
    _ZP_UNUSED(ztm);
    _ZP_UNUSED(t_msg);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _z_multicast_send_n_msg(_z_session_t *zn, const _z_network_message_t *n_msg, z_reliability_t reliability,
                                   z_congestion_control_t cong_ctrl) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(n_msg);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(cong_ctrl);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _z_multicast_send_n_batch(_z_session_t *zn, z_congestion_control_t cong_ctrl) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(cong_ctrl);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

#endif  // Z_FEATURE_MULTICAST_TRANSPORT == 1
