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

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/protocol/codec/core.h"
#include "zenoh-pico/protocol/codec/network.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/definitions/transport.h"
#include "zenoh-pico/transport/raweth/tx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/endianness.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ Transmission helper ------------------*/

static bool _z_transport_tx_get_express_status(const _z_network_message_t *msg) {
    switch (msg->_tag) {
        case _Z_N_DECLARE:
            return _Z_HAS_FLAG(msg->_body._declare._ext_qos._val, _Z_N_QOS_IS_EXPRESS_FLAG);
        case _Z_N_PUSH:
            return _Z_HAS_FLAG(msg->_body._push._qos._val, _Z_N_QOS_IS_EXPRESS_FLAG);
        case _Z_N_REQUEST:
            return _Z_HAS_FLAG(msg->_body._request._ext_qos._val, _Z_N_QOS_IS_EXPRESS_FLAG);
        case _Z_N_RESPONSE:
            return _Z_HAS_FLAG(msg->_body._response._ext_qos._val, _Z_N_QOS_IS_EXPRESS_FLAG);
        default:
            return false;
    }
}
static _z_zint_t _z_transport_tx_get_sn(_z_transport_common_t *ztc, z_reliability_t reliability) {
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
static z_result_t _z_transport_tx_send_fragment_inner(_z_transport_common_t *ztc, _z_wbuf_t *frag_buff,
                                                      const _z_network_message_t *n_msg, z_reliability_t reliability,
                                                      _z_zint_t first_sn, _z_transport_unicast_peer_list_t *peers) {
    bool is_first = true;
    _z_zint_t sn = first_sn;
    // Encode message on temp buffer
    _Z_RETURN_IF_ERR(_z_network_message_encode(frag_buff, n_msg));
    // Fragment message
    while (_z_wbuf_len(frag_buff) > 0) {
        // Get fragment sequence number
        if (!is_first) {
            sn = _z_transport_tx_get_sn(ztc, reliability);
        }
        // Serialize fragment
        __unsafe_z_prepare_wbuf(&ztc->_wbuf, ztc->_link._cap._flow);
        z_result_t ret = __unsafe_z_serialize_zenoh_fragment(&ztc->_wbuf, frag_buff, reliability, sn, is_first);
        if (ret != _Z_RES_OK) {
            _Z_ERROR("Fragment serialization failed with err %d", ret);
            return ret;
        }
        // Send fragment
        __unsafe_z_finalize_wbuf(&ztc->_wbuf, ztc->_link._cap._flow);
        if (peers == NULL) {
            _Z_RETURN_IF_ERR(_z_link_send_wbuf(&ztc->_link, &ztc->_wbuf, NULL));
        } else {
            _z_transport_unicast_peer_list_t *curr_list = peers;
            while (curr_list != NULL) {
                _z_transport_unicast_peer_t *curr_peer = _z_transport_unicast_peer_list_head(curr_list);
                // Send on peer socket
                _z_link_send_wbuf(&ztc->_link, &ztc->_wbuf, &curr_peer->_socket);
                curr_list = _z_transport_unicast_peer_list_tail(curr_list);
            }
        }
        ztc->_transmitted = true;  // Tell session we transmitted data
        is_first = false;
    }
    return _Z_RES_OK;
}

static z_result_t _z_transport_tx_send_fragment(_z_transport_common_t *ztc, const _z_network_message_t *n_msg,
                                                z_reliability_t reliability, _z_zint_t first_sn,
                                                _z_transport_unicast_peer_list_t *peers) {
    // Create an expandable wbuf for fragmentation
    _z_wbuf_t frag_buff = _z_wbuf_make(_Z_FRAG_BUFF_BASE_SIZE, true);
    // Send message as fragments
    z_result_t ret = _z_transport_tx_send_fragment_inner(ztc, &frag_buff, n_msg, reliability, first_sn, peers);
    // Clear the buffer as it's no longer required
    _z_wbuf_clear(&frag_buff);
    return ret;
}

#else
static z_result_t _z_transport_tx_send_fragment(_z_transport_common_t *ztc, const _z_network_message_t *n_msg,
                                                z_reliability_t reliability, _z_zint_t first_sn,
                                                _z_transport_unicast_peer_list_t *peers) {
    _ZP_UNUSED(ztc);
    _ZP_UNUSED(fbf);
    _ZP_UNUSED(n_msg);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(first_sn);
    _ZP_UNUSED(peers)
    _Z_INFO("Sending the message required fragmentation feature that is deactivated.");
    return _Z_RES_OK;
}
#endif

static inline bool _z_transport_tx_batch_has_data(_z_transport_common_t *ztc) {
#if Z_FEATURE_BATCHING == 1
    return (ztc->_batch_state == _Z_BATCHING_ACTIVE) && (ztc->_batch_count > 0);
#else
    _ZP_UNUSED(ztc);
    return false;
#endif
}

static z_result_t _z_transport_tx_flush_buffer(_z_transport_common_t *ztc, _z_transport_unicast_peer_list_t *peers) {
    __unsafe_z_finalize_wbuf(&ztc->_wbuf, ztc->_link._cap._flow);
    // Send network message
    if (peers == NULL) {
        _Z_RETURN_IF_ERR(_z_link_send_wbuf(&ztc->_link, &ztc->_wbuf, NULL));
    } else {
        _z_transport_unicast_peer_list_t *curr_list = peers;
        while (curr_list != NULL) {
            _z_transport_unicast_peer_t *curr_peer = _z_transport_unicast_peer_list_head(curr_list);
            // Send on peer socket
            _z_link_send_wbuf(&ztc->_link, &ztc->_wbuf, &curr_peer->_socket);
            curr_list = _z_transport_unicast_peer_list_tail(curr_list);
        }
    }
    ztc->_transmitted = true;  // Tell session we transmitted data
#if Z_FEATURE_BATCHING == 1
    ztc->_batch_count = 0;
#endif
    return _Z_RES_OK;
}

static z_result_t _z_transport_tx_flush_or_incr_batch(_z_transport_common_t *ztc,
                                                      _z_transport_unicast_peer_list_t *peers) {
#if Z_FEATURE_BATCHING == 1
    if (ztc->_batch_state == _Z_BATCHING_ACTIVE) {
        // Increment batch count
        ztc->_batch_count++;
        return _Z_RES_OK;
    } else {
        return _z_transport_tx_flush_buffer(ztc, peers);
    }
#else
    return _z_transport_tx_flush_buffer(ztc, peers);
#endif
}

static z_result_t _z_transport_tx_batch_overflow(_z_transport_common_t *ztc, const _z_network_message_t *n_msg,
                                                 z_reliability_t reliability, _z_zint_t sn, size_t prev_wpos,
                                                 _z_transport_unicast_peer_list_t *peers) {
#if Z_FEATURE_BATCHING == 1
    // Remove partially encoded data
    _z_wbuf_set_wpos(&ztc->_wbuf, prev_wpos);
    // Send batch
    _Z_RETURN_IF_ERR(_z_transport_tx_flush_buffer(ztc, peers));
    // Init buffer
    __unsafe_z_prepare_wbuf(&ztc->_wbuf, ztc->_link._cap._flow);
    sn = _z_transport_tx_get_sn(ztc, reliability);
    _z_transport_message_t t_msg = _z_t_msg_make_frame_header(sn, reliability);
    _Z_RETURN_IF_ERR(_z_transport_message_encode(&ztc->_wbuf, &t_msg));
    // Retry encode
    bool is_express = _z_transport_tx_get_express_status(n_msg);
    z_result_t ret = _z_network_message_encode(&ztc->_wbuf, n_msg);
    if (ret != _Z_RES_OK) {
        // Message still doesn't fit in buffer, send as fragments
        return _z_transport_tx_send_fragment(ztc, n_msg, reliability, sn, peers);
    } else {
        if (is_express) {
            // Send immediately
            return _z_transport_tx_flush_buffer(ztc, peers);
        } else {
            // Increment batch
            ztc->_batch_count++;
        }
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

static size_t _z_transport_tx_save_wpos(_z_wbuf_t *wbuf) {
#if Z_FEATURE_BATCHING == 1
    return _z_wbuf_get_wpos(wbuf);
#else
    _ZP_UNUSED(wbuf);
    return 0;
#endif
}

static z_result_t _z_transport_tx_send_n_msg_inner(_z_transport_common_t *ztc, const _z_network_message_t *n_msg,
                                                   z_reliability_t reliability,
                                                   _z_transport_unicast_peer_list_t *peers) {
    // Init buffer
    _z_zint_t sn = 0;
    bool batch_has_data = _z_transport_tx_batch_has_data(ztc);
    if (!batch_has_data) {
        __unsafe_z_prepare_wbuf(&ztc->_wbuf, ztc->_link._cap._flow);
        sn = _z_transport_tx_get_sn(ztc, reliability);
        _z_transport_message_t t_msg = _z_t_msg_make_frame_header(sn, reliability);
        _Z_RETURN_IF_ERR(_z_transport_message_encode(&ztc->_wbuf, &t_msg));
    }
    // Try encoding the network message
    size_t prev_wpos = _z_transport_tx_save_wpos(&ztc->_wbuf);
    bool is_express = _z_transport_tx_get_express_status(n_msg);
    z_result_t ret = _z_network_message_encode(&ztc->_wbuf, n_msg);
    if (ret == _Z_RES_OK) {
        if (is_express) {
            // Send immediately
            return _z_transport_tx_flush_buffer(ztc, peers);
        } else {
            // Flush buffer or increase batch
            return _z_transport_tx_flush_or_incr_batch(ztc, peers);
        }
    } else if (!batch_has_data) {
        // Message doesn't fit in buffer, send as fragments
        return _z_transport_tx_send_fragment(ztc, n_msg, reliability, sn, peers);
    } else {
        // Buffer is too full for message
        return _z_transport_tx_batch_overflow(ztc, n_msg, reliability, sn, prev_wpos, peers);
    }
}

static z_result_t _z_transport_tx_send_t_msg_inner(_z_transport_common_t *ztc, const _z_transport_message_t *t_msg,
                                                   _z_transport_unicast_peer_list_t *peers) {
    // Send batch if needed
    bool batch_has_data = _z_transport_tx_batch_has_data(ztc);
    if (batch_has_data) {
        _Z_RETURN_IF_ERR(_z_transport_tx_flush_buffer(ztc, peers));
    }
    // Encode transport message
    __unsafe_z_prepare_wbuf(&ztc->_wbuf, ztc->_link._cap._flow);
    _Z_RETURN_IF_ERR(_z_transport_message_encode(&ztc->_wbuf, t_msg));
    // Send message
    return _z_transport_tx_flush_buffer(ztc, peers);
}

z_result_t _z_transport_tx_send_t_msg(_z_transport_common_t *ztc, const _z_transport_message_t *t_msg,
                                      _z_transport_unicast_peer_list_t *peers) {
    z_result_t ret = _Z_RES_OK;
    _Z_DEBUG("Send session message");
    // If sending to a peer list, make sure the peer mutex is locked
    _z_transport_tx_mutex_lock(ztc, true);

    ret = _z_transport_tx_send_t_msg_inner(ztc, t_msg, peers);

    _z_transport_tx_mutex_unlock(ztc);
    return ret;
}

z_result_t _z_transport_tx_send_t_msg_wrapper(_z_transport_common_t *ztc, const _z_transport_message_t *t_msg) {
    return _z_transport_tx_send_t_msg(ztc, t_msg, NULL);
}

static z_result_t _z_transport_tx_send_n_msg(_z_transport_common_t *ztc, const _z_network_message_t *n_msg,
                                             z_reliability_t reliability, z_congestion_control_t cong_ctrl,
                                             _z_transport_unicast_peer_list_t *peers) {
    z_result_t ret = _Z_RES_OK;
    _Z_DEBUG("Send network message");

    // Acquire the lock and drop the message if needed
    ret = _z_transport_tx_mutex_lock(ztc, cong_ctrl == Z_CONGESTION_CONTROL_BLOCK);
    if (ret != _Z_RES_OK) {
        _Z_INFO("Dropping zenoh message because of congestion control");
        return ret;
    }
    // Process message
    ret = _z_transport_tx_send_n_msg_inner(ztc, n_msg, reliability, peers);
    _z_transport_tx_mutex_unlock(ztc);
    return ret;
}

static z_result_t _z_transport_tx_send_n_batch(_z_transport_common_t *ztc, z_congestion_control_t cong_ctrl,
                                               _z_transport_unicast_peer_list_t *peers) {
#if Z_FEATURE_BATCHING == 1
    // Check batch size
    if (ztc->_batch_count > 0) {
        // Acquire the lock and drop the message if needed
        z_result_t ret = _z_transport_tx_mutex_lock(ztc, cong_ctrl == Z_CONGESTION_CONTROL_BLOCK);
        if (ret != _Z_RES_OK) {
            _Z_INFO("Dropping zenoh batch because of congestion control");
            return ret;
        }
        // Send batch
        _Z_DEBUG("Send network batch");
        ret = _z_transport_tx_flush_buffer(ztc, peers);
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

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - ztu->mutex_tx
 */
void __unsafe_z_prepare_wbuf(_z_wbuf_t *buf, uint8_t link_flow_capability) {
    _z_wbuf_reset(buf);

    switch (link_flow_capability) {
        // Stream capable links
        case Z_LINK_CAP_FLOW_STREAM:
            for (uint8_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++) {
                _z_wbuf_put(buf, 0, i);
            }
            _z_wbuf_set_wpos(buf, _Z_MSG_LEN_ENC_SIZE);
            break;
        // Datagram capable links
        case Z_LINK_CAP_FLOW_DATAGRAM:
        default:
            break;
    }
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - ztu->mutex_tx
 */
void __unsafe_z_finalize_wbuf(_z_wbuf_t *buf, uint8_t link_flow_capability) {
    switch (link_flow_capability) {
        // Stream capable links
        case Z_LINK_CAP_FLOW_STREAM: {
            size_t len = _z_wbuf_len(buf) - _Z_MSG_LEN_ENC_SIZE;
            // Encode the u16 size as little endian
            _z_wbuf_put(buf, _z_get_u16_lsb((uint_fast16_t)len), 0);
            _z_wbuf_put(buf, _z_get_u16_msb((uint_fast16_t)len), 1);
            break;
        }
        // Datagram capable links
        case Z_LINK_CAP_FLOW_DATAGRAM:
        default:
            break;
    }
}

z_result_t _z_send_t_msg(_z_transport_t *zt, const _z_transport_message_t *t_msg) {
    z_result_t ret = _Z_RES_OK;
    switch (zt->_type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            ret = _z_transport_tx_send_t_msg(&zt->_transport._unicast._common, t_msg, NULL);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            ret = _z_transport_tx_send_t_msg(&zt->_transport._multicast._common, t_msg, NULL);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            ret = _z_raweth_send_t_msg(&zt->_transport._raweth._common, t_msg);
            break;
        default:
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            break;
    }
    return ret;
}

z_result_t _z_link_send_t_msg(const _z_link_t *zl, const _z_transport_message_t *t_msg, _z_sys_net_socket_t *socket) {
    z_result_t ret = _Z_RES_OK;

    // Create and prepare the buffer to serialize the message on
    uint16_t mtu = (zl->_mtu < Z_BATCH_UNICAST_SIZE) ? zl->_mtu : Z_BATCH_UNICAST_SIZE;
    _z_wbuf_t wbf = _z_wbuf_make(mtu, false);

    switch (zl->_cap._flow) {
        case Z_LINK_CAP_FLOW_STREAM:
            for (uint8_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++) {
                _z_wbuf_put(&wbf, 0, i);
            }
            _z_wbuf_set_wpos(&wbf, _Z_MSG_LEN_ENC_SIZE);
            break;
        case Z_LINK_CAP_FLOW_DATAGRAM:
            break;
        default:
            ret = _Z_ERR_GENERIC;
            break;
    }
    // Encode the session message
    ret = _z_transport_message_encode(&wbf, t_msg);
    if (ret == _Z_RES_OK) {
        switch (zl->_cap._flow) {
            case Z_LINK_CAP_FLOW_STREAM: {
                // Write the message length in the reserved space if needed
                size_t len = _z_wbuf_len(&wbf) - _Z_MSG_LEN_ENC_SIZE;
                for (uint8_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++) {
                    _z_wbuf_put(&wbf, (uint8_t)((len >> (uint8_t)8 * i) & (uint8_t)0xFF), i);
                }
                break;
            }
            case Z_LINK_CAP_FLOW_DATAGRAM:
                break;
            default:
                ret = _Z_ERR_GENERIC;
                break;
        }
        // Send the wbuf on the socket
        ret = _z_link_send_wbuf(zl, &wbf, socket);
    }
    _z_wbuf_clear(&wbf);

    return ret;
}

z_result_t __unsafe_z_serialize_zenoh_fragment(_z_wbuf_t *dst, _z_wbuf_t *src, z_reliability_t reliability, size_t sn,
                                               bool first) {
    z_result_t ret = _Z_RES_OK;

    // Assume first that this is not the final fragment
    bool is_final = false;
    do {
        size_t w_pos = _z_wbuf_get_wpos(dst);  // Mark the buffer for the writing operation

        _z_transport_message_t f_hdr =
            _z_t_msg_make_fragment_header(sn, reliability == Z_RELIABILITY_RELIABLE, is_final, first, false);
        ret = _z_transport_message_encode(dst, &f_hdr);  // Encode the frame header
        if (ret == _Z_RES_OK) {
            size_t space_left = _z_wbuf_space_left(dst);
            size_t bytes_left = _z_wbuf_len(src);

            if ((is_final == false) && (bytes_left <= space_left)) {  // Check if it is really the final fragment
                _z_wbuf_set_wpos(dst, w_pos);                         // Revert the buffer
                is_final = true;  // It is really the finally fragment, reserialize the header
                continue;
            }

            size_t to_copy = (bytes_left <= space_left) ? bytes_left : space_left;  // Compute bytes to write
            ret = _z_wbuf_siphon(dst, src, to_copy);                                // Write the fragment
        }
        break;
    } while (1);

    return ret;
}

z_result_t _z_send_n_msg(_z_session_t *zn, const _z_network_message_t *z_msg, z_reliability_t reliability,
                         z_congestion_control_t cong_ctrl) {
    z_result_t ret = _Z_RES_OK;
    // Call transport function
    switch (zn->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            if (zn->_mode == Z_WHATAMI_CLIENT) {
                ret = _z_transport_tx_send_n_msg(&zn->_tp._transport._unicast._common, z_msg, reliability, cong_ctrl,
                                                 NULL);
            } else if (_z_transport_unicast_peer_list_len(zn->_tp._transport._unicast._peers) > 0) {
                ret = _z_transport_tx_send_n_msg(&zn->_tp._transport._unicast._common, z_msg, reliability, cong_ctrl,
                                                 zn->_tp._transport._unicast._peers);
            }
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            ret =
                _z_transport_tx_send_n_msg(&zn->_tp._transport._multicast._common, z_msg, reliability, cong_ctrl, NULL);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            ret = _z_raweth_send_n_msg(zn, z_msg, reliability, cong_ctrl);
            break;
        default:
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            break;
    }
    return ret;
}

z_result_t _z_send_n_batch(_z_session_t *zn, z_congestion_control_t cong_ctrl) {
    z_result_t ret = _Z_RES_OK;
    // Call transport function
    switch (zn->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            if (zn->_mode == Z_WHATAMI_CLIENT) {
                ret = _z_transport_tx_send_n_batch(&zn->_tp._transport._unicast._common, cong_ctrl, NULL);
            } else if (_z_transport_unicast_peer_list_len(zn->_tp._transport._unicast._peers) > 0) {
                ret = _z_transport_tx_send_n_batch(&zn->_tp._transport._unicast._common, cong_ctrl,
                                                   zn->_tp._transport._unicast._peers);
            }

            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            ret = _z_transport_tx_send_n_batch(&zn->_tp._transport._multicast._common, cong_ctrl, NULL);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            _Z_INFO("Batching not yet supported on raweth transport");
            ret = _Z_ERR_TRANSPORT_TX_FAILED;
            break;
        default:
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            break;
    }
    return ret;
}
