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
 * Make sure that the tx mutex is locked before calling this function
 */
static _z_zint_t __unsafe_z_multicast_get_sn(_z_transport_common_t *ztc, z_reliability_t reliability) {
    _z_zint_t sn;
    if (reliability == Z_RELIABILITY_RELIABLE) {
        sn = ztc->_sn_tx_reliable;
        ztc->_sn_tx_reliable = _z_sn_increment(ztc->_sn_res, ztc->_sn_tx_reliable);
    } else {
        sn = ztc->_sn_tx_best_effort;
        ztc->_sn_tx_best_effort = _z_sn_increment(ztc->_sn_res, ztc->_sn_tx_best_effort);
    }
    return sn;
}

#if Z_FEATURE_FRAGMENTATION == 1
static z_result_t _z_multicast_send_fragment_inner(_z_transport_common_t *ztc, _z_wbuf_t *frag_buff,
                                                   const _z_network_message_t *n_msg, z_reliability_t reliability,
                                                   _z_zint_t first_sn) {
    bool is_first = true;
    _z_zint_t sn = first_sn;
    // Encode message on temp buffer
    _Z_RETURN_IF_ERR(_z_network_message_encode(frag_buff, n_msg));
    // Fragment message
    while (_z_wbuf_len(frag_buff) > 0) {
        // Get fragment sequence number
        if (!is_first) {
            sn = __unsafe_z_multicast_get_sn(ztc, reliability);
        }
        is_first = false;
        // Serialize fragment
        __unsafe_z_prepare_wbuf(&ztc->_wbuf, ztc->_link._cap._flow);
        z_result_t ret = __unsafe_z_serialize_zenoh_fragment(&ztc->_wbuf, frag_buff, reliability, sn);
        if (ret != _Z_RES_OK) {
            _Z_ERROR("Fragment serialization failed with err %d", ret);
            return ret;
        }
        // Send fragment
        __unsafe_z_finalize_wbuf(&ztc->_wbuf, ztc->_link._cap._flow);
        _Z_RETURN_IF_ERR(_z_link_send_wbuf(&ztc->_link, &ztc->_wbuf));
        ztc->_transmitted = true;  // Tell session we transmitted data
    }
    return _Z_RES_OK;
}

static z_result_t _z_multicast_send_fragment(_z_transport_common_t *ztc, const _z_network_message_t *n_msg,
                                             z_reliability_t reliability, _z_zint_t first_sn) {
    // Create an expandable wbuf for fragmentation
    _z_wbuf_t frag_buff = _z_wbuf_make(_Z_FRAG_BUFF_BASE_SIZE, true);
    // Send message as fragments
    z_result_t ret = _z_multicast_send_fragment_inner(ztc, &frag_buff, n_msg, reliability, first_sn);
    // Clear the buffer as it's no longer required
    _z_wbuf_clear(&frag_buff);
    return ret;
}

#else
static z_result_t _z_multicast_send_fragment(_z_transport_common_t *ztc, const _z_network_message_t *n_msg,
                                             z_reliability_t reliability, _z_zint_t first_sn) {
    _ZP_UNUSED(ztc);
    _ZP_UNUSED(fbf);
    _ZP_UNUSED(n_msg);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(first_sn);
    _Z_INFO("Sending the message required fragmentation feature that is deactivated.");
    return _Z_RES_OK;
}
#endif

static inline bool _z_multicast_batch_has_data(_z_transport_common_t *ztc) {
#if Z_FEATURE_BATCHING == 1
    return (ztc->_batch_state == _Z_BATCHING_ACTIVE) && (ztc->_batch_count > 0);
#else
    _ZP_UNUSED(ztc);
    return false;
#endif
}

static z_result_t __unsafe_z_multicast_flush_buffer(_z_transport_common_t *ztc) {
    // Send network message
    __unsafe_z_finalize_wbuf(&ztc->_wbuf, ztc->_link._cap._flow);
    _Z_RETURN_IF_ERR(_z_link_send_wbuf(&ztc->_link, &ztc->_wbuf));
    ztc->_transmitted = true;  // Tell session we transmitted data
#if Z_FEATURE_BATCHING == 1
    ztc->_batch_count = 0;
#endif
    return _Z_RES_OK;
}

static z_result_t _z_multicast_flush_or_incr_batch(_z_transport_common_t *ztc) {
#if Z_FEATURE_BATCHING == 1
    if (ztc->_batch_state == _Z_BATCHING_ACTIVE) {
        // Increment batch count
        ztc->_batch_count++;
        return _Z_RES_OK;
    } else {
        return __unsafe_z_multicast_flush_buffer(ztc);
    }
#else
    return __unsafe_z_multicast_flush_buffer(ztc);
#endif
}

static z_result_t _z_multicast_batch_overflow(_z_transport_common_t *ztc, const _z_network_message_t *n_msg,
                                              z_reliability_t reliability, _z_zint_t sn, size_t prev_wpos) {
#if Z_FEATURE_BATCHING == 1
    // Remove partially encoded data
    _z_wbuf_set_wpos(&ztc->_wbuf, prev_wpos);
    // Send batch
    _Z_RETURN_IF_ERR(__unsafe_z_multicast_flush_buffer(ztc));
    // Init buffer
    __unsafe_z_prepare_wbuf(&ztc->_wbuf, ztc->_link._cap._flow);
    sn = __unsafe_z_multicast_get_sn(ztc, reliability);
    _z_transport_message_t t_msg = _z_t_msg_make_frame_header(sn, reliability);
    _Z_RETURN_IF_ERR(_z_transport_message_encode(&ztc->_wbuf, &t_msg));
    // Retry encode
    z_result_t ret = _z_network_message_encode(&ztc->_wbuf, n_msg);
    if (ret != _Z_RES_OK) {
        // Message still doesn't fit in buffer, send as fragments
        return _z_multicast_send_fragment(ztc, n_msg, reliability, sn);
    } else {
        // Increment batch
        ztc->_batch_count++;
    }
    return _Z_RES_OK;
#else
    _ZP_UNUSED(ztc);
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

static z_result_t __unsafe_z_multicast_send_n_msg(_z_transport_common_t *ztc, const _z_network_message_t *n_msg,
                                                  z_reliability_t reliability) {
    // Init buffer
    _z_zint_t sn = 0;
    bool batch_has_data = _z_multicast_batch_has_data(ztc);
    if (!batch_has_data) {
        __unsafe_z_prepare_wbuf(&ztc->_wbuf, ztc->_link._cap._flow);
        sn = __unsafe_z_multicast_get_sn(ztc, reliability);
        _z_transport_message_t t_msg = _z_t_msg_make_frame_header(sn, reliability);
        _Z_RETURN_IF_ERR(_z_transport_message_encode(&ztc->_wbuf, &t_msg));
    }
    // Try encoding the network message
    size_t prev_wpos = _z_multicast_save_wpos(&ztc->_wbuf);
    z_result_t ret = _z_network_message_encode(&ztc->_wbuf, n_msg);
    if (ret == _Z_RES_OK) {
        // Flush buffer or increase batch
        return _z_multicast_flush_or_incr_batch(ztc);
    } else if (!batch_has_data) {
        // Message doesn't fit in buffer, send as fragments
        return _z_multicast_send_fragment(ztc, n_msg, reliability, sn);
    } else {
        // Buffer is too full for message
        return _z_multicast_batch_overflow(ztc, n_msg, reliability, sn, prev_wpos);
    }
    return _Z_RES_OK;
}

z_result_t _z_multicast_send_t_msg(_z_transport_common_t *ztc, const _z_transport_message_t *t_msg) {
    z_result_t ret = _Z_RES_OK;
    _Z_DEBUG("Send session message");
    _z_transport_tx_mutex_lock(ztc, true);

    // Encode transport message
    __unsafe_z_prepare_wbuf(&ztc->_wbuf, ztc->_link._cap._flow);
    ret = _z_transport_message_encode(&ztc->_wbuf, t_msg);
    if (ret == _Z_RES_OK) {
        // Send message
        __unsafe_z_finalize_wbuf(&ztc->_wbuf, ztc->_link._cap._flow);
        ret = _z_link_send_wbuf(&ztc->_link, &ztc->_wbuf);
        if (ret == _Z_RES_OK) {
            ztc->_transmitted = true;  // Tell session we transmitted data
        }
    }
    _z_transport_tx_mutex_unlock(ztc);
    return ret;
}

z_result_t _z_multicast_send_n_msg(_z_transport_common_t *ztc, const _z_network_message_t *n_msg, z_reliability_t reliability,
                                   z_congestion_control_t cong_ctrl) {
    z_result_t ret = _Z_RES_OK;
    _Z_DEBUG("Send network message");

    // Acquire the lock and drop the message if needed
    ret = _z_transport_tx_mutex_lock(ztc, cong_ctrl == Z_CONGESTION_CONTROL_BLOCK);
    if (ret != _Z_RES_OK) {
        _Z_INFO("Dropping zenoh message because of congestion control");
        return ret;
    }
    // Process message
    ret = __unsafe_z_multicast_send_n_msg(ztc, n_msg, reliability);
    _z_transport_tx_mutex_unlock(ztc);
    return ret;
}

z_result_t _z_multicast_send_n_batch(_z_transport_common_t *ztc, z_congestion_control_t cong_ctrl) {
#if Z_FEATURE_BATCHING == 1
    // Check batch size
    if (ztm->_batch_count > 0) {
        // Acquire the lock and drop the message if needed
        z_result_t ret = _z_transport_tx_mutex_lock(ztc, cong_ctrl == Z_CONGESTION_CONTROL_BLOCK);
        if (ret != _Z_RES_OK) {
            _Z_INFO("Dropping zenoh batch because of congestion control");
            return ret;
        }
        // Send batch
        _Z_DEBUG("Send network batch");
        ret = __unsafe_z_multicast_flush_buffer(ztc);
        _z_transport_tx_mutex_unlock(ztc);
        return ret;
    }
    return _Z_RES_OK;
#else
    _ZP_UNUSED(ztc);
    _ZP_UNUSED(cong_ctrl);
    return _Z_RES_OK;
#endif
}

#else
z_result_t _z_multicast_send_t_msg(_z_transport_common_t *ztc, const _z_transport_message_t *t_msg) {
    _ZP_UNUSED(ztc);
    _ZP_UNUSED(t_msg);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _z_multicast_send_n_msg(_z_transport_common_t *ztc, const _z_network_message_t *n_msg, z_reliability_t reliability,
                                   z_congestion_control_t cong_ctrl) {
    _ZP_UNUSED(ztc);
    _ZP_UNUSED(n_msg);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(cong_ctrl);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _z_multicast_send_n_batch(_z_transport_common_t *ztc, z_congestion_control_t cong_ctrl) {
    _ZP_UNUSED(ztc);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(cong_ctrl);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

#endif  // Z_FEATURE_MULTICAST_TRANSPORT == 1
