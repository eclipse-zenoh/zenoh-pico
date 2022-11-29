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

#include "zenoh-pico/transport/link/tx.h"

#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ Transmission helper ------------------*/
/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - ztu->mutex_tx
 */
void __unsafe_z_prepare_wbuf(_z_wbuf_t *buf, _Bool is_streamed) {
    _z_wbuf_reset(buf);

    if (is_streamed == true) {
        for (uint8_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++) {
            _z_wbuf_put(buf, 0, i);
        }
        _z_wbuf_set_wpos(buf, _Z_MSG_LEN_ENC_SIZE);
    }
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - ztu->mutex_tx
 */
void __unsafe_z_finalize_wbuf(_z_wbuf_t *buf, _Bool is_streamed) {
    if (is_streamed == true) {
        size_t len = _z_wbuf_len(buf) - _Z_MSG_LEN_ENC_SIZE;
        for (uint8_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++) {
            _z_wbuf_put(buf, (uint8_t)((len >> (uint8_t)8 * i) & (uint8_t)0xFF), i);
        }
    }
}

_z_transport_message_t _z_frame_header(z_reliability_t reliability, _Bool is_fragment, _Bool is_final, _z_zint_t sn) {
    // Create the frame session message that carries the zenoh message
    _Bool is_reliable = reliability == Z_RELIABILITY_RELIABLE;

    _z_transport_message_t t_msg = _z_t_msg_make_frame_header(sn, is_reliable, is_fragment, is_final);

    return t_msg;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - ztu->mutex_tx
 */
int8_t __unsafe_z_serialize_zenoh_fragment(_z_wbuf_t *dst, _z_wbuf_t *src, z_reliability_t reliability, size_t sn) {
    int8_t ret = _Z_RES_OK;

    // Assume first that this is not the final fragment
    _Bool is_final = false;
    do {
        size_t w_pos = _z_wbuf_get_wpos(dst);  // Mark the buffer for the writing operation

        _z_transport_message_t f_hdr = _z_frame_header(reliability, true, is_final, sn);
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

int8_t _z_send_t_msg(_z_transport_t *zt, const _z_transport_message_t *t_msg) {
    int8_t ret = _Z_RES_OK;

#if Z_UNICAST_TRANSPORT == 1
    if (zt->_type == _Z_TRANSPORT_UNICAST_TYPE) {
        ret = _z_unicast_send_t_msg(&zt->_transport._unicast, t_msg);
    } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
        if (zt->_type == _Z_TRANSPORT_MULTICAST_TYPE) {
        ret = _z_multicast_send_t_msg(&zt->_transport._multicast, t_msg);
    } else
#endif  // Z_MULTICAST_TRANSPORT == 1
    {
        ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }

    return ret;
}

#if Z_UNICAST_TRANSPORT == 1 || Z_MULTICAST_TRANSPORT == 1
int8_t _z_link_send_t_msg(const _z_link_t *zl, const _z_transport_message_t *t_msg) {
    int8_t ret = _Z_RES_OK;

    // Create and prepare the buffer to serialize the message on
    uint16_t mtu = (zl->_mtu < Z_BATCH_SIZE_TX) ? zl->_mtu : Z_BATCH_SIZE_TX;
    _z_wbuf_t wbf = _z_wbuf_make(mtu, false);
    if (_Z_LINK_IS_STREAMED(zl->_capabilities) == true) {
        for (uint8_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++) {
            _z_wbuf_put(&wbf, 0, i);
        }
        _z_wbuf_set_wpos(&wbf, _Z_MSG_LEN_ENC_SIZE);
    }

    // Encode the session message
    ret = _z_transport_message_encode(&wbf, t_msg);
    if (ret == _Z_RES_OK) {
        // Write the message legnth in the reserved space if needed
        if (_Z_LINK_IS_STREAMED(zl->_capabilities) == true) {
            size_t len = _z_wbuf_len(&wbf) - _Z_MSG_LEN_ENC_SIZE;
            for (uint8_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++) {
                _z_wbuf_put(&wbf, (uint8_t)((len >> (uint8_t)8 * i) & (uint8_t)0xFF), i);
            }
        }

        // Send the wbuf on the socket
        ret = _z_link_send_wbuf(zl, &wbf);
    }
    _z_wbuf_clear(&wbf);

    return ret;
}
#endif  // Z_UNICAST_TRANSPORT == 1 || Z_MULTICAST_TRANSPORT == 1
