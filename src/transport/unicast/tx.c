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

#include <assert.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/codec/network.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/transport/unicast/transport.h"
#include "zenoh-pico/transport/unicast/tx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_UNICAST_TRANSPORT == 1

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - ztu->_mutex_inner
 */
static _z_zint_t __unsafe_z_unicast_get_sn(_z_transport_unicast_t *ztu, z_reliability_t reliability) {
    _z_zint_t sn;
    if (reliability == Z_RELIABILITY_RELIABLE) {
        sn = ztu->_sn_tx_reliable;
        ztu->_sn_tx_reliable = _z_sn_increment(ztu->_sn_res, ztu->_sn_tx_reliable);
    } else {
        sn = ztu->_sn_tx_best_effort;
        ztu->_sn_tx_best_effort = _z_sn_increment(ztu->_sn_res, ztu->_sn_tx_best_effort);
    }
    return sn;
}

#if Z_FEATURE_FRAGMENTATION == 1
static z_result_t __unsafe_z_unicast_send_fragment(_z_transport_unicast_t *ztu, _z_wbuf_t *fbf,
                                                   const _z_network_message_t *n_msg, z_reliability_t reliability,
                                                   _z_zint_t *first_sn) {
    bool is_first = true;
    _z_zint_t sn = *first_sn;
    // Encode message on temp buffer
    _Z_RETURN_IF_ERR(_z_network_message_encode(fbf, n_msg));
    // Fragment message
    while (_z_wbuf_len(fbf) > 0) {
        // Get fragment sequence number
        if (is_first == false) {
            sn = __unsafe_z_unicast_get_sn(ztu, reliability);
        }
        is_first = false;
        // Serialize fragment
        __unsafe_z_prepare_wbuf(&ztu->_wbuf, ztu->_link._cap._flow);
        z_result_t ret = __unsafe_z_serialize_zenoh_fragment(&ztu->_wbuf, fbf, reliability, sn);
        if (ret != _Z_RES_OK) {
            _Z_ERROR("Fragment serialization failed with err %d", ret);
            return ret;
        }
        // Send fragment
        __unsafe_z_finalize_wbuf(&ztu->_wbuf, ztu->_link._cap._flow);
        _Z_RETURN_IF_ERR(_z_link_send_wbuf(&ztu->_link, &ztu->_wbuf));
        ztu->_transmitted = true;  // Tell session we transmitted data
    }
    return _Z_RES_OK;
}
#else
static z_result_t __unsafe_z_unicast_send_fragment(_z_transport_unicast_t *ztu, _z_wbuf_t *fbf,
                                                   const _z_network_message_t *n_msg, z_reliability_t reliability,
                                                   _z_zint_t *first_sn) {
    _ZP_UNUSED(ztu);
    _ZP_UNUSED(fbf);
    _ZP_UNUSED(n_msg);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(first_sn);
    _Z_INFO("Sending the message required fragmentation feature that is deactivated.");
    return _Z_RES_OK;
}
#endif

static z_result_t __unsafe_z_unicast_message_send(_z_transport_unicast_t *ztu, const _z_network_message_t *n_msg,
                                                  z_reliability_t reliability) {
    _Z_DEBUG("Send network message");
    // Encode frame header
    __unsafe_z_prepare_wbuf(&ztu->_wbuf, ztu->_link._cap._flow);
    _z_zint_t sn = __unsafe_z_unicast_get_sn(ztu, reliability);
    _z_transport_message_t t_msg = _z_t_msg_make_frame_header(sn, reliability);
    _Z_RETURN_IF_ERR(_z_transport_message_encode(&ztu->_wbuf, &t_msg));
    // Encode the network message
    z_result_t ret = _z_network_message_encode(&ztu->_wbuf, n_msg);
    // The message does not fit in the current batch, let's fragment it
    if (ret != _Z_RES_OK) {
        // Create an expandable wbuf for fragmentation
        _z_wbuf_t fbf = _z_wbuf_make(_Z_FRAG_BUFF_BASE_SIZE, true);
        // Send message as fragments
        ret = __unsafe_z_unicast_send_fragment(ztu, &fbf, n_msg, reliability, &sn);
        // Clear the buffer as it's no longer required
        _z_wbuf_clear(&fbf);
        return ret;
    }
    // Send network message
    __unsafe_z_finalize_wbuf(&ztu->_wbuf, ztu->_link._cap._flow);
    _Z_RETURN_IF_ERR(_z_link_send_wbuf(&ztu->_link, &ztu->_wbuf));
    ztu->_transmitted = true;  // Tell session we transmitted data
    return _Z_RES_OK;
}

z_result_t _z_unicast_send_t_msg(_z_transport_unicast_t *ztu, const _z_transport_message_t *t_msg) {
    z_result_t ret = _Z_RES_OK;
    _Z_DEBUG("Send session message");
    _z_unicast_tx_mutex_lock(ztu, true);

    // Encode transport message
    __unsafe_z_prepare_wbuf(&ztu->_wbuf, ztu->_link._cap._flow);
    ret = _z_transport_message_encode(&ztu->_wbuf, t_msg);
    if (ret == _Z_RES_OK) {
        // Send message
        __unsafe_z_finalize_wbuf(&ztu->_wbuf, ztu->_link._cap._flow);
        ret = _z_link_send_wbuf(&ztu->_link, &ztu->_wbuf);
        if (ret == _Z_RES_OK) {
            ztu->_transmitted = true;  // Tell session we transmitted data
        }
    }
    _z_unicast_tx_mutex_unlock(ztu);
    return ret;
}

z_result_t _z_unicast_send_n_msg(_z_session_t *zn, const _z_network_message_t *n_msg, z_reliability_t reliability,
                                 z_congestion_control_t cong_ctrl) {
    z_result_t ret = _Z_RES_OK;
    _z_transport_unicast_t *ztu = &zn->_tp._transport._unicast;

    // Acquire the lock and drop the message if needed
    ret = _z_unicast_tx_mutex_lock(ztu, cong_ctrl == Z_CONGESTION_CONTROL_BLOCK);
    if (ret != _Z_RES_OK) {
        _Z_INFO("Dropping zenoh message because of congestion control");
        return ret;
    }
    ret = __unsafe_z_unicast_message_send(ztu, n_msg, reliability);
    _z_unicast_tx_mutex_unlock(ztu);
    return ret;
}
#else
z_result_t _z_unicast_send_t_msg(_z_transport_unicast_t *ztu, const _z_transport_message_t *t_msg) {
    _ZP_UNUSED(ztu);
    _ZP_UNUSED(t_msg);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _z_unicast_send_n_msg(_z_session_t *zn, const _z_network_message_t *n_msg, z_reliability_t reliability,
                                 z_congestion_control_t cong_ctrl) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(n_msg);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(cong_ctrl);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif  // Z_FEATURE_UNICAST_TRANSPORT == 1
